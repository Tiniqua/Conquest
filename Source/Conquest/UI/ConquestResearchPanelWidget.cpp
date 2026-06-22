#include "Conquest/UI/ConquestResearchPanelWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"

#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/Tech/ConquestTechTypes.h"
#include "Conquest/UI/ConquestChoiceButtonWidget.h"
#include "Conquest/UI/ConquestHUD.h"

void UConquestResearchPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshResearchOptions();
}

void UConquestResearchPanelWidget::RefreshResearchOptions()
{
	AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!GS || !GS->TechManager || !GS->ContentManager)
	{
		return;
	}

	const FConquestPlayerEmpireState& Player = GS->GetHumanPlayer();

	if (CurrentResearchText)
	{
		if (!Player.CurrentResearchId.IsNone())
		{
			const FConquestTechRow* CurrentTech =
				GS->ContentManager->FindTech(Player.CurrentResearchId);

			if (CurrentTech)
			{
				const int32 Turns =
					GS->TechManager->EstimateTurnsToResearchById(
						Player.PlayerId,
						Player.CurrentResearchId
					);

				CurrentResearchText->SetText(FText::Format(
					NSLOCTEXT("Conquest", "CurrentResearchFormat", "Researching: {0} - {1} Turns"),
					CurrentTech->DisplayName,
					FText::AsNumber(Turns)
				));
			}
			else
			{
				CurrentResearchText->SetText(FText::Format(
					NSLOCTEXT("Conquest", "UnknownCurrentResearchFormat", "Researching: {0}"),
					FText::FromName(Player.CurrentResearchId)
				));
			}
		}
		else
		{
			CurrentResearchText->SetText(
				NSLOCTEXT("Conquest", "NoResearch", "Choose Research")
			);
		}
	}

	if (!TechButtonBox)
	{
		return;
	}

	TechButtonBox->ClearChildren();

	if (!ChoiceButtonWidgetClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("ConquestResearchPanelWidget: ChoiceButtonWidgetClass is not set.")
		);
		return;
	}

	const TArray<FName> AvailableTechIds =
		GS->TechManager->GetAvailableResearchIds(Player.PlayerId);

	for (const FName TechId : AvailableTechIds)
	{
		if (TechId.IsNone())
		{
			continue;
		}

		const FConquestTechRow* TechRow =
			GS->ContentManager->FindTech(TechId);

		if (!TechRow)
		{
			continue;
		}

		UConquestChoiceButtonWidget* ChoiceButton =
			CreateWidget<UConquestChoiceButtonWidget>(
				GetOwningPlayer(),
				ChoiceButtonWidgetClass
			);

		if (!ChoiceButton)
		{
			continue;
		}

		const int32 Turns =
			GS->TechManager->EstimateTurnsToResearchById(
				Player.PlayerId,
				TechId
			);

		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::ResearchTech;
		ChoiceData.ChoiceId = TechId;
		ChoiceData.Payload = nullptr;
		ChoiceData.Title = TechRow->DisplayName;
		ChoiceData.Cost = TechRow->ScienceCost;
		ChoiceData.Turns = Turns;
		ChoiceData.bEnabled = true;
		ChoiceData.DetailText = TechRow->Description;

		if (Turns == INDEX_NONE)
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "ResearchChoiceNoTurns", "{0} Science"),
				FText::AsNumber(TechRow->ScienceCost)
			);
		}
		else
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "ResearchChoiceTurns", "{0} Science | {1} Turns"),
				FText::AsNumber(TechRow->ScienceCost),
				FText::AsNumber(Turns)
			);
		}

		ChoiceButton->SetupChoiceButton(ChoiceData);

		ChoiceButton->OnChoiceClicked.RemoveDynamic(
			this,
			&UConquestResearchPanelWidget::HandleResearchChoiceClicked
		);

		ChoiceButton->OnChoiceClicked.AddDynamic(
			this,
			&UConquestResearchPanelWidget::HandleResearchChoiceClicked
		);

		TechButtonBox->AddChild(ChoiceButton);
	}
}

void UConquestResearchPanelWidget::HandleResearchChoiceClicked(
	const FConquestChoiceButtonData& ChoiceData
)
{
	if (ChoiceData.ChoiceType != EConquestChoiceType::ResearchTech)
	{
		return;
	}

	if (ChoiceData.ChoiceId.IsNone())
	{
		return;
	}

	HandleTechClicked(ChoiceData.ChoiceId);
}

void UConquestResearchPanelWidget::HandleTechClicked(FName TechId)
{
	if (TechId.IsNone())
	{
		return;
	}

	AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!GS || !GS->TechManager)
	{
		return;
	}

	bool bSelectedResearch = false;
	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->RequestSetCurrentResearch(TechId);
		bSelectedResearch = true;
	}
	RefreshResearchOptions();

	if (bSelectedResearch)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PC->GetHUD()))
			{
				ConquestHUD->HideResearchPanel();
			}
		}
	}
}
