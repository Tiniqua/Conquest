#include "ConquestPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
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
	CameraBoom->TargetArmLength = 0.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = false;

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

	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
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
}

void AConquestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AConquestPawn::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AConquestPawn::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("MoveUp"), this, &AConquestPawn::MoveUp);

	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AConquestPawn::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AConquestPawn::LookUp);
}

void AConquestPawn::MoveForward(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AConquestPawn::MoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AConquestPawn::MoveUp(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddMovementInput(FVector::UpVector, Value);
	}
}

void AConquestPawn::Turn(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddControllerYawInput(Value * BaseTurnRate);
	}
}

void AConquestPawn::LookUp(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddControllerPitchInput(Value * BaseLookUpRate);
	}
}