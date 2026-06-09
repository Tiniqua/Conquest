#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ConquestPlayerController.generated.h"

UCLASS()
class CONQUEST_API AConquestPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AConquestPlayerController();

protected:
	virtual void BeginPlay() override;
};