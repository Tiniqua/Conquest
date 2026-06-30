#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "ConquestMainMenuButton.generated.h"

UENUM(BlueprintType)
enum class EConquestMainMenuButtonAction : uint8
{
	FromButtonName UMETA(DisplayName = "From Button Name"),
	Singleplayer,
	Multiplayer,
	Settings,
	Quit,
	HostGame,
	JoinGame,
	ReturnToMainMenu,
	Custom
};

class UConquestMainMenuButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FConquestMainMenuButtonClickedSignature,
	UConquestMainMenuButton*,
	Button
);

UCLASS(meta = (DisplayName = "Conquest Main Menu Button"))
class CONQUEST_API UConquestMainMenuButton : public UButton
{
	GENERATED_BODY()

public:
	UConquestMainMenuButton(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Main Menu")
	EConquestMainMenuButtonAction MenuAction = EConquestMainMenuButtonAction::FromButtonName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Main Menu")
	FName CustomActionName = NAME_None;

	UPROPERTY(BlueprintAssignable, Category = "Conquest|Main Menu")
	FConquestMainMenuButtonClickedSignature OnConquestMainMenuButtonClicked;

	void ConfigureHoverScale(bool bInScaleOnHover, float InHoverScale, float InDefaultScale);
	void EnsureClickBinding();
	void EnsureHoverBinding();

protected:
	virtual void SynchronizeProperties() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Main Menu|Animation")
	bool bScaleOnHover = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Main Menu|Animation", meta = (EditCondition = "bScaleOnHover", ClampMin = "0.01"))
	float HoverScale = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conquest|Main Menu|Animation", meta = (EditCondition = "bScaleOnHover", ClampMin = "0.01"))
	float DefaultScale = 1.0f;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();
};
