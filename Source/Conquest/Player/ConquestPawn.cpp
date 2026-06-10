#include "ConquestPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

AConquestPawn::AConquestPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);

	CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->SetCanEverAffectNavigation(false);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(CapsuleComponent);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CapsuleComponent);

	// Fixed camera boom. It does not rotate from controller/mouse input.
	CameraBoom->TargetArmLength = 0.0f;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->SetRelativeRotation(FixedCameraRotation);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom);
	Camera->bUsePawnControlRotation = false;

	FloatingMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
	FloatingMovement->UpdatedComponent = CapsuleComponent;
	FloatingMovement->MaxSpeed = FlySpeed;
	FloatingMovement->Acceleration = Acceleration;
	FloatingMovement->Deceleration = Deceleration;
	FloatingMovement->TurningBoost = TurningBoost;

	bReplicates = true;
	SetReplicateMovement(true);

	// Mouse/controller movement must not rotate the pawn.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void AConquestPawn::BeginPlay()
{
	Super::BeginPlay();

	if (FloatingMovement)
	{
		FloatingMovement->MaxSpeed = FlySpeed;
		FloatingMovement->Acceleration = Acceleration;
		FloatingMovement->Deceleration = Deceleration;
		FloatingMovement->TurningBoost = TurningBoost;
	}

	if (CameraBoom)
	{
		CameraBoom->SetRelativeRotation(FixedCameraRotation);
	}

	ConfigurePlayerControllerForGameAndUI();
}

void AConquestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AConquestPawn::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AConquestPawn::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("MoveUp"), this, &AConquestPawn::MoveUp);

	// Add this axis mapping in Project Settings -> Input:
	// Zoom
	//     Mouse Wheel Axis = +1
	PlayerInputComponent->BindAxis(TEXT("Zoom"), this, &AConquestPawn::Zoom);

	// Intentionally no Turn / LookUp bindings.
	// Mouse movement is reserved for cursor hover / tile selection.
}

void AConquestPawn::MoveForward(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	FVector Direction = GetActorForwardVector();

	if (bMoveRelativeToCameraYaw && Camera)
	{
		const FRotator CameraRotation = Camera->GetComponentRotation();
		const FRotator YawOnlyRotation(0.0f, CameraRotation.Yaw, 0.0f);
		Direction = FRotationMatrix(YawOnlyRotation).GetUnitAxis(EAxis::X);
	}

	Direction.Z = 0.0f;
	Direction.Normalize();

	AddMovementInput(Direction, Value);
}

void AConquestPawn::MoveRight(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	FVector Direction = GetActorRightVector();

	if (bMoveRelativeToCameraYaw && Camera)
	{
		const FRotator CameraRotation = Camera->GetComponentRotation();
		const FRotator YawOnlyRotation(0.0f, CameraRotation.Yaw, 0.0f);
		Direction = FRotationMatrix(YawOnlyRotation).GetUnitAxis(EAxis::Y);
	}

	Direction.Z = 0.0f;
	Direction.Normalize();

	AddMovementInput(Direction, Value);
}

void AConquestPawn::MoveUp(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddMovementInput(FVector::UpVector, Value);
}

void AConquestPawn::Zoom(float Value)
{
	if (FMath::IsNearlyZero(Value) || !Camera)
	{
		return;
	}

	// Scroll zoom moves along the fixed camera forward vector.
	// Because the camera is pitched downward, zooming in moves forward and downward.
	// Zooming out moves backward and upward.
	const FVector ZoomDirection = Camera->GetForwardVector();

	AddActorWorldOffset(
		ZoomDirection * Value * ZoomSpeed * GetWorld()->GetDeltaSeconds(),
		true
	);

	ClampZoomHeightIfNeeded();
}

void AConquestPawn::ClampZoomHeightIfNeeded()
{
	if (!bClampZoomHeight)
	{
		return;
	}

	FVector Location = GetActorLocation();
	Location.Z = FMath::Clamp(Location.Z, MinZoomHeight, MaxZoomHeight);
	SetActorLocation(Location);
}

void AConquestPawn::ConfigurePlayerControllerForGameAndUI()
{
	if (!bConfigureGameAndUIInputMode)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	PlayerController->bShowMouseCursor = true;
	PlayerController->bEnableClickEvents = bEnableClickEvents;
	PlayerController->bEnableMouseOverEvents = bEnableMouseOverEvents;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	PlayerController->SetInputMode(InputMode);
}