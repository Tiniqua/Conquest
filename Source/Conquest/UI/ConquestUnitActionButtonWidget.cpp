#include "Conquest/UI/ConquestUnitActionButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UConquestUnitActionButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &UConquestUnitActionButtonWidget::HandleClicked);
		Button->OnClicked.AddDynamic(this, &UConquestUnitActionButtonWidget::HandleClicked);
	}

	if (TitleText)
	{
		TitleText->SetAutoWrapText(true);
		TitleText->SetJustification(ETextJustify::Center);
		TitleText->SetMinDesiredWidth(0.0f);
	}
}

void UConquestUnitActionButtonWidget::SetupUnitActionButton(FName InActionId, const FText& InTitle)
{
	ActionId = InActionId;

	if (TitleText)
	{
		TitleText->SetText(InTitle);
		TitleText->SetAutoWrapText(true);
		TitleText->SetJustification(ETextJustify::Center);
		TitleText->SetMinDesiredWidth(0.0f);
	}
}

void UConquestUnitActionButtonWidget::SetActionEnabled(bool bEnabled)
{
	SetIsEnabled(bEnabled);
	if (Button)
	{
		Button->SetIsEnabled(bEnabled);
	}
}

void UConquestUnitActionButtonWidget::HandleClicked()
{
	OnUnitActionClicked.Broadcast(ActionId);
}
