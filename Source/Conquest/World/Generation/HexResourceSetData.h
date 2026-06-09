#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HexResourceTypes.h"
#include "HexResourceSetData.generated.h"

struct FPropertyChangedEvent;

UCLASS(BlueprintType)
class CONQUEST_API UHexResourceSetData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources|Bonus")
	TArray<FHexResourceDefinition> BonusResources;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources|Luxury")
	TArray<FHexResourceDefinition> LuxuryResources;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources|Strategic")
	TArray<FHexResourceDefinition> StrategicResources;

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	const FHexResourceDefinition* FindResource(FName ResourceId) const;
	void GetResourcesForCategory(EHexResourceCategory Category, TArray<const FHexResourceDefinition*>& OutResources) const;
	void GetAllResources(TArray<const FHexResourceDefinition*>& OutResources) const;

private:
	void NormalizeResourceDefinitions();
};
