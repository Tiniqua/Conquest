#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ConquestPawn.generated.h"

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

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Visual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFloatingPawnMovement* FloatingMovement = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FlySpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float Acceleration = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float Deceleration = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TurningBoost = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float BaseTurnRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float BaseLookUpRate = 1.0f;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);

	void Turn(float Value);
	void LookUp(float Value);
};