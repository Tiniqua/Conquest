#include "ConquestPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestTurnManager.h"
#include "Conquest/UI/ConquestHUD.h"
#include "Conquest/World/Generation/HexTileTypes.h"
#include "Conquest/World/Generation/ModularHexGridActor.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

static FString HexTileTypeToString(EHexTileType TileType)
{
	if (const UEnum* EnumPtr = StaticEnum<EHexTileType>())
	{
		return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(TileType)).ToString();
	}

	return TEXT("Unknown");
}

static FString HexFeatureTypeToString(EHexFeatureType FeatureType)
{
	if (const UEnum* EnumPtr = StaticEnum<EHexFeatureType>())
	{
		return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(FeatureType)).ToString();
	}

	return TEXT("Unknown");
}

static FString FormatTileFeatures(const FHexTileData& TileData)
{
	TArray<FString> FeatureNames;

	for (const EHexFeatureType Feature : TileData.Features)
	{
		if (Feature != EHexFeatureType::None)
		{
			FeatureNames.Add(HexFeatureTypeToString(Feature));
		}
	}

	return FeatureNames.Num() > 0
		? FString::Join(FeatureNames, TEXT(", "))
		: TEXT("None");
}

static FString FormatTileResource(const FHexTileData& TileData)
{
	if (!TileData.Resource.HasResource())
	{
		return TEXT("None");
	}

	return TileData.Resource.Quantity > 0
		? FString::Printf(
			TEXT("%s x%d"),
			*TileData.Resource.ResourceId.ToString(),
			TileData.Resource.Quantity
		)
		: TileData.Resource.ResourceId.ToString();
}

static FString FormatTileImprovement(const FHexTileData& TileData)
{
	return TileData.ImprovementId.IsNone()
		? TEXT("None")
		: TileData.ImprovementId.ToString();
}

AConquestPawn::AConquestPawn()
{
	PrimaryActorTick.bCanEverTick = true;
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

void AConquestPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bEnableTileHoverDebug || !Camera)
	{
		ClearHoveredTileInfoWidget();
		ClearHoveredTileVisual();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		ClearHoveredTileInfoWidget();
		ClearHoveredTileVisual();
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		ClearHoveredTileInfoWidget();
		ClearHoveredTileVisual();
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;

	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		ClearHoveredTileInfoWidget();
		ClearHoveredTileVisual();
		return;
	}

	FVector WorldOrigin;
	FVector WorldDirection;

	if (!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDirection))
	{
		ClearHoveredTileInfoWidget();
		ClearHoveredTileVisual();
		return;
	}

	const FVector TraceStart = WorldOrigin;
	const FVector TraceEnd = TraceStart + WorldDirection * TileHoverTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TileHoverTrace), true);
	QueryParams.AddIgnoredActor(this);

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		TileHoverTraceChannel,
		QueryParams
	);

	if (!bHit)
	{
		ClearHoveredTileInfoWidget();
		ClearHoveredTileVisual();
		return;
	}

	AModularHexGridActor* HexGridActor = Cast<AModularHexGridActor>(Hit.GetActor());

	if (!HexGridActor && Hit.GetComponent())
	{
		HexGridActor = Cast<AModularHexGridActor>(Hit.GetComponent()->GetOwner());
	}

	if (!HexGridActor)
	{
		ClearHoveredTileInfoWidget();
		ClearHoveredTileVisual();
		return;
	}

	int32 HoveredQ = INDEX_NONE;
	int32 HoveredR = INDEX_NONE;
	FHexTileData HoveredTile;

	if (!HexGridActor->GetTileAtWorldLocation(Hit.ImpactPoint, HoveredQ, HoveredR, HoveredTile))
	{
		ClearHoveredTileInfoWidget();
		ClearHoveredTileVisual();
		return;
	}

	UpdateHoveredTileInfoWidget(HoveredQ, HoveredR, HoveredTile);
	UpdateHoveredTileVisual(HexGridActor, HoveredQ, HoveredR);
}

void AConquestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AConquestPawn::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AConquestPawn::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("MoveUp"), this, &AConquestPawn::MoveUp);

	PlayerInputComponent->BindAxis(TEXT("Zoom"), this, &AConquestPawn::Zoom);
	PlayerInputComponent->BindAction(TEXT("PrimaryClick"), IE_Pressed, this, &AConquestPawn::HandlePrimaryClick);

	PlayerInputComponent->BindAction(TEXT("ToggleFogOfWar"), IE_Pressed, this, &AConquestPawn::ToggleFogOfWar);
	PlayerInputComponent->BindAction(TEXT("ToggleHexGridOverlay"), IE_Pressed, this, &AConquestPawn::ToggleHexGridOverlay);
	PlayerInputComponent->BindAction(TEXT("RegenerateGrid"), IE_Pressed, this, &AConquestPawn::RegenerateMapWithNewSeed);
	// Intentionally no Turn / LookUp bindings.
	// Mouse movement is reserved for cursor hover / tile selection.
}

void AConquestPawn::ClearHoveredTileVisual()
{
	if (LastHoveredGridActor)
	{
		LastHoveredGridActor->ClearHoveredTile();
	}

	LastHoveredGridActor = nullptr;
	LastHoveredQ = INDEX_NONE;
	LastHoveredR = INDEX_NONE;
}

void AConquestPawn::UpdateHoveredTileVisual(AModularHexGridActor* HexGridActor, int32 Q, int32 R)
{
	if (!HexGridActor)
	{
		ClearHoveredTileVisual();
		return;
	}

	if (LastHoveredGridActor == HexGridActor && LastHoveredQ == Q && LastHoveredR == R)
	{
		return;
	}

	if (LastHoveredGridActor && LastHoveredGridActor != HexGridActor)
	{
		LastHoveredGridActor->ClearHoveredTile();
	}

	LastHoveredGridActor = HexGridActor;
	LastHoveredQ = Q;
	LastHoveredR = R;

	HexGridActor->SetHoveredTile(Q, R);
}

bool AConquestPawn::GetTileUnderMouse(
	AModularHexGridActor*& OutGridActor,
	int32& OutQ,
	int32& OutR,
	FHexTileData& OutTileData
) const
{
	OutGridActor = nullptr;
	OutQ = INDEX_NONE;
	OutR = INDEX_NONE;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return false;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;

	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;

	if (!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDirection))
	{
		return false;
	}

	const FVector TraceStart = WorldOrigin;
	const FVector TraceEnd = TraceStart + WorldDirection * TileHoverTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TileClickTrace), true);
	QueryParams.AddIgnoredActor(this);

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		TileHoverTraceChannel,
		QueryParams
	);

	if (!bHit)
	{
		return false;
	}

	AModularHexGridActor* HexGridActor = Cast<AModularHexGridActor>(Hit.GetActor());

	if (!HexGridActor && Hit.GetComponent())
	{
		HexGridActor = Cast<AModularHexGridActor>(Hit.GetComponent()->GetOwner());
	}

	if (!HexGridActor)
	{
		return false;
	}

	if (!HexGridActor->GetTileAtWorldLocation(Hit.ImpactPoint, OutQ, OutR, OutTileData))
	{
		return false;
	}

	OutGridActor = HexGridActor;
	return true;
}

void AConquestPawn::TryOpenCityPanelAtTile(const FIntPoint& Coord)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AConquestGameState* ConquestGS = World->GetGameState<AConquestGameState>();
	if (!ConquestGS || !ConquestGS->CityManager)
	{
		return;
	}

	const int32 CityId = ConquestGS->CityManager->FindCityAtTile(Coord);
	if (CityId == INDEX_NONE)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PlayerController->GetHUD());
	if (!ConquestHUD)
	{
		return;
	}

	ConquestHUD->ShowCityPanel(CityId);
}

void AConquestPawn::HandlePrimaryClick()
{
	AModularHexGridActor* HexGridActor = nullptr;
	int32 Q = INDEX_NONE;
	int32 R = INDEX_NONE;
	FHexTileData TileData;

	if (!GetTileUnderMouse(HexGridActor, Q, R, TileData))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AConquestGameState* ConquestGS = World->GetGameState<AConquestGameState>();
	AConquestGameMode* ConquestGM = World->GetAuthGameMode<AConquestGameMode>();

	if (!ConquestGS || !ConquestGM || !ConquestGS->TurnManager)
	{
		return;
	}

	if (!ConquestGS->ActiveGridActor && HexGridActor)
	{
		ConquestGS->ActiveGridActor = HexGridActor;
	}

	const FIntPoint ClickedCoord(Q, R);

	// First click: found capital.
	if (ConquestGS->TurnManager->CurrentPhase == EConquestTurnPhase::AwaitingFirstCity)
	{
		const bool bFounded = ConquestGM->FoundStartingCity(
			ClickedCoord,
			FName(TEXT("Capital"))
		);

		if (bFounded)
		{
			TryOpenCityPanelAtTile(ClickedCoord);
		}

		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PlayerController->GetHUD()))
		{
			if (ConquestHUD->SelectExpansionCandidateTile(Q, R))
			{
				return;
			}
		}
	}

	// Normal click: if this tile has a city, open its city panel.
	TryOpenCityPanelAtTile(ClickedCoord);

	// Later:
	// select unit / tile action panel / improvement actions.
}

AModularHexGridActor* AConquestPawn::FindHexGridActor() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return Cast<AModularHexGridActor>(UGameplayStatics::GetActorOfClass(World, AModularHexGridActor::StaticClass()));
}

void AConquestPawn::ToggleFogOfWar()
{
	AModularHexGridActor* HexGridActor = FindHexGridActor();
	if (!HexGridActor)
	{
		return;
	}

	HexGridActor->SetFogOfWarVisible(!HexGridActor->IsFogOfWarVisible());
}

void AConquestPawn::ToggleHexGridOverlay()
{
	AModularHexGridActor* HexGridActor = FindHexGridActor();
	if (!HexGridActor)
	{
		return;
	}

	HexGridActor->SetHexGridOverlayVisible(!HexGridActor->IsHexGridOverlayVisible());
}

void AConquestPawn::RegenerateMapWithNewSeed()
{
	AModularHexGridActor* HexGridActor = FindHexGridActor();
	if (!HexGridActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("RegenerateMapWithNewSeed failed: no ModularHexGridActor found."));
		return;
	}

	ClearHoveredTileInfoWidget();

	HexGridActor->RegenerateGridWithNewRandomSeed();
}

UConquestGameWidget* AConquestPawn::GetActiveGameWidget() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return nullptr;
	}

	AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PlayerController->GetHUD());
	if (!ConquestHUD)
	{
		return nullptr;
	}

	return ConquestHUD->GetActiveGameWidget();
}

void AConquestPawn::ClearHoveredTileInfoWidget() const
{
	if (UConquestGameWidget* GameWidget = GetActiveGameWidget())
	{
		GameWidget->ClearHoveredTileInfo();
	}
}

void AConquestPawn::UpdateHoveredTileInfoWidget(int32 Q, int32 R, const FHexTileData& TileData) const
{
	UConquestGameWidget* GameWidget = GetActiveGameWidget();
	if (!GameWidget)
	{
		return;
	}

	FHoveredHexTileWidgetData WidgetData;
	WidgetData.Q = Q;
	WidgetData.R = R;
	WidgetData.TileType = HexTileTypeToString(TileData.TileType);
	WidgetData.Features = FormatTileFeatures(TileData);
	WidgetData.Resource = FormatTileResource(TileData);
	WidgetData.Improvement = FormatTileImprovement(TileData);
	WidgetData.FreshWater = TileData.bHasFreshWater ? TEXT("Yes") : TEXT("No");
	WidgetData.River = TileData.bHasRiver ? TEXT("Yes") : TEXT("No");
	WidgetData.Height = TileData.Height;
	WidgetData.Yield = TileData.FinalYield;

	GameWidget->UpdateHoveredTileInfo(WidgetData);
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
