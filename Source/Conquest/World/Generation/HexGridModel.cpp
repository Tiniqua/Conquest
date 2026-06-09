#include "HexGridModel.h"

void FHexGridModel::Initialize(const FHexGridSizeSettings& InSize, const FHexHeightSettings& InHeight, const UHexTileResourceData* InTileData)
{
	Size = InSize;
	Height = InHeight;
	TileData = InTileData;

	Tiles.Reset();
	Tiles.SetNum(Size.GridWidth * Size.GridHeight);
	ResolvedVertexHeights.Reset();
}

void FHexGridModel::ResolveTileHeights(const TArray<FHexTileGenerationRule>& GenerationRules)
{
	for (FHexTileData& Tile : Tiles)
	{
		if (!Height.bUseHeightOffsets)
		{
			Tile.Height = 0.0f;
			continue;
		}

		Tile.Height = GetHeightOffsetForTileType(Tile.TileType, GenerationRules) * Height.HeightScale;
	}
}

void FHexGridModel::ResolveSharedVertexHeights()
{
	ResolvedVertexHeights.Reset();

	TMap<FHexVertexKey, float> HeightSums;
	TMap<FHexVertexKey, int32> HeightCounts;

	for (int32 R = 0; R < Size.GridHeight; ++R)
	{
		for (int32 Q = 0; Q < Size.GridWidth; ++Q)
		{
			if (!IsValidTile(Q, R))
			{
				continue;
			}

			const FHexTileData& Tile = Tiles[GetTileIndex(Q, R)];
			const FVector FlatCenter = GetHexCenter(Q, R);

			for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
			{
				const FVector FlatCorner = FlatCenter + GetHexCornerOffset(CornerIndex);
				const FHexVertexKey VertexKey = MakeVertexKey(FlatCorner);

				HeightSums.FindOrAdd(VertexKey) += Tile.Height;
				HeightCounts.FindOrAdd(VertexKey) += 1;
			}
		}
	}

	for (const TPair<FHexVertexKey, float>& Pair : HeightSums)
	{
		const int32 HeightCount = HeightCounts.FindRef(Pair.Key);
		ResolvedVertexHeights.Add(Pair.Key, HeightCount > 0 ? Pair.Value / static_cast<float>(HeightCount) : 0.0f);
	}
}

void FHexGridModel::ResolveTileYields()
{
	const UHexTileResourceData* Data = TileData.Get();
	if (!Data)
	{
		return;
	}

	for (FHexTileData& Tile : Tiles)
	{
		FHexYield TotalYield;

		if (const FHexTileDefinition* TileDefinition = Data->FindTileDefinition(Tile.TileType))
		{
			TotalYield += TileDefinition->BaseYield;
		}

		for (const EHexFeatureType FeatureType : Tile.Features)
		{
			if (const FHexFeatureDefinition* FeatureDefinition = Data->FindFeatureDefinition(FeatureType))
			{
				TotalYield += FeatureDefinition->YieldModifier;
			}
		}

		if (const FHexResourceDefinition* ResourceDefinition = Data->FindResourceDefinition(Tile.ResourceType))
		{
			TotalYield += ResourceDefinition->YieldModifier;
		}

		Tile.Yield = TotalYield;
	}
}

FVector FHexGridModel::GetHexCenter(int32 Q, int32 R) const
{
	const float HexWidth = FMath::Sqrt(3.0f) * Size.HexRadius;
	const float VerticalSpacing = 1.5f * Size.HexRadius;
	const float X = HexWidth * (static_cast<float>(Q) + 0.5f * static_cast<float>(R & 1));
	const float Y = VerticalSpacing * static_cast<float>(R);
	return FVector(X, Y, 0.0f);
}

FVector FHexGridModel::GetHexCornerOffset(int32 CornerIndex) const
{
	const float AngleDeg = 60.0f * static_cast<float>(CornerIndex) + 30.0f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);
	return FVector(Size.HexRadius * FMath::Cos(AngleRad), Size.HexRadius * FMath::Sin(AngleRad), 0.0f);
}

FVector2D FHexGridModel::GetHexCornerUV(int32 CornerIndex) const
{
	const float AngleDeg = 60.0f * static_cast<float>(CornerIndex) + 30.0f;
	const float AngleRad = FMath::DegreesToRadians(AngleDeg);
	return FVector2D(0.5f + 0.5f * FMath::Cos(AngleRad), 0.5f + 0.5f * FMath::Sin(AngleRad));
}

FHexVertexKey FHexGridModel::MakeVertexKey(const FVector& FlatPosition) const
{
	const float SafePrecision = FMath::Max(1.0f, Height.VertexKeyPrecision);
	FHexVertexKey Key;
	Key.X = FMath::RoundToInt(FlatPosition.X * SafePrecision);
	Key.Y = FMath::RoundToInt(FlatPosition.Y * SafePrecision);
	return Key;
}

float FHexGridModel::GetResolvedCornerHeight(const FVector& FlatCornerPosition) const
{
	if (const float* ResolvedHeight = ResolvedVertexHeights.Find(MakeVertexKey(FlatCornerPosition)))
	{
		return *ResolvedHeight;
	}
	return 0.0f;
}

float FHexGridModel::GetHeightOffsetForTileType(EHexTileType TileType, const TArray<FHexTileGenerationRule>& GenerationRules) const
{
	if (const UHexTileResourceData* Data = TileData.Get())
	{
		if (const FHexTileDefinition* TileDefinition = Data->FindTileDefinition(TileType))
		{
			return TileDefinition->HeightOffset;
		}
	}

	for (const FHexTileGenerationRule& Rule : GenerationRules)
	{
		if (Rule.TileType == TileType)
		{
			return Rule.HeightOffset;
		}
	}

	return 0.0f;
}

bool FHexGridModel::GetNeighbourCoord(int32 Q, int32 R, int32 Direction, int32& OutQ, int32& OutR) const
{
	const bool bOddRow = (R & 1) != 0;

	switch (Direction)
	{
	case 0: OutQ = bOddRow ? Q + 1 : Q;     OutR = R - 1; break;
	case 1: OutQ = bOddRow ? Q : Q - 1;     OutR = R - 1; break;
	case 2: OutQ = Q - 1;                   OutR = R;     break;
	case 3: OutQ = bOddRow ? Q : Q - 1;     OutR = R + 1; break;
	case 4: OutQ = bOddRow ? Q + 1 : Q;     OutR = R + 1; break;
	case 5: OutQ = Q + 1;                   OutR = R;     break;
	default: return false;
	}

	return IsValidCoord(OutQ, OutR);
}

int32 FHexGridModel::GetTileIndex(int32 Q, int32 R) const
{
	return R * Size.GridWidth + Q;
}

bool FHexGridModel::IsValidCoord(int32 Q, int32 R) const
{
	return Q >= 0 && Q < Size.GridWidth && R >= 0 && R < Size.GridHeight;
}

bool FHexGridModel::IsValidTile(int32 Q, int32 R) const
{
	return IsValidCoord(Q, R) && Tiles.IsValidIndex(GetTileIndex(Q, R));
}

bool FHexGridModel::IsWaterTileType(EHexTileType TileType) const
{
	if (const UHexTileResourceData* Data = TileData.Get())
	{
		if (const FHexTileDefinition* TileDefinition = Data->FindTileDefinition(TileType))
		{
			return TileDefinition->bIsWater;
		}
	}

	return TileType == EHexTileType::Ocean || TileType == EHexTileType::Coast || TileType == EHexTileType::Lake;
}
