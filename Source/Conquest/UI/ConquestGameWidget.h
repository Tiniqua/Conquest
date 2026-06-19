#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Conquest/UI/ConquestChoiceTypes.h"
#include "Conquest/World/Generation/HexTileTypes.h"
#include "ConquestGameWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UWidget;
class UConquestChoiceButtonWidget;
class UConquestUnitActionButtonWidget;

USTRUCT(BlueprintType)
struct FConquestSelectedUnitWidgetData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Unit")
	int32 UnitInstanceId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Unit")
	FText UnitName;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Unit")
	FText HealthText;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Unit")
	bool bCanFoundCity = false;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Unit")
	bool bIsValid = false;
};

USTRUCT(BlueprintType)
struct FConquestTileImprovementChoiceData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Improvement")
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Improvement")
	bool bIsValid = false;
};

USTRUCT(BlueprintType)
struct FConquestTileExpansionChoiceData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	int32 CityId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	FString TileType;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	FString Resource;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	FString Features;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	FHexYield Yield;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	bool bAssigningToOwnedTile = false;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	int32 CurrentAssignedCitizens = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	int32 ResultAssignedCitizens = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Tile Expansion")
	bool bIsValid = false;
};

USTRUCT(BlueprintType)
struct FConquestTopBarYieldData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Yields")
	FHexYield EmpireStoredYields;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Yields")
	FHexYield EmpireYieldPerTurn;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Yields")
	FHexYield SelectedCityYieldPerTurn;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Yields")
	int32 SelectedCityId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Yields")
	bool bShowSelectedCityLocalYields = false;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Yields")
	int32 Happiness = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Conquest|Yields")
	bool bIsUnhappy = false;
};

USTRUCT(BlueprintType)
struct FHoveredHexTileWidgetData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	int32 Q = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	int32 R = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	FString TileType;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	FString Features;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	FString Resource;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	FString Improvement;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	FString FreshWater;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	FString River;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	float Height = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Hovered Tile")
	FHexYield Yield;
};

UCLASS()
class CONQUEST_API UConquestGameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Hover")
	void UpdateHoveredTileInfo(const FHoveredHexTileWidgetData& HoveredTileData);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Hover")
	void ClearHoveredTileInfo();

	UFUNCTION()
	void HandleEndTurnClicked();

	UFUNCTION()
	void HandleResearchClicked();

	UFUNCTION()
	void HandleTurnChanged(int32 NewTurn);

	UFUNCTION()
	void HandleConquestStateChanged();

	UFUNCTION(BlueprintCallable)
	void RefreshTurnInfo();

	UFUNCTION(BlueprintPure, Category = "Conquest|Turn")
	FText GetEndTurnButtonText() const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Yields")
	void RefreshTopBarYieldInfo();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Research")
	void RefreshResearchInfo();

	UFUNCTION(BlueprintPure, Category = "Conquest|Research")
	FText GetCurrentResearchStatusText() const;

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void ShowSelectedUnitInfo(const FConquestSelectedUnitWidgetData& UnitData);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Unit")
	void ClearSelectedUnitInfo();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Yields")
	void SetSelectedCityYieldContext(int32 CityId);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Yields")
	void ClearSelectedCityYieldContext();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Expansion")
	void ShowTileExpansionConfirmation(const FConquestTileExpansionChoiceData& ChoiceData);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Expansion")
	void ClearTileExpansionConfirmation();

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Improvement")
	void ShowTileImprovementChoices(const FConquestTileImprovementChoiceData& ChoiceData, const TArray<FConquestChoiceButtonData>& ImprovementChoices);

	UFUNCTION(BlueprintCallable, Category = "Conquest|Tile Improvement")
	void ClearTileImprovementChoices();

	UFUNCTION(BlueprintPure, Category = "Conquest|Yields")
	FConquestTopBarYieldData GetTopBarYieldData() const;

	UFUNCTION(BlueprintPure, Category = "Conquest|Tile Expansion")
	FConquestTileExpansionChoiceData GetPendingTileExpansionChoice() const { return PendingTileExpansionChoice; }

	UFUNCTION(BlueprintPure, Category = "Conquest|Tile Improvement")
	FConquestTileImprovementChoiceData GetPendingTileImprovementChoice() const { return PendingTileImprovementChoice; }

	UFUNCTION(BlueprintPure, Category = "Conquest|Yields")
	bool ShouldShowSelectedCityLocalYields() const { return SelectedCityYieldContextId != INDEX_NONE; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> HoveredTileInfoPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileCoordText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileTypeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileFeaturesText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileResourceText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileImprovementText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileFreshWaterText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileRiverText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileHeightText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> TileDetailsBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TileYieldsBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileFoodText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileProductionText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileGoldText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileScienceText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileCultureText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileFaithText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EndTurnButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResearchButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TurnText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TopBarYieldPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TopBarLocalYieldPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TopBarFoodText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TopBarProductionText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TopBarGoldText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TopBarScienceText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TopBarCultureText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TopBarFaithText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TopBarHappinessText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentResearchText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TileExpansionConfirmPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileExpansionTitleText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileExpansionDetailText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileExpansionYieldText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TileExpansionConfirmButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TileExpansionCancelButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TileImprovementChoicePanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TileImprovementTitleText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> TileImprovementButtonBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TileImprovementCloseButton = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestChoiceButtonWidget> ChoiceButtonWidgetClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> UnitActionPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedUnitText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> UnitActionButtonBox = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Conquest|UI")
	TSubclassOf<UConquestUnitActionButtonWidget> UnitActionButtonWidgetClass;

private:
	static void SetText(UTextBlock* TextBlock, const FText& Text);
	static void SetText(UTextBlock* TextBlock, const FString& Text);
	static void ClearText(UTextBlock* TextBlock);
	static void SetVisibility(UWidget* Widget, ESlateVisibility Visibility);

	void SetYieldTexts(const FHexYield& Yield);
	void SetTopBarYieldTexts(const FConquestTopBarYieldData& YieldData);
	static FText FormatStoredYieldText(const FText& Label, int32 Stored, int32 PerTurn);
	static FText FormatPerTurnYieldText(const FText& Label, int32 PerTurn);
	void ClearTileTexts();
	void RefreshSelectedUnitInfoFromGameState();

	int32 SelectedCityYieldContextId = INDEX_NONE;
	FConquestTileExpansionChoiceData PendingTileExpansionChoice;
	FConquestTileImprovementChoiceData PendingTileImprovementChoice;

	UFUNCTION()
	void HandleTileExpansionConfirmClicked();

	UFUNCTION()
	void HandleTileExpansionCancelClicked();

	UFUNCTION()
	void HandleTileImprovementChoiceClicked(const FConquestChoiceButtonData& ChoiceData);

	UFUNCTION()
	void HandleTileImprovementCloseClicked();

	UFUNCTION()
	void HandleUnitActionClicked(FName ActionId);

	UFUNCTION()
	void HandleResearchChanged();
};
