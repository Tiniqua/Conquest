#pragma once

#include "CoreMinimal.h"
#include "Conquest/UI/ConquestGameWidget.h"
#include "GameFramework/Pawn.h"
#include "ConquestPawn.generated.h"

class AModularHexGridActor;
class UCapsuleComponent;
class UCameraComponent;
class USpringArmComponent;
class UFloatingPawnMovement;
class UStaticMeshComponent;

UCLASS()
class CONQUEST_API AConquestPawn : public APawn
{
	GENERATED_BODY()

public:
	AConquestPawn();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UConquestGameWidget* GetActiveGameWidget() const;
	void ClearHoveredTileInfoWidget() const;
	void UpdateHoveredTileInfoWidget(int32 Q, int32 R, const FHexTileData& TileData) const;

	void HandlePrimaryPressed();
	void HandlePrimaryReleased();
	void HandlePrimaryClick();
	void HandleSecondaryPressed();
	void HandleSecondaryReleased();
	void HandleSecondaryClick();
	bool GetTileUnderMouse(AModularHexGridActor*& OutGridActor, int32& OutQ, int32& OutR, FHexTileData& OutTileData) const;

	void TryOpenCityPanelAtTile(const FIntPoint& Coord);
	void FocusCameraOnHex(AModularHexGridActor* HexGridActor, const FIntPoint& Coord, bool bPreserveCurrentHeight);
	void FocusCameraOnWorldLocation(const FVector& TargetWorldLocation, bool bPreserveCurrentHeight);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Hover Debug")
	bool bEnableTileHoverDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Hover Debug")
	float TileHoverTraceDistance = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Hover Debug")
	TEnumAsByte<ECollisionChannel> TileHoverTraceChannel = ECC_Visibility;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Visual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> FloatingMovement = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FlySpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float Acceleration = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float Deceleration = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TurningBoost = 8.0f;

	// Fixed camera angle. Negative pitch looks down.
	// Example: -35 to -45 gives a Civ-style angled view.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator FixedCameraRotation = FRotator(-40.0f, 0.0f, 0.0f);

	// If true, MoveForward / MoveRight use the fixed camera yaw but stay flat on the XY plane.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bMoveRelativeToCameraYaw = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Drag Pan")
	bool bEnableLeftMouseDragPan = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Drag Pan", meta = (EditCondition = "bEnableLeftMouseDragPan"))
	float DragPanWorldUnitsPerPixel = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Drag Pan", meta = (EditCondition = "bEnableLeftMouseDragPan"))
	float DragPanZoomScale = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Drag Pan", meta = (EditCondition = "bEnableLeftMouseDragPan"))
	float DragPanClickThresholdPixels = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Drag Pan", meta = (EditCondition = "bEnableLeftMouseDragPan"))
	bool bSmoothDragPan = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Drag Pan", meta = (EditCondition = "bEnableLeftMouseDragPan && bSmoothDragPan", ClampMin = "0.1"))
	float DragPanSmoothingSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus")
	float CameraFocusHeightAboveTarget = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus")
	float CameraFocusBackwardOffset = 1000.0f;

	// Scroll zoom movement speed.
	// This moves along the actual camera forward vector, not world up/down.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom")
	float ZoomSpeed = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom")
	bool bSmoothZoom = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (EditCondition = "bSmoothZoom", ClampMin = "0.1"))
	float ZoomSmoothingSpeed = 14.0f;

	// Optional bounds so the camera cannot zoom too close or too far.
	// This clamps actor world Z after zoom movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom")
	bool bClampZoomHeight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (EditCondition = "bClampZoomHeight"))
	float MinZoomHeight = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom", meta = (EditCondition = "bClampZoomHeight"))
	float MaxZoomHeight = 5000.0f;

	// If true, BeginPlay configures the local controller for visible cursor + Game/UI input.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bConfigureGameAndUIInputMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bEnableClickEvents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bEnableMouseOverEvents = true;

	UPROPERTY(Transient)
	TObjectPtr<AModularHexGridActor> LastHoveredGridActor = nullptr;

	int32 LastHoveredQ = INDEX_NONE;
	int32 LastHoveredR = INDEX_NONE;
	bool bPrimaryMouseDown = false;
	bool bPrimaryDragPanning = false;
	bool bPrimaryPressStartedOverWorld = false;
	bool bSecondaryClickHandledOnPress = false;
	FVector2D PrimaryPressMousePosition = FVector2D::ZeroVector;
	FVector2D LastDragMousePosition = FVector2D::ZeroVector;
	bool bHasSmoothedCameraTarget = false;
	FVector SmoothedCameraTargetLocation = FVector::ZeroVector;
	float ActiveCameraSmoothingSpeed = 0.0f;

	void ClearHoveredTileVisual();
	void UpdateHoveredTileVisual(AModularHexGridActor* HexGridActor, int32 Q, int32 R);

	void ToggleFogOfWar();
	void ToggleHexGridOverlay();
	void ToggleTileYieldOverlay();
	void ToggleFoodYieldLens();
	void ToggleProductionYieldLens();
	void ToggleScienceYieldLens();
	void ToggleCultureYieldLens();
	void ToggleGoldYieldLens();
	void RegenerateMapWithNewSeed();
	void HandleEnterShortcut();
	void ToggleSpecificTileYieldLens(EConquestYieldType YieldType);

	AModularHexGridActor* FindHexGridActor() const;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);

	void Zoom(float Value);
	void UpdateDragPan(float DeltaTime);
	void AddCameraOffset(const FVector& Offset, bool bUseSmoothing, float SmoothingSpeed);
	void UpdateSmoothedCameraMovement(float DeltaTime);
	void ResetSmoothedCameraTarget();
	FVector ClampZoomHeightForLocation(const FVector& Location) const;

	void ConfigurePlayerControllerForGameAndUI();
	void ClampZoomHeightIfNeeded();
};
