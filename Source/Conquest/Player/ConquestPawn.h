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

protected:
	virtual void BeginPlay() override;
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

	// Scroll zoom movement speed.
	// This moves along the actual camera forward vector, not world up/down.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zoom")
	float ZoomSpeed = 1800.0f;

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

	void ClearHoveredTileVisual();
	void UpdateHoveredTileVisual(AModularHexGridActor* HexGridActor, int32 Q, int32 R);

	void ToggleFogOfWar();
	void ToggleHexGridOverlay();
	void RegenerateMapWithNewSeed();

	AModularHexGridActor* FindHexGridActor() const;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);

	void Zoom(float Value);

	void ConfigurePlayerControllerForGameAndUI();
	void ClampZoomHeightIfNeeded();
};