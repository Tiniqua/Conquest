#pragma once

#include "CoreMinimal.h"
#include "ConquestChoiceTypes.generated.h"

UENUM(BlueprintType)
enum class EConquestChoiceType : uint8
{
	None,

	ProductionBuilding,
	ProductionUnit,
	ProductionWonder,
	ProductionProject,

	ResearchTech,
	PolicyTenet,
	TileImprovement,

	GenericAction
};

USTRUCT(BlueprintType)
struct FConquestChoiceButtonData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConquestChoiceType ChoiceType = EConquestChoiceType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ChoiceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DetailText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Turns = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisabledReason;

	// Optional payload. Useful for UConquestBuildingData, UConquestTechData, UConquestUnitData, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UObject> Payload = nullptr;
};