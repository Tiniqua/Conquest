// ConquestGameSetupTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Conquest/Civilisations/ConquestCivilisationTypes.h"
#include "Conquest/Core/ConquestGameplayTypes.h"
#include "Conquest/World/Generation/HexGridSettings.h"
#include "ConquestGameSetupTypes.generated.h"

UENUM(BlueprintType)
enum class EConquestMapSizePreset : uint8
{
	Tiny     UMETA(DisplayName = "Tiny"),
	Small    UMETA(DisplayName = "Small"),
	Standard UMETA(DisplayName = "Standard"),
	Large    UMETA(DisplayName = "Large"),
	Huge     UMETA(DisplayName = "Huge"),
	Stupid     UMETA(DisplayName = "Stupid")
};

USTRUCT(BlueprintType)
struct FConquestMapSizePresetDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConquestMapSizePreset Preset = EConquestMapSizePreset::Standard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Width = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Height = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PlayerCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Tooltip;
};

UENUM(BlueprintType)
enum class EConquestLobbySlotType : uint8
{
	Human UMETA(DisplayName = "Human"),
	AI UMETA(DisplayName = "AI"),
	Closed UMETA(DisplayName = "Closed")
};

USTRUCT(BlueprintType)
struct FConquestLobbyPlayerSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	EConquestLobbySlotType SlotType = EConquestLobbySlotType::AI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	bool bIsHost = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	bool bIsReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	TObjectPtr<UConquestCivilisationData> Civilisation = nullptr;
};

USTRUCT(BlueprintType)
struct FConquestGameSetupSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer")
	EConquestTurnMode TurnMode = EConquestTurnMode::Simultaneous;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	EHexMapTypePreset MapTypePreset = EHexMapTypePreset::Continents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	EConquestMapSizePreset MapSizePreset = EConquestMapSizePreset::Standard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FHexGridSizeSettings SizeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FHexTemperatureSettings TemperatureSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FHexMapShapeSettings MapShapeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bUseCustomMapShapeSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0"))
	float MountainWeightScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	FHexResourceGenerationSettings ResourceGenerationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rivers")
	FHexSimpleRiverSettings RiverSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 RandomSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	TArray<FConquestLobbyPlayerSlot> PlayerSlots;
};

class FConquestMapSizePresets
{
public:
	static TArray<FConquestMapSizePresetDefinition> GetAllPresets()
	{
		return {
			Make(EConquestMapSizePreset::Tiny,     TEXT("Tiny"),      50,  50,  2),
			Make(EConquestMapSizePreset::Small,    TEXT("Small"),     75,  75,  4),
			Make(EConquestMapSizePreset::Standard, TEXT("Standard"), 100, 100,  6),
			Make(EConquestMapSizePreset::Large,    TEXT("Large"),    150, 150,  8),
			Make(EConquestMapSizePreset::Huge,     TEXT("Huge"),     200, 200, 10),
			Make(EConquestMapSizePreset::Stupid,   TEXT("Stupid"),   300, 300, 12)
		};
	}

	static bool GetPreset(EConquestMapSizePreset Preset, FConquestMapSizePresetDefinition& OutPreset)
	{
		for (const FConquestMapSizePresetDefinition& Definition : GetAllPresets())
		{
			if (Definition.Preset == Preset)
			{
				OutPreset = Definition;
				return true;
			}
		}

		return false;
	}

private:
	static FConquestMapSizePresetDefinition Make(
		EConquestMapSizePreset Preset,
		const TCHAR* Name,
		int32 Width,
		int32 Height,
		int32 PlayerCount)
	{
		FConquestMapSizePresetDefinition Result;
		Result.Preset = Preset;
		Result.DisplayName = FText::FromString(Name);
		Result.Width = Width;
		Result.Height = Height;
		Result.PlayerCount = PlayerCount;
		Result.Tooltip = FText::FromString(
			FString::Printf(TEXT("%s map: %dx%d tiles, %d players"), Name, Width, Height, PlayerCount)
		);
		return Result;
	}
};
