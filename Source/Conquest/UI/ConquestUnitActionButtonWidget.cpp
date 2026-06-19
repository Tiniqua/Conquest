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
}

void UConquestUnitActionButtonWidget::SetupUnitActionButton(FName InActionId, const FText& InTitle)
{
	ActionId = InActionId;

	if (TitleText)
	{
		TitleText->SetText(InTitle);
	}
}

void UConquestUnitActionButtonWidget::HandleClicked()
{
	OnUnitActionClicked.Broadcast(ActionId);
}
