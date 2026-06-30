#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ConquestUserSettingsSaveGame.generated.h"

UCLASS()
class CONQUEST_API UConquestUserSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Settings|Audio")
	bool bMusicEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MusicVolumeMultiplier = 1.0f;
};
