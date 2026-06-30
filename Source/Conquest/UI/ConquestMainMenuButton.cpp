#include "Conquest/UI/ConquestMainMenuButton.h"

UConquestMainMenuButton::UConquestMainMenuButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UConquestMainMenuButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	EnsureClickBinding();
	EnsureHoverBinding();
	SetRenderScale(FVector2D(DefaultScale, DefaultScale));
}

void UConquestMainMenuButton::HandleClicked()
{
	OnConquestMainMenuButtonClicked.Broadcast(this);
}

void UConquestMainMenuButton::ConfigureHoverScale(bool bInScaleOnHover, float InHoverScale, float InDefaultScale)
{
	bScaleOnHover = bInScaleOnHover;
	HoverScale = InHoverScale;
	DefaultScale = InDefaultScale;

	SetRenderScale(FVector2D(DefaultScale, DefaultScale));
}

void UConquestMainMenuButton::HandleHovered()
{
	if (bScaleOnHover)
	{
		SetRenderScale(FVector2D(HoverScale, HoverScale));
	}
}

void UConquestMainMenuButton::HandleUnhovered()
{
	if (bScaleOnHover)
	{
		SetRenderScale(FVector2D(DefaultScale, DefaultScale));
	}
}

void UConquestMainMenuButton::EnsureClickBinding()
{
	OnClicked.RemoveDynamic(this, &UConquestMainMenuButton::HandleClicked);
	OnClicked.AddDynamic(this, &UConquestMainMenuButton::HandleClicked);
}

void UConquestMainMenuButton::EnsureHoverBinding()
{
	OnHovered.RemoveDynamic(this, &UConquestMainMenuButton::HandleHovered);
	OnHovered.AddDynamic(this, &UConquestMainMenuButton::HandleHovered);

	OnUnhovered.RemoveDynamic(this, &UConquestMainMenuButton::HandleUnhovered);
	OnUnhovered.AddDynamic(this, &UConquestMainMenuButton::HandleUnhovered);
}
