#include "Conquest/UI/ConquestChoiceButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UConquestChoiceButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &UConquestChoiceButtonWidget::HandleClicked);
		Button->OnClicked.AddDynamic(this, &UConquestChoiceButtonWidget::HandleClicked);
	}

	RefreshVisuals();
}

void UConquestChoiceButtonWidget::SetupChoiceButton(const FConquestChoiceButtonData& InChoiceData)
{
	ChoiceData = InChoiceData;
	RefreshVisuals();
}

void UConquestChoiceButtonWidget::RefreshVisuals()
{
	if (TitleText)
	{
		TitleText->SetText(ChoiceData.Title);
	}

	if (SubtitleText)
	{
		SubtitleText->SetText(ChoiceData.Subtitle);
		SubtitleText->SetVisibility(ChoiceData.Subtitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (DetailText)
	{
		DetailText->SetText(ChoiceData.DetailText);
		DetailText->SetVisibility(ChoiceData.DetailText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (Button)
	{
		Button->SetIsEnabled(ChoiceData.bEnabled);
	}
}

void UConquestChoiceButtonWidget::HandleClicked()
{
	if (!ChoiceData.bEnabled)
	{
		return;
	}

	OnChoiceClicked.Broadcast(ChoiceData);
}