#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Conquest/World/Generation/HexTileTypes.h"
#include "ConquestGameWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UWidget;

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

	UFUNCTION(BlueprintCallable)
	void RefreshTurnInfo();

protected:
	virtual void NativeConstruct() override;

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

private:
	static void SetText(UTextBlock* TextBlock, const FText& Text);
	static void SetText(UTextBlock* TextBlock, const FString& Text);
	static void ClearText(UTextBlock* TextBlock);
	static void SetVisibility(UWidget* Widget, ESlateVisibility Visibility);

	void SetYieldTexts(const FHexYield& Yield);
	void ClearTileTexts();
};
