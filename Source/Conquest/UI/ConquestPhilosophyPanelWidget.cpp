#include "Conquest/UI/ConquestPhilosophyPanelWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"

#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestPhilosophyManager.h"
#include "Conquest/Philosophies/ConquestPhilosophyTypes.h"
#include "Conquest/Player/ConquestPlayerController.h"
#include "Conquest/UI/ConquestChoiceButtonWidget.h"
#include "Conquest/UI/ConquestHUD.h"

void UConquestPhilosophyPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UConquestPhilosophyPanelWidget::HandleCloseClicked);
		CloseButton->OnClicked.AddDynamic(this, &UConquestPhilosophyPanelWidget::HandleCloseClicked);
	}

	RefreshPhilosophyOptions();
}

void UConquestPhilosophyPanelWidget::NativeDestruct()
{
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UConquestPhilosophyPanelWidget::HandleCloseClicked);
	}

	Super::NativeDestruct();
}

void UConquestPhilosophyPanelWidget::RefreshPhilosophyOptions()
{
	AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;

	if (!GS || !GS->PhilosophyManager || !GS->ContentManager)
	{
		return;
	}

	const FConquestPlayerEmpireState& Player = GS->GetHumanPlayer();
	const int32 NextCost = GS->PhilosophyManager->GetNextPhilosophyCultureCost(Player.PlayerId);
	const int32 StoredCulture = Player.StoredYields.Culture;
	const int32 CulturePerTurn = Player.CachedYieldPerTurn.Culture;
	const int32 TurnsToNext = StoredCulture >= NextCost
		? 0
		: (CulturePerTurn > 0 ? FMath::CeilToInt(static_cast<float>(NextCost - StoredCulture) / static_cast<float>(CulturePerTurn)) : INDEX_NONE);

	if (CultureStatusText)
	{
		CultureStatusText->SetText(TurnsToNext == INDEX_NONE
			? FText::Format(
				NSLOCTEXT("Conquest", "PhilosophyCultureNoTurns", "Culture: {0} (+{1}) | Next: {2}"),
				FText::AsNumber(StoredCulture),
				FText::AsNumber(CulturePerTurn),
				FText::AsNumber(NextCost)
			)
			: FText::Format(
				NSLOCTEXT("Conquest", "PhilosophyCultureTurns", "Culture: {0} (+{1}) | Next: {2} | {3} Turns"),
				FText::AsNumber(StoredCulture),
				FText::AsNumber(CulturePerTurn),
				FText::AsNumber(NextCost),
				FText::AsNumber(TurnsToNext)
			));
	}

	if (AdoptedPhilosophyText)
	{
		AdoptedPhilosophyText->SetText(FText::Format(
			NSLOCTEXT("Conquest", "AdoptedPhilosophyCount", "Adopted: {0}"),
			FText::AsNumber(Player.AdoptedPhilosophyIds.Num())
		));
	}

	if (!PhilosophyButtonBox)
	{
		return;
	}

	PhilosophyButtonBox->ClearChildren();

	if (!ChoiceButtonWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ConquestPhilosophyPanelWidget: ChoiceButtonWidgetClass is not set."));
		return;
	}

	TArray<const FConquestPhilosophyRow*> AllPhilosophies;
	GS->ContentManager->GetAllPhilosophies(AllPhilosophies);

	AllPhilosophies.Sort([](const FConquestPhilosophyRow& A, const FConquestPhilosophyRow& B)
	{
		if (A.BranchId != B.BranchId)
		{
			return A.BranchId.LexicalLess(B.BranchId);
		}
		return A.PhilosophyId.LexicalLess(B.PhilosophyId);
	});

	for (const FConquestPhilosophyRow* PhilosophyRow : AllPhilosophies)
	{
		if (!PhilosophyRow || PhilosophyRow->PhilosophyId.IsNone())
		{
			continue;
		}

		UConquestChoiceButtonWidget* ChoiceButton = CreateWidget<UConquestChoiceButtonWidget>(
			GetOwningPlayer(),
			ChoiceButtonWidgetClass
		);

		if (!ChoiceButton)
		{
			continue;
		}

		const bool bAdopted = Player.HasAdoptedPhilosophy(PhilosophyRow->PhilosophyId);
		const bool bPrereqsMet = GS->PhilosophyManager->CanAdoptPhilosophy(Player.PlayerId, PhilosophyRow->PhilosophyId);
		const bool bCanAfford = StoredCulture >= NextCost;

		FConquestChoiceButtonData ChoiceData;
		ChoiceData.ChoiceType = EConquestChoiceType::Philosophy;
		ChoiceData.ChoiceId = PhilosophyRow->PhilosophyId;
		ChoiceData.Title = PhilosophyRow->DisplayName.IsEmpty()
			? FText::FromName(PhilosophyRow->PhilosophyId)
			: PhilosophyRow->DisplayName;
		ChoiceData.DetailText = PhilosophyRow->Description;
		ChoiceData.Cost = NextCost;
		ChoiceData.Turns = TurnsToNext;
		ChoiceData.bEnabled = !bAdopted && bPrereqsMet && bCanAfford;

		if (bAdopted)
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "PhilosophyChoiceAdopted", "Adopted | {0}"),
				FText::FromName(PhilosophyRow->BranchId)
			);
			ChoiceData.DisabledReason = NSLOCTEXT("Conquest", "PhilosophyAlreadyAdopted", "Already adopted");
		}
		else if (!bPrereqsMet)
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "PhilosophyChoiceLocked", "Locked | {0}"),
				BuildLockedReasonText(PhilosophyRow->PrerequisitePhilosophyIds)
			);
			ChoiceData.DisabledReason = ChoiceData.Subtitle;
		}
		else if (!bCanAfford)
		{
			ChoiceData.Subtitle = TurnsToNext == INDEX_NONE
				? FText::Format(
					NSLOCTEXT("Conquest", "PhilosophyChoiceNeedCultureNoTurns", "{0} Culture"),
					FText::AsNumber(NextCost)
				)
				: FText::Format(
					NSLOCTEXT("Conquest", "PhilosophyChoiceNeedCultureTurns", "{0} Culture | {1} Turns"),
					FText::AsNumber(NextCost),
					FText::AsNumber(TurnsToNext)
				);
			ChoiceData.DisabledReason = NSLOCTEXT("Conquest", "PhilosophyNeedCulture", "Not enough culture");
		}
		else
		{
			ChoiceData.Subtitle = FText::Format(
				NSLOCTEXT("Conquest", "PhilosophyChoiceAvailable", "{0} Culture | Available"),
				FText::AsNumber(NextCost)
			);
		}

		ChoiceButton->SetupChoiceButton(ChoiceData);
		ChoiceButton->OnChoiceClicked.RemoveDynamic(this, &UConquestPhilosophyPanelWidget::HandlePhilosophyChoiceClicked);
		ChoiceButton->OnChoiceClicked.AddDynamic(this, &UConquestPhilosophyPanelWidget::HandlePhilosophyChoiceClicked);
		PhilosophyButtonBox->AddChild(ChoiceButton);
	}
}

void UConquestPhilosophyPanelWidget::HandlePhilosophyChoiceClicked(const FConquestChoiceButtonData& ChoiceData)
{
	if (ChoiceData.ChoiceType != EConquestChoiceType::Philosophy || ChoiceData.ChoiceId.IsNone())
	{
		return;
	}

	if (AConquestPlayerController* ConquestPC = Cast<AConquestPlayerController>(GetOwningPlayer()))
	{
		ConquestPC->RequestAdoptPhilosophy(ChoiceData.ChoiceId);
	}

	RefreshPhilosophyOptions();
}

void UConquestPhilosophyPanelWidget::HandleCloseClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(PC->GetHUD()))
		{
			ConquestHUD->HidePhilosophyPanel();
		}
	}
}

FText UConquestPhilosophyPanelWidget::BuildLockedReasonText(const TArray<FName>& PrerequisiteIds) const
{
	TArray<FString> MissingNames;
	const AConquestGameState* GS = GetWorld()
		? GetWorld()->GetGameState<AConquestGameState>()
		: nullptr;
	const FConquestPlayerEmpireState* Player = GS
		? &GS->GetHumanPlayer()
		: nullptr;

	for (const FName PrereqId : PrerequisiteIds)
	{
		if (PrereqId.IsNone() || (Player && Player->HasAdoptedPhilosophy(PrereqId)))
		{
			continue;
		}

		const FConquestPhilosophyRow* PrereqRow = GS && GS->ContentManager
			? GS->ContentManager->FindPhilosophy(PrereqId)
			: nullptr;
		MissingNames.Add(PrereqRow && !PrereqRow->DisplayName.IsEmpty()
			? PrereqRow->DisplayName.ToString()
			: PrereqId.ToString());
	}

	return MissingNames.Num() > 0
		? FText::FromString(FString::Printf(TEXT("Requires %s"), *FString::Join(MissingNames, TEXT(", "))))
		: NSLOCTEXT("Conquest", "PhilosophyLockedUnknown", "Requires prerequisites");
}
