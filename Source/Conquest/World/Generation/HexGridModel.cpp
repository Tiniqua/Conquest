#include "HexGridModel.h"

void FHexGridModel::Initialize(
	const FHexGridSizeSettings& InSize,
	const FHexHeightSettings& InHeight,
	const UHexTileResourceData* InTileData,
	const UHexResourceSetData* InResourceSetData,
	const UHexImprovementSetData* InImprovementSetData
)
{
	Size = InSize;
	Height = InHeight;
	TileData = InTileData;
	ResourceSetData = InResourceSetData;
	ImprovementSetData = InImprovementSetData;

	Tiles.Reset();
	Tiles.SetNum(Size.GridWidth * Size.GridHeight);
	ResolvedVertexHeights.Reset();
}

float FHexGridModel::GetHeightVariancePercentForTileType(
	EHexTileType TileType,
	const TArray<FHexTileGenerationRule>& GenerationRules
) const
{
	for (const FHexTileGenerationRule& Rule : GenerationRules)
	{
		if (Rule.TileType == TileType)
		{
			return FMath::Max(0.0f, Rule.RingVertexHeightVariancePercent);
		}
	}

	return 0.0f;
}

void FHexGridModel::ResolveSharedVertexHeightVariance(
	const TArray<FHexTileGenerationRule>& GenerationRules,
	int32 RandomSeed
)
{
	ResolvedVertexHeightVarianceOffsets.Reset();

	for (int32 R = 0; R < Size.GridHeight; ++R)
	{
		for (int32 Q = 0; Q < Size.GridWidth; ++Q)
		{
			const int32 TileIndex = GetTileIndex(Q, R);
			if (!Tiles.IsValidIndex(TileIndex))
			{
				continue;
			}

			const FHexTileData& Tile = Tiles[TileIndex];
			const FVector FlatCenter = GetHexCenter(Q, R);

			for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
			{
				const FVector FlatCorner = FlatCenter + GetHexCornerOffset(CornerIndex);
				const FHexVertexKey VertexKey = MakeVertexKey(FlatCorner);

				// Important: if this shared vertex was already assigned by another tile,
				// keep that value. This prevents neighbouring tiles from creating gaps.
				if (ResolvedVertexHeightVarianceOffsets.Contains(VertexKey))
				{
					continue;
				}

				const float VariancePercent = GetHeightVariancePercentForTileType(
					Tile.TileType,
					GenerationRules
				);

				if (VariancePercent <= 0.0f)
				{
					ResolvedVertexHeightVarianceOffsets.Add(VertexKey, 0.0f);
					continue;
				}

				const float MaxVariance = FMath::Abs(Tile.Height) * VariancePercent;

				if (MaxVariance <= KINDA_SMALL_NUMBER)
				{
					ResolvedVertexHeightVarianceOffsets.Add(VertexKey, 0.0f);
					continue;
				}

				const int32 VertexSeed =
					RandomSeed ^
					(VertexKey.X * 73856093) ^
					(VertexKey.Y * 19349663);

				FRandomStream VertexRandom(VertexSeed);

				const float Offset = VertexRandom.FRandRange(
					-MaxVariance,
					MaxVariance
				);

				ResolvedVertexHeightVarianceOffsets.Add(VertexKey, Offset);
			}
		}
	}
}

float FHexGridModel::GetResolvedCornerHeightVarianceOffset(
	const FVector& FlatCornerPosition
) const
{
	const FHexVertexKey VertexKey = MakeVertexKey(FlatCornerPosition);

	if (const float* Offset = ResolvedVertexHeightVarianceOffsets.Find(VertexKey))
	{
		return *Offset;
	}

	return 0.0f;
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

FHexTileData* FHexGridModel::GetTileMutable(int32 Q, int32 R)
{
	if (!IsValidTile(Q, R))
	{
		return nullptr;
	}

	return &Tiles[GetTileIndex(Q, R)];
}

FHexTileData* FHexGridModel::GetTileMutable(const FIntPoint& Coord)
{
	return GetTileMutable(Coord.X, Coord.Y);
}

const FHexTileData* FHexGridModel::GetTile(int32 Q, int32 R) const
{
	if (!IsValidTile(Q, R))
	{
		return nullptr;
	}

	return &Tiles[GetTileIndex(Q, R)];
}

const FHexTileData* FHexGridModel::GetTile(const FIntPoint& Coord) const
{
	return GetTile(Coord.X, Coord.Y);
}

void FHexGridModel::ResolveTileYields()
{
	const UHexTileResourceData* TerrainData = TileData.Get();
	const UHexResourceSetData* ResourceData = ResourceSetData.Get();
	const UHexImprovementSetData* ImprovementData = ImprovementSetData.Get();

	for (FHexTileData& Tile : Tiles)
	{
		FHexYield TotalYield;

		if (TerrainData)
		{
			if (const FHexTileDefinition* TileDefinition = TerrainData->FindTileDefinition(Tile.TileType))
			{
				TotalYield += TileDefinition->BaseYield;
			}

			for (const EHexFeatureType FeatureType : Tile.Features)
			{
				if (const FHexFeatureDefinition* FeatureDefinition = TerrainData->FindFeatureDefinition(FeatureType))
				{
					TotalYield += FeatureDefinition->YieldModifier;
				}
			}
		}

		if (ResourceData)
		{
			if (const FHexResourceDefinition* ResourceDefinition = ResourceData->FindResource(Tile.Resource.ResourceId))
			{
				TotalYield += ResourceDefinition->YieldModifier;
			}
		}

		if (ImprovementData)
		{
			if (const FHexImprovementDefinition* ImprovementDefinition = ImprovementData->FindImprovement(Tile.ImprovementId))
			{
				TotalYield += ImprovementDefinition->YieldModifier;
			}
		}

		Tile.FinalYield = TotalYield;
	}
}

bool FHexGridModel::SetTileImprovement(int32 Q, int32 R, FName ImprovementId)
{
	if (!IsValidTile(Q, R))
	{
		return false;
	}

	FHexTileData& Tile = Tiles[GetTileIndex(Q, R)];
	if (ImprovementId.IsNone())
	{
		Tile.ImprovementId = NAME_None;
		ResolveTileYields();
		return true;
	}

	const UHexImprovementSetData* ImprovementData = ImprovementSetData.Get();
	if (!ImprovementData)
	{
		return false;
	}

	const FHexImprovementDefinition* Improvement = ImprovementData->FindImprovement(ImprovementId);
	if (!Improvement || !Improvement->IsValidForTile(Tile, IsWaterTileType(Tile.TileType)))
	{
		return false;
	}

	if (Improvement->bRemovesFeature)
	{
		for (const EHexFeatureType RemovedFeature : Improvement->RemovedFeatures)
		{
			Tile.Features.Remove(RemovedFeature);
		}
	}

	Tile.ImprovementId = ImprovementId;
	ResolveTileYields();
	return true;
}

const FHexResourceDefinition* FHexGridModel::FindResourceDefinition(FName ResourceId) const
{
	const UHexResourceSetData* ResourceData = ResourceSetData.Get();
	return ResourceData ? ResourceData->FindResource(ResourceId) : nullptr;
}

const FHexImprovementDefinition* FHexGridModel::FindImprovementDefinition(FName ImprovementId) const
{
	const UHexImprovementSetData* ImprovementData = ImprovementSetData.Get();
	return ImprovementData ? ImprovementData->FindImprovement(ImprovementId) : nullptr;
}

void FHexGridModel::GetPossibleImprovementsForTile(int32 Q, int32 R, TArray<const FHexImprovementDefinition*>& OutImprovements) const
{
	OutImprovements.Reset();
	if (!IsValidTile(Q, R))
	{
		return;
	}

	const UHexImprovementSetData* ImprovementData = ImprovementSetData.Get();
	if (!ImprovementData)
	{
		return;
	}

	const FHexTileData& Tile = Tiles[GetTileIndex(Q, R)];
	ImprovementData->GetPossibleImprovementsForTile(Tile, IsWaterTileType(Tile.TileType), OutImprovements);
}

void FHexGridModel::GetPossibleImprovementIdsForTile(int32 Q, int32 R, TArray<FName>& OutImprovementIds) const
{
	OutImprovementIds.Reset();
	TArray<const FHexImprovementDefinition*> PossibleImprovements;
	GetPossibleImprovementsForTile(Q, R, PossibleImprovements);
	for (const FHexImprovementDefinition* Improvement : PossibleImprovements)
	{
		if (Improvement)
		{
			OutImprovementIds.Add(Improvement->ImprovementId);
		}
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

TArray<FIntPoint> FHexGridModel::GetCoordsInRange(const FIntPoint& Center, int32 Range) const
{
	TArray<FIntPoint> Result;

	if (Range < 0)
	{
		return Result;
	}

	const int32 MinQ = FMath::Max(0, Center.X - Range);
	const int32 MaxQ = FMath::Min(Size.GridWidth - 1, Center.X + Range);
	const int32 MinR = FMath::Max(0, Center.Y - Range);
	const int32 MaxR = FMath::Min(Size.GridHeight - 1, Center.Y + Range);

	for (int32 R = MinR; R <= MaxR; ++R)
	{
		for (int32 Q = MinQ; Q <= MaxQ; ++Q)
		{
			const FIntPoint Coord(Q, R);

			if (IsValidTile(Coord.X, Coord.Y) && GetHexDistance(Center, Coord) <= Range)
			{
				Result.Add(Coord);
			}
		}
	}

	return Result;
}

int32 FHexGridModel::GetHexDistance(const FIntPoint& A, const FIntPoint& B) const
{
	auto OffsetToCube = [](const FIntPoint& Coord, int32& OutX, int32& OutY, int32& OutZ)
	{
		OutX = Coord.X - ((Coord.Y - (Coord.Y & 1)) / 2);
		OutZ = Coord.Y;
		OutY = -OutX - OutZ;
	};

	int32 AX = 0;
	int32 AY = 0;
	int32 AZ = 0;
	int32 BX = 0;
	int32 BY = 0;
	int32 BZ = 0;

	OffsetToCube(A, AX, AY, AZ);
	OffsetToCube(B, BX, BY, BZ);

	return FMath::Max3(
		FMath::Abs(AX - BX),
		FMath::Abs(AY - BY),
		FMath::Abs(AZ - BZ)
	);
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

bool FHexGridModel::GetCoordFromIndex(int32 TileIndex, int32& OutQ, int32& OutR) const
{
	if (!Tiles.IsValidIndex(TileIndex) || Size.GridWidth <= 0)
	{
		return false;
	}

	OutQ = TileIndex % Size.GridWidth;
	OutR = TileIndex / Size.GridWidth;

	return IsValidCoord(OutQ, OutR);
}
