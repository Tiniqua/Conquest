#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConquestContentManager.generated.h"

class AConquestGameState;
class UConquestCivilisationData;

struct FConquestBuildingRow;
struct FConquestTechRow;
struct FConquestUnitRow;
struct FConquestUnitAugmentRow;
struct FConquestPhilosophyRow;

UCLASS(BlueprintType)
class CONQUEST_API UConquestContentManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AConquestGameState* InGameState);

	const FConquestBuildingRow* FindBuilding(FName BuildingId) const;
	const FConquestTechRow* FindTech(FName TechId) const;
	const FConquestUnitRow* FindUnit(FName UnitId) const;
	const FConquestUnitAugmentRow* FindUnitAugment(FName AugmentId) const;
	const FConquestPhilosophyRow* FindPhilosophy(FName PhilosophyId) const;

	FName ResolveBuildingIdForPlayer(int32 PlayerId, FName BaseBuildingId) const;
	FName ResolveUnitIdForPlayer(int32 PlayerId, FName BaseUnitId) const;

	const FConquestBuildingRow* ResolveBuildingForPlayer(int32 PlayerId, FName BaseBuildingId) const;
	const FConquestUnitRow* ResolveUnitForPlayer(int32 PlayerId, FName BaseUnitId) const;

	void GetAllBaseBuildings(TArray<const FConquestBuildingRow*>& OutRows) const;
	void GetAllBaseUnits(TArray<const FConquestUnitRow*>& OutRows) const;
	void GetAllUnitAugments(TArray<const FConquestUnitAugmentRow*>& OutRows) const;
	void GetStartingBuildingIdsForPlayer(int32 PlayerId, TArray<FName>& OutBuildingIds) const;
	void GetAllTechs(TArray<const FConquestTechRow*>& OutRows) const;

private:
	UPROPERTY()
	TObjectPtr<AConquestGameState> GameStateRef;
};
