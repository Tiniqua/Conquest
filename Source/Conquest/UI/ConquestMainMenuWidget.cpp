#include "ConquestMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Conquest/Core/ConquestMultiplayerSessionSubsystem.h"
#include "ConquestHUDWidget.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/WidgetTree.h"

void UConquestMainMenuWidget::InitializeMainMenuWidget(UConquestHUDWidget* InParentHUDWidget)
{
	ParentHUDWidget = InParentHUDWidget;
}

void UConquestMainMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ConfigureBoundMainMenuButtonHoverScale();
}

void UConquestMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindMainMenuButtons();

	if (SingleplayerButton && !Cast<UConquestMainMenuButton>(SingleplayerButton))
	{
		SingleplayerButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonClicked);
		SingleplayerButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonClicked);
		SingleplayerButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonHovered);
		SingleplayerButton->OnHovered.AddDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonHovered);
		SingleplayerButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonUnhovered);
		SingleplayerButton->OnUnhovered.AddDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonUnhovered);
	}

	if (MultiplayerButton && !Cast<UConquestMainMenuButton>(MultiplayerButton))
	{
		MultiplayerButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonClicked);
		MultiplayerButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonClicked);
		MultiplayerButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonHovered);
		MultiplayerButton->OnHovered.AddDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonHovered);
		MultiplayerButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonUnhovered);
		MultiplayerButton->OnUnhovered.AddDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonUnhovered);
	}

	if (SettingsButton && !Cast<UConquestMainMenuButton>(SettingsButton))
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonClicked);
		SettingsButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonClicked);
		SettingsButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonHovered);
		SettingsButton->OnHovered.AddDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonHovered);
		SettingsButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonUnhovered);
		SettingsButton->OnUnhovered.AddDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonUnhovered);
	}

	if (QuitButton && !Cast<UConquestMainMenuButton>(QuitButton))
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonClicked);
		QuitButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonClicked);
		QuitButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonHovered);
		QuitButton->OnHovered.AddDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonHovered);
		QuitButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonUnhovered);
		QuitButton->OnUnhovered.AddDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonUnhovered);
	}

	if (HostGameButton && !Cast<UConquestMainMenuButton>(HostGameButton))
	{
		HostGameButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonClicked);
		HostGameButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonClicked);
		HostGameButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonHovered);
		HostGameButton->OnHovered.AddDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonHovered);
		HostGameButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonUnhovered);
		HostGameButton->OnUnhovered.AddDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonUnhovered);
	}

	if (JoinGameButton && !Cast<UConquestMainMenuButton>(JoinGameButton))
	{
		JoinGameButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonClicked);
		JoinGameButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonClicked);
		JoinGameButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonHovered);
		JoinGameButton->OnHovered.AddDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonHovered);
		JoinGameButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonUnhovered);
		JoinGameButton->OnUnhovered.AddDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonUnhovered);
	}

	if (ReturnButton && !Cast<UConquestMainMenuButton>(ReturnButton))
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonClicked);
		ReturnButton->OnClicked.AddDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonClicked);
		ReturnButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonHovered);
		ReturnButton->OnHovered.AddDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonHovered);
		ReturnButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonUnhovered);
		ReturnButton->OnUnhovered.AddDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonUnhovered);
	}

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->OnCreateSessionComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleCreateSessionComplete);
		SessionSubsystem->OnCreateSessionComplete.AddDynamic(this, &UConquestMainMenuWidget::HandleCreateSessionComplete);

		SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleFindSessionsComplete);
		SessionSubsystem->OnFindSessionsComplete.AddDynamic(this, &UConquestMainMenuWidget::HandleFindSessionsComplete);

		SessionSubsystem->OnJoinSessionComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinSessionComplete);
		SessionSubsystem->OnJoinSessionComplete.AddDynamic(this, &UConquestMainMenuWidget::HandleJoinSessionComplete);
	}

	SetLoadingStatus(FText::GetEmpty());
	SetMenuButtonsEnabled(true);
	ShowPrimaryMainMenuButtons();
}

void UConquestMainMenuWidget::NativeDestruct()
{
	UnbindMainMenuButtons();

	if (SingleplayerButton)
	{
		SingleplayerButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonClicked);
		SingleplayerButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonHovered);
		SingleplayerButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSingleplayerButtonUnhovered);
	}

	if (MultiplayerButton)
	{
		MultiplayerButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonClicked);
		MultiplayerButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonHovered);
		MultiplayerButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleMultiplayerButtonUnhovered);
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonClicked);
		SettingsButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonHovered);
		SettingsButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleSettingsButtonUnhovered);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonClicked);
		QuitButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonHovered);
		QuitButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleQuitButtonUnhovered);
	}

	if (HostGameButton)
	{
		HostGameButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonClicked);
		HostGameButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonHovered);
		HostGameButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleHostGameButtonUnhovered);
	}

	if (JoinGameButton)
	{
		JoinGameButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonClicked);
		JoinGameButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonHovered);
		JoinGameButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinGameButtonUnhovered);
	}

	if (ReturnButton)
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonClicked);
		ReturnButton->OnHovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonHovered);
		ReturnButton->OnUnhovered.RemoveDynamic(this, &UConquestMainMenuWidget::HandleReturnButtonUnhovered);
	}

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->OnCreateSessionComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleCreateSessionComplete);
		SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleFindSessionsComplete);
		SessionSubsystem->OnJoinSessionComplete.RemoveDynamic(this, &UConquestMainMenuWidget::HandleJoinSessionComplete);
	}

	Super::NativeDestruct();
}

void UConquestMainMenuWidget::HandleMainMenuButtonClicked(UConquestMainMenuButton* Button)
{
	if (!Button)
	{
		return;
	}

	const EConquestMainMenuButtonAction Action = ResolveButtonAction(Button);
	OnMainMenuActionSelected.Broadcast(Action, Button);
	ExecuteMainMenuAction(Action, Button);
}

void UConquestMainMenuWidget::HandleSingleplayerButtonClicked()
{
	OnMainMenuActionSelected.Broadcast(EConquestMainMenuButtonAction::Singleplayer, nullptr);
	ExecuteMainMenuAction(EConquestMainMenuButtonAction::Singleplayer, nullptr);
}

void UConquestMainMenuWidget::HandleSingleplayerButtonHovered()
{
	ApplyButtonHoverScale(SingleplayerButton);
}

void UConquestMainMenuWidget::HandleSingleplayerButtonUnhovered()
{
	ApplyButtonDefaultScale(SingleplayerButton);
}

void UConquestMainMenuWidget::HandleMultiplayerButtonClicked()
{
	OnMainMenuActionSelected.Broadcast(EConquestMainMenuButtonAction::Multiplayer, nullptr);
	ExecuteMainMenuAction(EConquestMainMenuButtonAction::Multiplayer, nullptr);
}

void UConquestMainMenuWidget::HandleMultiplayerButtonHovered()
{
	ApplyButtonHoverScale(MultiplayerButton);
}

void UConquestMainMenuWidget::HandleMultiplayerButtonUnhovered()
{
	ApplyButtonDefaultScale(MultiplayerButton);
}

void UConquestMainMenuWidget::HandleSettingsButtonClicked()
{
	OnMainMenuActionSelected.Broadcast(EConquestMainMenuButtonAction::Settings, nullptr);
	ExecuteMainMenuAction(EConquestMainMenuButtonAction::Settings, nullptr);
}

void UConquestMainMenuWidget::HandleSettingsButtonHovered()
{
	ApplyButtonHoverScale(SettingsButton);
}

void UConquestMainMenuWidget::HandleSettingsButtonUnhovered()
{
	ApplyButtonDefaultScale(SettingsButton);
}

void UConquestMainMenuWidget::HandleQuitButtonClicked()
{
	OnMainMenuActionSelected.Broadcast(EConquestMainMenuButtonAction::Quit, nullptr);
	ExecuteMainMenuAction(EConquestMainMenuButtonAction::Quit, nullptr);
}

void UConquestMainMenuWidget::HandleQuitButtonHovered()
{
	ApplyButtonHoverScale(QuitButton);
}

void UConquestMainMenuWidget::HandleQuitButtonUnhovered()
{
	ApplyButtonDefaultScale(QuitButton);
}

void UConquestMainMenuWidget::HandleHostGameButtonClicked()
{
	SetMenuButtonsEnabled(false);
	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuHostingGame", "Hosting game"));

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->HostSession(PublicConnections, bUseLanSessions);
	}
	else
	{
		HandleCreateSessionComplete(false, NAME_None);
	}
}

void UConquestMainMenuWidget::HandleHostGameButtonHovered()
{
	ApplyButtonHoverScale(HostGameButton);
}

void UConquestMainMenuWidget::HandleHostGameButtonUnhovered()
{
	ApplyButtonDefaultScale(HostGameButton);
}

void UConquestMainMenuWidget::HandleJoinGameButtonClicked()
{
	SetMenuButtonsEnabled(false);
	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuSearchingSessions", "Searching"));

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->FindSessions(MaxSessionSearchResults, bUseLanSessions);
	}
	else
	{
		HandleFindSessionsComplete(0);
	}
}

void UConquestMainMenuWidget::HandleJoinGameButtonHovered()
{
	ApplyButtonHoverScale(JoinGameButton);
}

void UConquestMainMenuWidget::HandleJoinGameButtonUnhovered()
{
	ApplyButtonDefaultScale(JoinGameButton);
}

void UConquestMainMenuWidget::HandleReturnButtonClicked()
{
	OnMainMenuActionSelected.Broadcast(EConquestMainMenuButtonAction::ReturnToMainMenu, nullptr);
	ExecuteMainMenuAction(EConquestMainMenuButtonAction::ReturnToMainMenu, nullptr);
}

void UConquestMainMenuWidget::HandleReturnButtonHovered()
{
	ApplyButtonHoverScale(ReturnButton);
}

void UConquestMainMenuWidget::HandleReturnButtonUnhovered()
{
	ApplyButtonDefaultScale(ReturnButton);
}

void UConquestMainMenuWidget::HandleCreateSessionComplete(bool bWasSuccessful, FName)
{
	if (!bWasSuccessful)
	{
		SetMenuButtonsEnabled(true);
		SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuHostFailed", "Could not host game"));
		return;
	}

	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuHostReady", "Host ready"));

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (!CurrentLevelName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(
			this,
			FName(*CurrentLevelName),
			true,
			TEXT("?listen?ConquestHostSetup=1")
		);
	}
}

void UConquestMainMenuWidget::HandleFindSessionsComplete(int32 ResultCount)
{
	SetLoadingStatus(FText::Format(
		NSLOCTEXT("Conquest", "MainMenuSessionsFound", "{0} found"),
		FText::AsNumber(ResultCount)
	));

	OnFindSessionsFinished(ResultCount);

	if (ResultCount <= JoinSessionResultIndex)
	{
		SetMenuButtonsEnabled(true);
		return;
	}

	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuJoiningGame", "Joining game"));

	if (UConquestMultiplayerSessionSubsystem* SessionSubsystem = GetMultiplayerSessionSubsystem())
	{
		SessionSubsystem->JoinSessionByIndex(JoinSessionResultIndex);
	}
	else
	{
		HandleJoinSessionComplete(false, FString());
	}
}

void UConquestMainMenuWidget::HandleJoinSessionComplete(bool bWasSuccessful, FString TravelURL)
{
	OnJoinSessionFinished(bWasSuccessful, TravelURL);

	if (!bWasSuccessful || TravelURL.IsEmpty())
	{
		SetMenuButtonsEnabled(true);
		SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuJoinFailed", "Could not join game"));
		return;
	}

	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuJoiningGame", "Joining game"));

	SetLoadingStatus(NSLOCTEXT("Conquest", "MainMenuJoinTravel", "Joining game"));
}

void UConquestMainMenuWidget::SetLoadingStatus(const FText& NewLoadingStatus)
{
	LoadingStatus = NewLoadingStatus;

	if (LoadingStatusText)
	{
		LoadingStatusText->SetText(LoadingStatus);
	}
}

FText UConquestMainMenuWidget::GetLoadingStatus() const
{
	return LoadingStatus;
}

void UConquestMainMenuWidget::BindMainMenuButtons()
{
	UnbindMainMenuButtons();

	TArray<UWidget*> Widgets;
	if (WidgetTree)
	{
		WidgetTree->GetAllWidgets(Widgets);
	}

	for (UWidget* Widget : Widgets)
	{
		UConquestMainMenuButton* MainMenuButton = Cast<UConquestMainMenuButton>(Widget);
		if (!MainMenuButton)
		{
			continue;
		}

		MainMenuButton->EnsureClickBinding();
		MainMenuButton->EnsureHoverBinding();
		MainMenuButton->OnConquestMainMenuButtonClicked.RemoveDynamic(
			this,
			&UConquestMainMenuWidget::HandleMainMenuButtonClicked
		);
		MainMenuButton->OnConquestMainMenuButtonClicked.AddDynamic(
			this,
			&UConquestMainMenuWidget::HandleMainMenuButtonClicked
		);

		BoundMainMenuButtons.Add(MainMenuButton);
	}

	ConfigureBoundMainMenuButtonHoverScale();
}

void UConquestMainMenuWidget::UnbindMainMenuButtons()
{
	for (UConquestMainMenuButton* Button : BoundMainMenuButtons)
	{
		if (!Button)
		{
			continue;
		}

		Button->OnConquestMainMenuButtonClicked.RemoveDynamic(
			this,
			&UConquestMainMenuWidget::HandleMainMenuButtonClicked
		);
	}

	BoundMainMenuButtons.Reset();
}

EConquestMainMenuButtonAction UConquestMainMenuWidget::ResolveButtonAction(const UConquestMainMenuButton* Button) const
{
	if (!Button)
	{
		return EConquestMainMenuButtonAction::FromButtonName;
	}

	if (Button->MenuAction != EConquestMainMenuButtonAction::FromButtonName)
	{
		return Button->MenuAction;
	}

	return ResolveButtonActionFromName(Button->GetFName());
}

EConquestMainMenuButtonAction UConquestMainMenuWidget::ResolveButtonActionFromName(FName ButtonName) const
{
	const FString LowerName = ButtonName.ToString().ToLower();

	if (LowerName.Contains(TEXT("singleplayer")) || LowerName.Contains(TEXT("single_player")))
	{
		return EConquestMainMenuButtonAction::Singleplayer;
	}

	if (LowerName.Contains(TEXT("multiplayer")) || LowerName.Contains(TEXT("multi_player")))
	{
		return EConquestMainMenuButtonAction::Multiplayer;
	}

	if (LowerName.Contains(TEXT("settings")) || LowerName.Contains(TEXT("options")))
	{
		return EConquestMainMenuButtonAction::Settings;
	}

	if (LowerName.Contains(TEXT("quit")) || LowerName.Contains(TEXT("exit")))
	{
		return EConquestMainMenuButtonAction::Quit;
	}

	if (LowerName.Contains(TEXT("host")))
	{
		return EConquestMainMenuButtonAction::HostGame;
	}

	if (LowerName.Contains(TEXT("join")))
	{
		return EConquestMainMenuButtonAction::JoinGame;
	}

	if (LowerName.Contains(TEXT("return")) || LowerName.Contains(TEXT("back")))
	{
		return EConquestMainMenuButtonAction::ReturnToMainMenu;
	}

	return EConquestMainMenuButtonAction::FromButtonName;
}

void UConquestMainMenuWidget::ExecuteMainMenuAction(
	EConquestMainMenuButtonAction Action,
	UConquestMainMenuButton* Button
)
{
	switch (Action)
	{
	case EConquestMainMenuButtonAction::Singleplayer:
		if (ParentHUDWidget)
		{
			ParentHUDWidget->ShowGameSetup();
		}
		break;

	case EConquestMainMenuButtonAction::Multiplayer:
		ShowMultiplayerMenuButtons();
		OnMultiplayerRequested(Button);
		break;

	case EConquestMainMenuButtonAction::Settings:
		OnSettingsRequested(Button);
		break;

	case EConquestMainMenuButtonAction::Quit:
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		break;

	case EConquestMainMenuButtonAction::HostGame:
		HandleHostGameButtonClicked();
		break;

	case EConquestMainMenuButtonAction::JoinGame:
		HandleJoinGameButtonClicked();
		break;

	case EConquestMainMenuButtonAction::ReturnToMainMenu:
		SetLoadingStatus(FText::GetEmpty());
		SetMenuButtonsEnabled(true);
		ShowPrimaryMainMenuButtons();
		break;

	case EConquestMainMenuButtonAction::Custom:
		OnCustomMainMenuButtonRequested(Button, Button ? Button->CustomActionName : NAME_None);
		break;

	case EConquestMainMenuButtonAction::FromButtonName:
	default:
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("ConquestMainMenuWidget: Could not resolve action for main menu button '%s'."),
			Button ? *Button->GetName() : TEXT("None")
		);
		break;
	}
}

void UConquestMainMenuWidget::ShowPrimaryMainMenuButtons()
{
	if (SingleplayerButton)
	{
		ApplyButtonDefaultScale(SingleplayerButton);
		SingleplayerButton->SetVisibility(ESlateVisibility::Visible);
	}

	if (MultiplayerButton)
	{
		ApplyButtonDefaultScale(MultiplayerButton);
		MultiplayerButton->SetVisibility(ESlateVisibility::Visible);
	}

	if (SettingsButton)
	{
		ApplyButtonDefaultScale(SettingsButton);
		SettingsButton->SetVisibility(ESlateVisibility::Visible);
	}

	if (QuitButton)
	{
		ApplyButtonDefaultScale(QuitButton);
		QuitButton->SetVisibility(ESlateVisibility::Visible);
	}

	if (HostGameButton)
	{
		ApplyButtonDefaultScale(HostGameButton);
		HostGameButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (JoinGameButton)
	{
		ApplyButtonDefaultScale(JoinGameButton);
		JoinGameButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ReturnButton)
	{
		ApplyButtonDefaultScale(ReturnButton);
		ReturnButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	RefreshMenuButtonSpacerVisibility();
}

void UConquestMainMenuWidget::ShowMultiplayerMenuButtons()
{
	if (SingleplayerButton)
	{
		ApplyButtonDefaultScale(SingleplayerButton);
		SingleplayerButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (MultiplayerButton)
	{
		ApplyButtonDefaultScale(MultiplayerButton);
		MultiplayerButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SettingsButton)
	{
		ApplyButtonDefaultScale(SettingsButton);
		SettingsButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (QuitButton)
	{
		ApplyButtonDefaultScale(QuitButton);
		QuitButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (HostGameButton)
	{
		ApplyButtonDefaultScale(HostGameButton);
		HostGameButton->SetVisibility(ESlateVisibility::Visible);
	}

	if (JoinGameButton)
	{
		ApplyButtonDefaultScale(JoinGameButton);
		JoinGameButton->SetVisibility(ESlateVisibility::Visible);
	}

	if (ReturnButton)
	{
		ApplyButtonDefaultScale(ReturnButton);
		ReturnButton->SetVisibility(ESlateVisibility::Visible);
	}

	RefreshMenuButtonSpacerVisibility();
}

void UConquestMainMenuWidget::ConfigureBoundMainMenuButtonHoverScale()
{
	for (UConquestMainMenuButton* Button : BoundMainMenuButtons)
	{
		if (Button)
		{
			Button->ConfigureHoverScale(bScaleButtonsOnHover, ButtonHoverScale, ButtonDefaultScale);
		}
	}
}

void UConquestMainMenuWidget::ApplyButtonHoverScale(UButton* Button) const
{
	if (bScaleButtonsOnHover && Button)
	{
		Button->SetRenderScale(FVector2D(ButtonHoverScale, ButtonHoverScale));
	}
}

void UConquestMainMenuWidget::ApplyButtonDefaultScale(UButton* Button) const
{
	if (bScaleButtonsOnHover && Button)
	{
		Button->SetRenderScale(FVector2D(ButtonDefaultScale, ButtonDefaultScale));
	}
}

void UConquestMainMenuWidget::RefreshMenuButtonSpacerVisibility()
{
	UPanelWidget* Panel = GetMenuButtonPanel();
	if (!Panel)
	{
		return;
	}

	const int32 ChildCount = Panel->GetChildrenCount();
	for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
	{
		UWidget* Child = Panel->GetChildAt(ChildIndex);
		if (!Child || !Child->IsA<USpacer>())
		{
			continue;
		}

		const bool bShouldShowSpacer =
			HasVisibleNonSpacerChildBefore(Panel, ChildIndex) &&
			HasVisibleNonSpacerChildAfter(Panel, ChildIndex);

		Child->SetVisibility(bShouldShowSpacer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

UPanelWidget* UConquestMainMenuWidget::GetMenuButtonPanel() const
{
	if (MenuButtonVertBox)
	{
		return MenuButtonVertBox;
	}

	if (SingleplayerButton)
	{
		return SingleplayerButton->GetParent();
	}

	if (MultiplayerButton)
	{
		return MultiplayerButton->GetParent();
	}

	if (SettingsButton)
	{
		return SettingsButton->GetParent();
	}

	if (QuitButton)
	{
		return QuitButton->GetParent();
	}

	if (HostGameButton)
	{
		return HostGameButton->GetParent();
	}

	if (JoinGameButton)
	{
		return JoinGameButton->GetParent();
	}

	return ReturnButton ? ReturnButton->GetParent() : nullptr;
}

bool UConquestMainMenuWidget::HasVisibleNonSpacerChildBefore(const UPanelWidget* Panel, int32 ChildIndex) const
{
	if (!Panel)
	{
		return false;
	}

	for (int32 Index = ChildIndex - 1; Index >= 0; --Index)
	{
		if (IsVisibleNonSpacerWidget(Panel->GetChildAt(Index)))
		{
			return true;
		}
	}

	return false;
}

bool UConquestMainMenuWidget::HasVisibleNonSpacerChildAfter(const UPanelWidget* Panel, int32 ChildIndex) const
{
	if (!Panel)
	{
		return false;
	}

	const int32 ChildCount = Panel->GetChildrenCount();
	for (int32 Index = ChildIndex + 1; Index < ChildCount; ++Index)
	{
		if (IsVisibleNonSpacerWidget(Panel->GetChildAt(Index)))
		{
			return true;
		}
	}

	return false;
}

bool UConquestMainMenuWidget::IsVisibleNonSpacerWidget(const UWidget* Widget) const
{
	if (!Widget || Widget->IsA<USpacer>())
	{
		return false;
	}

	const ESlateVisibility WidgetVisibility = Widget->GetVisibility();
	return WidgetVisibility != ESlateVisibility::Collapsed && WidgetVisibility != ESlateVisibility::Hidden;
}

void UConquestMainMenuWidget::SetMenuButtonsEnabled(bool bEnabled)
{
	TSet<UButton*> UpdatedButtons;

	for (UConquestMainMenuButton* Button : BoundMainMenuButtons)
	{
		if (Button)
		{
			Button->SetIsEnabled(bEnabled);
			UpdatedButtons.Add(Button);
		}
	}

	if (SingleplayerButton)
	{
		if (!UpdatedButtons.Contains(SingleplayerButton))
		{
			SingleplayerButton->SetIsEnabled(bEnabled);
		}
	}

	if (MultiplayerButton)
	{
		if (!UpdatedButtons.Contains(MultiplayerButton))
		{
			MultiplayerButton->SetIsEnabled(bEnabled);
		}
	}

	if (SettingsButton)
	{
		if (!UpdatedButtons.Contains(SettingsButton))
		{
			SettingsButton->SetIsEnabled(bEnabled);
		}
	}

	if (QuitButton)
	{
		if (!UpdatedButtons.Contains(QuitButton))
		{
			QuitButton->SetIsEnabled(bEnabled);
		}
	}

	if (HostGameButton)
	{
		if (!UpdatedButtons.Contains(HostGameButton))
		{
			HostGameButton->SetIsEnabled(bEnabled);
		}
	}

	if (JoinGameButton)
	{
		if (!UpdatedButtons.Contains(JoinGameButton))
		{
			JoinGameButton->SetIsEnabled(bEnabled);
		}
	}

	if (ReturnButton)
	{
		if (!UpdatedButtons.Contains(ReturnButton))
		{
			ReturnButton->SetIsEnabled(bEnabled);
		}
	}
}

UConquestMultiplayerSessionSubsystem* UConquestMainMenuWidget::GetMultiplayerSessionSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UConquestMultiplayerSessionSubsystem>() : nullptr;
}
