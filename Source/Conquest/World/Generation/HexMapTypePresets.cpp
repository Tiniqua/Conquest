#include "HexMapTypePresets.h"

TArray<FHexMapTypePreset> FHexMapTypePresets::GetAllPresets()
{
	TArray<FHexMapTypePreset> Presets;
	Presets.Add(MakePangaea());
	Presets.Add(MakeContinents());
	Presets.Add(MakeArchipelago());
	Presets.Add(MakeFractal());
	Presets.Add(MakeArctic());
	Presets.Add(MakeInlandSea());
	Presets.Add(MakeSmallIslands());
	Presets.Add(MakeLakes());
	return Presets;
}

bool FHexMapTypePresets::GetPreset(EHexMapTypePreset PresetType, FHexMapTypePreset& OutPreset)
{
	for (const FHexMapTypePreset& Preset : GetAllPresets())
	{
		if (Preset.PresetType == PresetType)
		{
			OutPreset = Preset;
			return true;
		}
	}
	return false;
}

FHexMapTypePreset FHexMapTypePresets::MakeBase(EHexMapTypePreset Type, const FName Id, const FText& Name, const FText& Description, EHexLandmassStyle LandmassStyle)
{
	FHexMapTypePreset Preset;
	Preset.PresetType = Type;
	Preset.PresetId = Id;
	Preset.DisplayName = Name;
	Preset.Description = Description;
	Preset.LandmassStyle = LandmassStyle;
	return Preset;
}

FHexMapTypePreset FHexMapTypePresets::MakePangaea()
{
	FHexMapTypePreset Preset = MakeBase(EHexMapTypePreset::Pangaea, TEXT("Pangaea"), NSLOCTEXT("HexMapPresets", "PangaeaName", "Pangaea"), NSLOCTEXT("HexMapPresets", "PangaeaDesc", "One dominant landmass with smaller surrounding seas and occasional islands."), EHexLandmassStyle::SingleLargeLandmass);
	Preset.Shape.TargetLandRatio = 0.72f;
	Preset.Shape.MajorLandmassCount = 1;
	Preset.Shape.CoastlineNoise = 0.28f;
	Preset.Shape.LandmassFragmentation = 0.12f;
	Preset.Shape.IslandChainChance = 0.08f;
	Preset.Shape.LakeFrequency = 0.06f;
	Preset.Shape.LakeMinSize = 1;
	Preset.Shape.LakeMaxSize = 4;
	Preset.Shape.LakeSpacing = 4;
	Preset.Shape.OceanBorderWidth = 2;
	Preset.Shape.LandmassSeparation = 0;
	Preset.TerrainWeights = {{EHexTileType::Ocean,4.0f},{EHexTileType::Coast,4.0f},{EHexTileType::Grassland,30.0f},{EHexTileType::Plains,24.0f},{EHexTileType::Desert,9.0f},{EHexTileType::Tundra,13.0f},{EHexTileType::Snow,8.0f},{EHexTileType::Lake,2.0f},{EHexTileType::Mountain,3.5f}};
	return Preset;
}

FHexMapTypePreset FHexMapTypePresets::MakeContinents()
{
	FHexMapTypePreset Preset = MakeBase(EHexMapTypePreset::Continents, TEXT("Continents"), NSLOCTEXT("HexMapPresets", "ContinentsName", "Continents"), NSLOCTEXT("HexMapPresets", "ContinentsDesc", "Several large landmasses separated by oceans. Good default Civ-style map."), EHexLandmassStyle::FewContinents);
	Preset.Shape.TargetLandRatio = 0.58f;
	Preset.Shape.MajorLandmassCount = 5;
	Preset.Shape.CoastlineNoise = 0.34f;
	Preset.Shape.LandmassFragmentation = 0.18f;
	Preset.Shape.IslandChainChance = 0.10f;
	Preset.Shape.LakeFrequency = 0.08f;
	Preset.Shape.LakeMinSize = 1;
	Preset.Shape.LakeMaxSize = 5;
	Preset.Shape.LakeSpacing = 4;
	Preset.Shape.OceanBorderWidth = 2;
	Preset.Shape.LandmassSeparation = 4;
	Preset.TerrainWeights = {{EHexTileType::Ocean,8.0f},{EHexTileType::Coast,5.0f},{EHexTileType::Grassland,28.0f},{EHexTileType::Plains,22.0f},{EHexTileType::Desert,9.0f},{EHexTileType::Tundra,13.0f},{EHexTileType::Snow,8.0f},{EHexTileType::Lake,3.0f},{EHexTileType::Mountain,3.5f}};
	return Preset;
}

FHexMapTypePreset FHexMapTypePresets::MakeArchipelago()
{
	FHexMapTypePreset Preset = MakeBase(EHexMapTypePreset::Archipelago, TEXT("Archipelago"), NSLOCTEXT("HexMapPresets", "ArchipelagoName", "Archipelago"), NSLOCTEXT("HexMapPresets", "ArchipelagoDesc", "Many smaller islands and narrow landmasses with heavy ocean presence."), EHexLandmassStyle::ManyIslands);
	Preset.Shape.TargetLandRatio = 0.38f;
	Preset.Shape.MajorLandmassCount = 8;
	Preset.Shape.CoastlineNoise = 0.62f;
	Preset.Shape.LandmassFragmentation = 0.72f;
	Preset.Shape.IslandChainChance = 0.55f;
	Preset.Shape.LakeFrequency = 0.03f;
	Preset.Shape.LakeMinSize = 1;
	Preset.Shape.LakeMaxSize = 2;
	Preset.Shape.LakeSpacing = 5;
	Preset.Shape.OceanBorderWidth = 2;
	Preset.Shape.LandmassSeparation = 3;
	Preset.TerrainWeights = {{EHexTileType::Ocean,16.0f},{EHexTileType::Coast,10.0f},{EHexTileType::Grassland,22.0f},{EHexTileType::Plains,18.0f},{EHexTileType::Desert,6.0f},{EHexTileType::Tundra,9.0f},{EHexTileType::Snow,5.0f},{EHexTileType::Lake,1.0f},{EHexTileType::Mountain,2.5f}};
	return Preset;
}

FHexMapTypePreset FHexMapTypePresets::MakeFractal()
{
	FHexMapTypePreset Preset = MakeBase(EHexMapTypePreset::Fractal, TEXT("Fractal"), NSLOCTEXT("HexMapPresets", "FractalName", "Fractal"), NSLOCTEXT("HexMapPresets", "FractalDesc", "Unpredictable mixed landmasses, varied coastlines, and inconsistent continent sizes."), EHexLandmassStyle::FractalMixed);
	Preset.Shape.TargetLandRatio = 0.64f;
	Preset.Shape.MajorLandmassCount = 1;
	Preset.Shape.CoastlineNoise = 0.52f;
	Preset.Shape.LandmassFragmentation = 0.28f;
	Preset.Shape.IslandChainChance = 0.14f;
	Preset.Shape.LakeFrequency = 0.08f;
	Preset.Shape.LakeMinSize = 1;
	Preset.Shape.LakeMaxSize = 5;
	Preset.Shape.LakeSpacing = 3;
	Preset.Shape.OceanBorderWidth = 2;
	Preset.Shape.LandmassSeparation = 0;
	Preset.TerrainWeights = {{EHexTileType::Ocean,9.0f},{EHexTileType::Coast,6.0f},{EHexTileType::Grassland,27.0f},{EHexTileType::Plains,22.0f},{EHexTileType::Desert,10.0f},{EHexTileType::Tundra,12.0f},{EHexTileType::Snow,7.0f},{EHexTileType::Lake,4.0f},{EHexTileType::Mountain,4.0f}};
	return Preset;
}

FHexMapTypePreset FHexMapTypePresets::MakeArctic()
{
	FHexMapTypePreset Preset = MakeBase(EHexMapTypePreset::Arctic, TEXT("Arctic"), NSLOCTEXT("HexMapPresets", "ArcticName", "Arctic"), NSLOCTEXT("HexMapPresets", "ArcticDesc", "A colder world with expanded snow/tundra bands and reduced desert presence."), EHexLandmassStyle::ColdWorld);
	Preset.Shape.TargetLandRatio = 0.60f;
	Preset.Shape.MajorLandmassCount = 2;
	Preset.Shape.CoastlineNoise = 0.35f;
	Preset.Shape.LandmassFragmentation = 0.28f;
	Preset.Shape.IslandChainChance = 0.12f;
	Preset.Shape.LakeFrequency = 0.12f;
	Preset.Shape.LakeMinSize = 1;
	Preset.Shape.LakeMaxSize = 5;
	Preset.Shape.LakeSpacing = 4;
	Preset.Shape.OceanBorderWidth = 2;
	Preset.Shape.LandmassSeparation = 5;
	Preset.Shape.TemperatureBiasStrength = 3.0f;
	Preset.Shape.PolarFalloffPower = 0.75f;
	Preset.TerrainWeights = {{EHexTileType::Ocean,8.0f},{EHexTileType::Coast,5.0f},{EHexTileType::Grassland,20.0f},{EHexTileType::Plains,18.0f},{EHexTileType::Desert,3.0f},{EHexTileType::Tundra,22.0f},{EHexTileType::Snow,15.0f},{EHexTileType::Lake,4.0f},{EHexTileType::Mountain,4.0f}};
	return Preset;
}

FHexMapTypePreset FHexMapTypePresets::MakeInlandSea()
{
	FHexMapTypePreset Preset = MakeBase(EHexMapTypePreset::InlandSea, TEXT("InlandSea"), NSLOCTEXT("HexMapPresets", "InlandSeaName", "Inland Sea"), NSLOCTEXT("HexMapPresets", "InlandSeaDesc", "Mostly connected land surrounding a large central body of water."), EHexLandmassStyle::InlandOcean);
	Preset.Shape.TargetLandRatio = 0.76f;
	Preset.Shape.MajorLandmassCount = 1;
	Preset.Shape.CoastlineNoise = 0.34f;
	Preset.Shape.LandmassFragmentation = 0.18f;
	Preset.Shape.IslandChainChance = 0.04f;
	Preset.Shape.LakeFrequency = 0.06f;
	Preset.Shape.LakeMinSize = 1;
	Preset.Shape.LakeMaxSize = 4;
	Preset.Shape.LakeSpacing = 4;
	Preset.Shape.OceanBorderWidth = 1;
	Preset.Shape.LandmassSeparation = 0;
	Preset.Shape.bUseInlandSea = true;
	Preset.Shape.InlandSeaRadiusRatio = 0.26f;
	Preset.TerrainWeights = {{EHexTileType::Ocean,5.0f},{EHexTileType::Coast,7.0f},{EHexTileType::Grassland,29.0f},{EHexTileType::Plains,23.0f},{EHexTileType::Desert,10.0f},{EHexTileType::Tundra,12.0f},{EHexTileType::Snow,7.0f},{EHexTileType::Lake,2.0f},{EHexTileType::Mountain,3.5f}};
	return Preset;
}

FHexMapTypePreset FHexMapTypePresets::MakeSmallIslands()
{
	FHexMapTypePreset Preset = MakeBase(EHexMapTypePreset::SmallIslands, TEXT("SmallIslands"), NSLOCTEXT("HexMapPresets", "SmallIslandsName", "Small Islands"), NSLOCTEXT("HexMapPresets", "SmallIslandsDesc", "Extreme island generation with scattered small land clusters and lots of coast."), EHexLandmassStyle::ManyIslands);
	Preset.Shape.TargetLandRatio = 0.34f;
	Preset.Shape.MajorLandmassCount = 12;
	Preset.Shape.CoastlineNoise = 0.54f;
	Preset.Shape.LandmassFragmentation = 0.42f;
	Preset.Shape.IslandChainChance = 1.0f;
	Preset.Shape.LakeFrequency = 0.01f;
	Preset.Shape.LakeMinSize = 1;
	Preset.Shape.LakeMaxSize = 1;
	Preset.Shape.LakeSpacing = 6;
	Preset.Shape.OceanBorderWidth = 2;
	Preset.Shape.LandmassSeparation = 2;
	Preset.TerrainWeights = {{EHexTileType::Ocean,22.0f},{EHexTileType::Coast,14.0f},{EHexTileType::Grassland,22.0f},{EHexTileType::Plains,17.0f},{EHexTileType::Desert,5.0f},{EHexTileType::Tundra,7.0f},{EHexTileType::Snow,4.0f},{EHexTileType::Lake,0.5f},{EHexTileType::Mountain,1.0f}};
	return Preset;
}

FHexMapTypePreset FHexMapTypePresets::MakeLakes()
{
	FHexMapTypePreset Preset = MakeBase(EHexMapTypePreset::Lakes, TEXT("Lakes"), NSLOCTEXT("HexMapPresets", "LakesName", "Lakes"), NSLOCTEXT("HexMapPresets", "LakesDesc", "Mostly land-based map with frequent inland lakes and less ocean."), EHexLandmassStyle::LakeHeavy);
	Preset.Shape.TargetLandRatio = 0.84f;
	Preset.Shape.MajorLandmassCount = 1;
	Preset.Shape.CoastlineNoise = 0.22f;
	Preset.Shape.LandmassFragmentation = 0.16f;
	Preset.Shape.IslandChainChance = 0.0f;
	Preset.Shape.LakeFrequency = 0.12f;
	Preset.Shape.LakeMinSize = 4;
	Preset.Shape.LakeMaxSize = 11;
	Preset.Shape.LakeSpacing = 5;
	Preset.Shape.OceanBorderWidth = 1;
	Preset.Shape.LandmassSeparation = 0;
	Preset.TerrainWeights = {{EHexTileType::Ocean,3.0f},{EHexTileType::Coast,3.0f},{EHexTileType::Grassland,31.0f},{EHexTileType::Plains,24.0f},{EHexTileType::Desert,9.0f},{EHexTileType::Tundra,13.0f},{EHexTileType::Snow,8.0f},{EHexTileType::Lake,10.0f},{EHexTileType::Mountain,3.5f}};
	return Preset;
}
