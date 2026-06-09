#include "HexResourceSetData.h"
#include "UObject/UnrealType.h"

void UHexResourceSetData::PostLoad()
{
	Super::PostLoad();
	NormalizeResourceDefinitions();
}

#if WITH_EDITOR
void UHexResourceSetData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	NormalizeResourceDefinitions();
}
#endif

void UHexResourceSetData::NormalizeResourceDefinitions()
{
	for (FHexResourceDefinition& Resource : BonusResources)
	{
		Resource.Category = EHexResourceCategory::Bonus;
		Resource.Happiness = 0;
		Resource.MinStrategicQuantity = 0;
		Resource.MaxStrategicQuantity = 0;
	}

	for (FHexResourceDefinition& Resource : LuxuryResources)
	{
		Resource.Category = EHexResourceCategory::Luxury;
		if (Resource.Happiness <= 0)
		{
			Resource.Happiness = 4;
		}
		Resource.MinStrategicQuantity = 0;
		Resource.MaxStrategicQuantity = 0;
	}

	for (FHexResourceDefinition& Resource : StrategicResources)
	{
		Resource.Category = EHexResourceCategory::Strategic;
		Resource.Happiness = 0;
		Resource.MinStrategicQuantity = FMath::Max(0, Resource.MinStrategicQuantity);
		Resource.MaxStrategicQuantity = FMath::Max(Resource.MinStrategicQuantity, Resource.MaxStrategicQuantity);
	}
}


const FHexResourceDefinition* UHexResourceSetData::FindResource(FName ResourceId) const
{
	if (ResourceId.IsNone())
	{
		return nullptr;
	}

	for (const FHexResourceDefinition& Resource : BonusResources)
	{
		if (Resource.ResourceId == ResourceId)
		{
			return &Resource;
		}
	}

	for (const FHexResourceDefinition& Resource : LuxuryResources)
	{
		if (Resource.ResourceId == ResourceId)
		{
			return &Resource;
		}
	}

	for (const FHexResourceDefinition& Resource : StrategicResources)
	{
		if (Resource.ResourceId == ResourceId)
		{
			return &Resource;
		}
	}

	return nullptr;
}

void UHexResourceSetData::GetResourcesForCategory(EHexResourceCategory Category, TArray<const FHexResourceDefinition*>& OutResources) const
{
	OutResources.Reset();

	const TArray<FHexResourceDefinition>* SourceArray = nullptr;
	switch (Category)
	{
	case EHexResourceCategory::Bonus:
		SourceArray = &BonusResources;
		break;
	case EHexResourceCategory::Luxury:
		SourceArray = &LuxuryResources;
		break;
	case EHexResourceCategory::Strategic:
		SourceArray = &StrategicResources;
		break;
	default:
		break;
	}

	if (!SourceArray)
	{
		return;
	}

	for (const FHexResourceDefinition& Resource : *SourceArray)
	{
		if (!Resource.ResourceId.IsNone() && Resource.PlacementWeight > 0.0f)
		{
			OutResources.Add(&Resource);
		}
	}
}

void UHexResourceSetData::GetAllResources(TArray<const FHexResourceDefinition*>& OutResources) const
{
	OutResources.Reset();

	for (const FHexResourceDefinition& Resource : BonusResources)
	{
		if (!Resource.ResourceId.IsNone())
		{
			OutResources.Add(&Resource);
		}
	}

	for (const FHexResourceDefinition& Resource : LuxuryResources)
	{
		if (!Resource.ResourceId.IsNone())
		{
			OutResources.Add(&Resource);
		}
	}

	for (const FHexResourceDefinition& Resource : StrategicResources)
	{
		if (!Resource.ResourceId.IsNone())
		{
			OutResources.Add(&Resource);
		}
	}
}
