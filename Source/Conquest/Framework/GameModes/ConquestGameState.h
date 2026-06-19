#pragma once

#include "CoreMinimal.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Units/ConquestUnitActor.h"
#include "GameFramework/GameStateBase.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/World/Generation/ModularHexGridActor.h"
#include "ConquestGameState.generated.h"

class UConquestContentManager;
class UConquestTurnManager;
class UConquestCityManager;
class UConquestYieldManager;
class UConquestTechManager;
class UConquestBuildingData;
class UConquestTechData;
class FHexGridModel;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConquestStateChanged);

UCLASS()
class CONQUEST_API AConquestGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AConquestGameState();

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable)
	FOnConquestStateChanged OnConquestStateChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestTurnManager> TurnManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestCityManager> CityManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestYieldManager> YieldManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestTechManager> TechManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UConquestContentManager> ContentManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> BuildingTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> UnitTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> UnitAugmentTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> TechTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conquest|Data")
	TObjectPtr<UDataTable> PhilosophyTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Players")
	TObjectPtr<UConquestCivilisationData> HumanCivilisation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Players")
	FConquestPlayerEmpireState HumanPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Conquest|Units")
	TSubclassOf<AConquestUnitActor> UnitActorClass;

	UPROPERTY(BlueprintReadOnly, Category="Conquest|Units")
	TMap<int32, TObjectPtr<AConquestUnitActor>> UnitActorsByInstanceId;

	UPROPERTY(BlueprintReadOnly, Category="Conquest|Units")
	int32 SelectedUnitInstanceId = INDEX_NONE;

	UFUNCTION(BlueprintCallable)
	void BroadcastStateChanged();

	UFUNCTION(BlueprintCallable)
	FConquestPlayerEmpireState& GetHumanPlayerMutable();

	UFUNCTION(BlueprintCallable)
	const FConquestPlayerEmpireState& GetHumanPlayer() const;
	
	UPROPERTY(BlueprintReadWrite, Category="Conquest|Map")
	TObjectPtr<AModularHexGridActor> ActiveGridActor;

	FHexGridModel* GetHexGridModelMutable();
	const FHexGridModel* GetHexGridModel() const;
};
