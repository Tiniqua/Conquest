// ConquestGameSetupTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Conquest/World/Generation/HexGridSettings.h"
#include "ConquestGameSetupTypes.generated.h"

UENUM(BlueprintType)
enum class EConquestMapSizePreset : uint8
{
	Tiny     UMETA(DisplayName = "Tiny"),
	Small    UMETA(DisplayName = "Small"),
	Standard UMETA(DisplayName = "Standard"),
	Large    UMETA(DisplayName = "Large"),
	Huge     UMETA(DisplayName = "Huge")
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
	FText Tooltip;
};

USTRUCT(BlueprintType)
struct FConquestGameSetupSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	EHexMapTypePreset MapTypePreset = EHexMapTypePreset::Continents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	EConquestMapSizePreset MapSizePreset = EConquestMapSizePreset::Standard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FHexGridSizeSettings SizeSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FHexTemperatureSettings TemperatureSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	FHexResourceGenerationSettings ResourceGenerationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rivers")
	FHexRiverGenerationSettings RiverGenerationSettings;
};

class FConquestMapSizePresets
{
public:
	static TArray<FConquestMapSizePresetDefinition> GetAllPresets()
	{
		return {
			Make(EConquestMapSizePreset::Tiny,     TEXT("Tiny"),     50,  50),
			Make(EConquestMapSizePreset::Small,    TEXT("Small"),    75,  75),
			Make(EConquestMapSizePreset::Standard, TEXT("Standard"), 100, 100),
			Make(EConquestMapSizePreset::Large,    TEXT("Large"),    150, 150),
			Make(EConquestMapSizePreset::Huge,     TEXT("Huge"),     200, 200)
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
		int32 Height)
	{
		FConquestMapSizePresetDefinition Result;
		Result.Preset = Preset;
		Result.DisplayName = FText::FromString(Name);
		Result.Width = Width;
		Result.Height = Height;
		Result.Tooltip = FText::FromString(
			FString::Printf(TEXT("%s map: %dx%d tiles"), Name, Width, Height)
		);
		return Result;
	}
};