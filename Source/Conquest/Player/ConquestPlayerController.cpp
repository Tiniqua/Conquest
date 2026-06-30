#include "ConquestPlayerController.h"

#include "Components/AudioComponent.h"
#include "Conquest/Core/ConquestCheats.h"
#include "Conquest/Core/ConquestContentManager.h"
#include "Conquest/Core/ConquestPlayerEmpireState.h"
#include "Conquest/Core/ConquestUserSettingsSaveGame.h"
#include "Conquest/Framework/GameModes/ConquestGameMode.h"
#include "Conquest/Framework/GameModes/ConquestGameState.h"
#include "Conquest/Managers/ConquestCityManager.h"
#include "Conquest/Managers/ConquestPhilosophyManager.h"
#include "Conquest/Managers/ConquestTechManager.h"
#include "Conquest/UI/ConquestHUD.h"
#include "Conquest/World/Generation/HexGridModel.h"
#include "Conquest/World/Generation/HexImprovementTypes.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

namespace
{
	constexpr const TCHAR* ConquestUserSettingsSaveSlotName = TEXT("ConquestUserSettings");
	constexpr int32 ConquestUserSettingsSaveUserIndex = 0;

	AConquestGameMode* GetConquestGameMode(const AConquestPlayerController* PlayerController)
	{
		return PlayerController && PlayerController->GetWorld()
			? PlayerController->GetWorld()->GetAuthGameMode<AConquestGameMode>()
			: nullptr;
	}
}

AConquestPlayerController::AConquestPlayerController()
{
	BackgroundMusicAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BackgroundMusicAudioComponent"));
	if (BackgroundMusicAudioComponent)
	{
		BackgroundMusicAudioComponent->bAutoActivate = false;
		BackgroundMusicAudioComponent->bIsUISound = true;
	}

	CheatClass = UConquestCheats::StaticClass();
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	DefaultMouseCursor = EMouseCursor::Default;
}

void AConquestPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	if (IsLocalController() && !CheatManager)
	{
		EnableCheats();
	}
#endif

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	if (IsLocalController())
	{
		LoadUserSettings();
		ApplyMusicSettings();
	}

	if (BackgroundMusicAudioComponent)
	{
		BackgroundMusicAudioComponent->OnAudioFinished.RemoveDynamic(this, &AConquestPlayerController::HandleBackgroundMusicFinished);
		BackgroundMusicAudioComponent->OnAudioFinished.AddDynamic(this, &AConquestPlayerController::HandleBackgroundMusicFinished);
	}

	if (HasAuthority())
	{
		if (const AConquestGameMode* ConquestGM = GetWorld() ? GetWorld()->GetAuthGameMode<AConquestGameMode>() : nullptr)
		{
			InitializeBackgroundMusic(ConquestGM->GetBackgroundMusicTracks());
		}
	}
}

void AConquestPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindAction(TEXT("ToggleSettingsMenu"), IE_Pressed, this, &AConquestPlayerController::ToggleSettingsMenu);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AConquestPlayerController::ToggleSettingsMenu);
}

void AConquestPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AConquestPlayerController, AssignedPlayerId);
}

void AConquestPlayerController::SetAssignedPlayerId(int32 NewPlayerId)
{
	if (HasAuthority())
	{
		AssignedPlayerId = NewPlayerId;
	}
}

void AConquestPlayerController::RequestEndTurn()
{
	if (HasAuthority())
	{
		ServerRequestEndTurn_Implementation();
	}
	else
	{
		ServerRequestEndTurn();
	}
}

void AConquestPlayerController::RequestReturnToMainMenu()
{
	if (HasAuthority())
	{
		ServerRequestReturnToMainMenu_Implementation();
	}
	else
	{
		ServerRequestReturnToMainMenu();
	}
}

void AConquestPlayerController::ToggleSettingsMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (AConquestHUD* ConquestHUD = Cast<AConquestHUD>(GetHUD()))
	{
		ConquestHUD->ToggleSettingsMenu();
	}
}

void AConquestPlayerController::SetMusicEnabled(bool bEnabled)
{
	if (!IsLocalController())
	{
		return;
	}

	bMusicEnabled = bEnabled;
	ApplyMusicSettings();
	SaveUserSettings();
}

void AConquestPlayerController::SetMusicVolumeMultiplier(float NewVolumeMultiplier)
{
	if (!IsLocalController())
	{
		return;
	}

	MusicVolumeMultiplier = FMath::Clamp(NewVolumeMultiplier, 0.0f, 2.0f);
	ApplyMusicSettings();
	SaveUserSettings();
}

void AConquestPlayerController::InitializeBackgroundMusic(const TArray<USoundBase*>& InBackgroundMusicTracks)
{
	if (!IsLocalController())
	{
		ClientInitializeBackgroundMusic(InBackgroundMusicTracks);
		return;
	}

	BackgroundMusicTracks.Reset();
	for (USoundBase* Track : InBackgroundMusicTracks)
	{
		if (Track)
		{
			BackgroundMusicTracks.Add(Track);
		}
	}

	if (
		!CurrentBackgroundMusicTrack ||
		!BackgroundMusicTracks.ContainsByPredicate([this](const TObjectPtr<USoundBase>& Track)
		{
			return Track.Get() == CurrentBackgroundMusicTrack.Get();
		})
	)
	{
		CurrentBackgroundMusicTrack = nullptr;
	}

	ApplyMusicSettings();
	if (bMusicEnabled && BackgroundMusicTracks.Num() > 0 && BackgroundMusicAudioComponent && !BackgroundMusicAudioComponent->IsPlaying())
	{
		PlayNextBackgroundMusicTrack();
	}
}

void AConquestPlayerController::RequestRegenerateFirstTurnMap()
{
	if (HasAuthority())
	{
		ServerRequestRegenerateFirstTurnMap_Implementation();
	}
	else
	{
		ServerRequestRegenerateFirstTurnMap();
	}
}

void AConquestPlayerController::RequestSetCurrentResearch(FName TechId)
{
	if (HasAuthority())
	{
		ServerRequestSetCurrentResearch_Implementation(TechId);
	}
	else
	{
		ServerRequestSetCurrentResearch(TechId);
	}
}

void AConquestPlayerController::RequestAdoptPhilosophy(FName PhilosophyId)
{
	if (HasAuthority())
	{
		ServerRequestAdoptPhilosophy_Implementation(PhilosophyId);
	}
	else
	{
		ServerRequestAdoptPhilosophy(PhilosophyId);
	}
}

void AConquestPlayerController::RequestSetCityProduction(int32 CityId, ECityProductionType ProductionType, FName ProductionId)
{
	if (HasAuthority())
	{
		ServerRequestSetCityProduction_Implementation(CityId, ProductionType, ProductionId);
	}
	else
	{
		ServerRequestSetCityProduction(CityId, ProductionType, ProductionId);
	}
}

void AConquestPlayerController::RequestClaimExpansionTile(int32 CityId, FIntPoint Coord)
{
	if (HasAuthority())
	{
		ServerRequestClaimExpansionTile_Implementation(CityId, Coord);
	}
	else
	{
		ServerRequestClaimExpansionTile(CityId, Coord);
	}
}

void AConquestPlayerController::RequestPurchaseTileImprovement(FIntPoint Coord, FName ImprovementId)
{
	if (HasAuthority())
	{
		ServerRequestPurchaseTileImprovement_Implementation(Coord, ImprovementId);
	}
	else
	{
		ServerRequestPurchaseTileImprovement(Coord, ImprovementId);
	}
}

void AConquestPlayerController::RequestFoundStartingCity(FIntPoint Coord, FName CityName)
{
	if (HasAuthority())
	{
		ServerRequestFoundStartingCity_Implementation(Coord, CityName);
	}
	else
	{
		ServerRequestFoundStartingCity(Coord, CityName);
	}
}

void AConquestPlayerController::RequestMoveUnit(int32 UnitInstanceId, FIntPoint TargetCoord)
{
	if (HasAuthority())
	{
		ServerRequestMoveUnit_Implementation(UnitInstanceId, TargetCoord);
	}
	else
	{
		ServerRequestMoveUnit(UnitInstanceId, TargetCoord);
	}
}

void AConquestPlayerController::RequestUnitAction(int32 UnitInstanceId, FName ActionId)
{
	if (HasAuthority())
	{
		ServerRequestUnitAction_Implementation(UnitInstanceId, ActionId);
	}
	else
	{
		ServerRequestUnitAction(UnitInstanceId, ActionId);
	}
}

void AConquestPlayerController::RequestApplyUnitAugment(int32 UnitInstanceId, FName AugmentId)
{
	if (HasAuthority())
	{
		ServerRequestApplyUnitAugment_Implementation(UnitInstanceId, AugmentId);
	}
	else
	{
		ServerRequestApplyUnitAugment(UnitInstanceId, AugmentId);
	}
}

void AConquestPlayerController::RequestAttackUnit(int32 AttackerUnitInstanceId, int32 DefenderUnitInstanceId)
{
	if (HasAuthority())
	{
		ServerRequestAttackUnit_Implementation(AttackerUnitInstanceId, DefenderUnitInstanceId);
	}
	else
	{
		ServerRequestAttackUnit(AttackerUnitInstanceId, DefenderUnitInstanceId);
	}
}

void AConquestPlayerController::RequestAttackCity(int32 AttackerUnitInstanceId, int32 DefenderCityId)
{
	if (HasAuthority())
	{
		ServerRequestAttackCity_Implementation(AttackerUnitInstanceId, DefenderCityId);
	}
	else
	{
		ServerRequestAttackCity(AttackerUnitInstanceId, DefenderCityId);
	}
}

void AConquestPlayerController::RequestAttackTile(int32 AttackerUnitInstanceId, FIntPoint TargetCoord)
{
	if (HasAuthority())
	{
		ServerRequestAttackTile_Implementation(AttackerUnitInstanceId, TargetCoord);
	}
	else
	{
		ServerRequestAttackTile(AttackerUnitInstanceId, TargetCoord);
	}
}

void AConquestPlayerController::RequestSetLobbySlotCivilisation(
	int32 SlotIndex,
	UConquestCivilisationData* Civilisation
)
{
	if (HasAuthority())
	{
		ServerRequestSetLobbySlotCivilisation_Implementation(SlotIndex, Civilisation);
	}
	else
	{
		ServerRequestSetLobbySlotCivilisation(SlotIndex, Civilisation);
	}
}

void AConquestPlayerController::RequestSetLobbyReady(bool bReady)
{
	if (HasAuthority())
	{
		ServerRequestSetLobbyReady_Implementation(bReady);
	}
	else
	{
		ServerRequestSetLobbyReady(bReady);
	}
}

void AConquestPlayerController::RequestCheatImproveAllResources()
{
	if (HasAuthority())
	{
		ServerRequestCheatImproveAllResources_Implementation();
	}
	else
	{
		ServerRequestCheatImproveAllResources();
	}
}

void AConquestPlayerController::RequestCheatGrantTech(FName TechId)
{
	if (HasAuthority())
	{
		ServerRequestCheatGrantTech_Implementation(TechId);
	}
	else
	{
		ServerRequestCheatGrantTech(TechId);
	}
}

void AConquestPlayerController::RequestCheatGrantPhilosophy(FName PhilosophyId)
{
	if (HasAuthority())
	{
		ServerRequestCheatGrantPhilosophy_Implementation(PhilosophyId);
	}
	else
	{
		ServerRequestCheatGrantPhilosophy(PhilosophyId);
	}
}

void AConquestPlayerController::RequestCheatSpawnUnit(FName UnitId, FIntPoint TileCoord)
{
	if (HasAuthority())
	{
		ServerRequestCheatSpawnUnit_Implementation(UnitId, TileCoord);
	}
	else
	{
		ServerRequestCheatSpawnUnit(UnitId, TileCoord);
	}
}

void AConquestPlayerController::ServerRequestEndTurn_Implementation()
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->EndTurnForPlayer(AssignedPlayerId);
	}
}

void AConquestPlayerController::ClientInitializeBackgroundMusic_Implementation(const TArray<USoundBase*>& InBackgroundMusicTracks)
{
	InitializeBackgroundMusic(InBackgroundMusicTracks);
}

void AConquestPlayerController::HandleBackgroundMusicFinished()
{
	if (IsLocalController() && bMusicEnabled)
	{
		PlayNextBackgroundMusicTrack();
	}
}

void AConquestPlayerController::LoadUserSettings()
{
	if (!IsLocalController())
	{
		return;
	}

	UConquestUserSettingsSaveGame* SettingsSave = Cast<UConquestUserSettingsSaveGame>(
		UGameplayStatics::LoadGameFromSlot(ConquestUserSettingsSaveSlotName, ConquestUserSettingsSaveUserIndex)
	);

	if (!SettingsSave)
	{
		SettingsSave = Cast<UConquestUserSettingsSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UConquestUserSettingsSaveGame::StaticClass())
		);
	}

	if (!SettingsSave)
	{
		return;
	}

	bMusicEnabled = SettingsSave->bMusicEnabled;
	MusicVolumeMultiplier = FMath::Clamp(SettingsSave->MusicVolumeMultiplier, 0.0f, 2.0f);
}

void AConquestPlayerController::SaveUserSettings() const
{
	if (!IsLocalController())
	{
		return;
	}

	UConquestUserSettingsSaveGame* SettingsSave = Cast<UConquestUserSettingsSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UConquestUserSettingsSaveGame::StaticClass())
	);

	if (!SettingsSave)
	{
		return;
	}

	SettingsSave->bMusicEnabled = bMusicEnabled;
	SettingsSave->MusicVolumeMultiplier = FMath::Clamp(MusicVolumeMultiplier, 0.0f, 2.0f);
	UGameplayStatics::SaveGameToSlot(SettingsSave, ConquestUserSettingsSaveSlotName, ConquestUserSettingsSaveUserIndex);
}

void AConquestPlayerController::ApplyMusicSettings()
{
	if (!IsLocalController() || !BackgroundMusicAudioComponent)
	{
		return;
	}

	BackgroundMusicAudioComponent->SetVolumeMultiplier(FMath::Clamp(MusicVolumeMultiplier, 0.0f, 2.0f));

	if (!bMusicEnabled)
	{
		BackgroundMusicAudioComponent->Stop();
		return;
	}

	if (BackgroundMusicTracks.Num() > 0 && !BackgroundMusicAudioComponent->IsPlaying())
	{
		PlayNextBackgroundMusicTrack();
	}
}

void AConquestPlayerController::PlayNextBackgroundMusicTrack()
{
	if (!IsLocalController() || !BackgroundMusicAudioComponent || !bMusicEnabled || BackgroundMusicTracks.Num() <= 0)
	{
		return;
	}

	USoundBase* NextTrack = ChooseNextBackgroundMusicTrack();
	if (!NextTrack)
	{
		return;
	}

	CurrentBackgroundMusicTrack = NextTrack;
	BackgroundMusicAudioComponent->SetSound(NextTrack);
	BackgroundMusicAudioComponent->SetVolumeMultiplier(FMath::Clamp(MusicVolumeMultiplier, 0.0f, 2.0f));
	BackgroundMusicAudioComponent->Play(0.0f);
}

USoundBase* AConquestPlayerController::ChooseNextBackgroundMusicTrack() const
{
	TArray<USoundBase*> ValidTracks;
	for (const TObjectPtr<USoundBase>& Track : BackgroundMusicTracks)
	{
		if (Track)
		{
			ValidTracks.Add(Track.Get());
		}
	}

	if (ValidTracks.Num() <= 0)
	{
		return nullptr;
	}

	if (ValidTracks.Num() == 1)
	{
		return ValidTracks[0];
	}

	USoundBase* SelectedTrack = nullptr;
	for (int32 Attempt = 0; Attempt < 4 && (!SelectedTrack || SelectedTrack == CurrentBackgroundMusicTrack); ++Attempt)
	{
		SelectedTrack = ValidTracks[FMath::RandRange(0, ValidTracks.Num() - 1)];
	}

	if (!SelectedTrack || SelectedTrack == CurrentBackgroundMusicTrack)
	{
		const int32 CurrentIndex = ValidTracks.IndexOfByKey(CurrentBackgroundMusicTrack);
		return ValidTracks[(CurrentIndex + 1) % ValidTracks.Num()];
	}

	return SelectedTrack;
}

void AConquestPlayerController::ServerRequestReturnToMainMenu_Implementation()
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->ResetGameToMainMenu();
	}
}

void AConquestPlayerController::ServerRequestRegenerateFirstTurnMap_Implementation()
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->RegenerateFirstTurnMapForPlayer(AssignedPlayerId);
	}
}

void AConquestPlayerController::ServerRequestSetCurrentResearch_Implementation(FName TechId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->SetCurrentResearchForPlayer(AssignedPlayerId, TechId);
	}
}

void AConquestPlayerController::ServerRequestAdoptPhilosophy_Implementation(FName PhilosophyId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->AdoptPhilosophyForPlayer(AssignedPlayerId, PhilosophyId);
	}
}

void AConquestPlayerController::ServerRequestSetCityProduction_Implementation(
	int32 CityId,
	ECityProductionType ProductionType,
	FName ProductionId
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->SetCityProductionForPlayer(AssignedPlayerId, CityId, ProductionType, ProductionId);
	}
}

void AConquestPlayerController::ServerRequestClaimExpansionTile_Implementation(int32 CityId, FIntPoint Coord)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->ClaimExpansionTileForPlayer(AssignedPlayerId, CityId, Coord);
	}
}

void AConquestPlayerController::ServerRequestPurchaseTileImprovement_Implementation(FIntPoint Coord, FName ImprovementId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->PurchaseTileImprovementForPlayer(AssignedPlayerId, Coord, ImprovementId);
	}
}

void AConquestPlayerController::ServerRequestFoundStartingCity_Implementation(FIntPoint Coord, FName CityName)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->FoundStartingCityForPlayer(AssignedPlayerId, Coord, CityName);
	}
}

void AConquestPlayerController::ServerRequestMoveUnit_Implementation(int32 UnitInstanceId, FIntPoint TargetCoord)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->MoveUnitForPlayer(AssignedPlayerId, UnitInstanceId, TargetCoord);
	}
}

void AConquestPlayerController::ServerRequestUnitAction_Implementation(int32 UnitInstanceId, FName ActionId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->ApplyUnitActionForPlayer(AssignedPlayerId, UnitInstanceId, ActionId);
	}
}

void AConquestPlayerController::ServerRequestApplyUnitAugment_Implementation(int32 UnitInstanceId, FName AugmentId)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->ApplyUnitAugmentForPlayer(AssignedPlayerId, UnitInstanceId, AugmentId);
	}
}

void AConquestPlayerController::ServerRequestAttackUnit_Implementation(
	int32 AttackerUnitInstanceId,
	int32 DefenderUnitInstanceId
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->AttackUnitForPlayer(AssignedPlayerId, AttackerUnitInstanceId, DefenderUnitInstanceId);
	}
}

void AConquestPlayerController::ServerRequestAttackCity_Implementation(
	int32 AttackerUnitInstanceId,
	int32 DefenderCityId
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->AttackCityForPlayer(AssignedPlayerId, AttackerUnitInstanceId, DefenderCityId);
	}
}

void AConquestPlayerController::ServerRequestAttackTile_Implementation(
	int32 AttackerUnitInstanceId,
	FIntPoint TargetCoord
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->AttackTileForPlayer(AssignedPlayerId, AttackerUnitInstanceId, TargetCoord);
	}
}

void AConquestPlayerController::ServerRequestSetLobbySlotCivilisation_Implementation(
	int32 SlotIndex,
	UConquestCivilisationData* Civilisation
)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->SetLobbySlotCivilisationForPlayer(AssignedPlayerId, SlotIndex, Civilisation);
	}
}

void AConquestPlayerController::ServerRequestSetLobbyReady_Implementation(bool bReady)
{
	if (AConquestGameMode* ConquestGM = GetConquestGameMode(this))
	{
		ConquestGM->SetLobbyReadyForPlayer(AssignedPlayerId, bReady);
	}
}

void AConquestPlayerController::ServerRequestCheatImproveAllResources_Implementation()
{
	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS || !ConquestGS->ActiveGridActor)
	{
		return;
	}

	FHexGridModel& GridModel = ConquestGS->ActiveGridActor->GetMutableGridModel();
	int32 ImprovedCount = 0;
	TArray<const FHexImprovementDefinition*> AllImprovements;
	GridModel.GetAllImprovementDefinitions(AllImprovements);

	auto FindResourceMatchedImprovement = [&AllImprovements](FName ResourceId) -> const FHexImprovementDefinition*
	{
		if (ResourceId.IsNone())
		{
			return nullptr;
		}

		for (const FHexImprovementDefinition* Improvement : AllImprovements)
		{
			if (
				Improvement &&
				Improvement->bRequiresResource &&
				Improvement->RequiredResources.Contains(ResourceId)
			)
			{
				return Improvement;
			}
		}

		for (const FHexImprovementDefinition* Improvement : AllImprovements)
		{
			if (Improvement && Improvement->RequiredResources.Contains(ResourceId))
			{
				return Improvement;
			}
		}

		return nullptr;
	};

	for (int32 R = 0; R < GridModel.GetGridHeight(); ++R)
	{
		for (int32 Q = 0; Q < GridModel.GetGridWidth(); ++Q)
		{
			const FHexTileData* Tile = GridModel.GetTile(Q, R);
			if (!Tile || !Tile->Resource.HasResource())
			{
				continue;
			}

			if (!Tile->ImprovementId.IsNone())
			{
				++ImprovedCount;
				continue;
			}

			TArray<const FHexImprovementDefinition*> PossibleImprovements;
			GridModel.GetPossibleImprovementsForTile(Q, R, PossibleImprovements);

			const FHexImprovementDefinition* BestImprovement = nullptr;
			for (const FHexImprovementDefinition* Improvement : PossibleImprovements)
			{
				if (
					Improvement &&
					Improvement->bRequiresResource &&
					Improvement->RequiredResources.Contains(Tile->Resource.ResourceId)
				)
				{
					BestImprovement = Improvement;
					break;
				}
			}

			if (!BestImprovement && PossibleImprovements.Num() > 0)
			{
				BestImprovement = PossibleImprovements[0];
			}

			if (!BestImprovement)
			{
				BestImprovement = FindResourceMatchedImprovement(Tile->Resource.ResourceId);
			}

			if (!BestImprovement)
			{
				continue;
			}

			const bool bAppliedNormally = ConquestGS->ActiveGridActor->SetTileImprovement(Q, R, BestImprovement->ImprovementId);
			const bool bAppliedUnchecked = bAppliedNormally || GridModel.SetTileImprovementUnchecked(Q, R, BestImprovement->ImprovementId);
			if (bAppliedUnchecked)
			{
				++ImprovedCount;
			}
		}
	}

	if (ImprovedCount > 0)
	{
		ConquestGS->ActiveGridActor->RefreshPlacedTileVisualMeshes();
		ConquestGS->BroadcastStateChangedWithVisuals(EConquestStateVisualDirtyFlags::TileImprovements | EConquestStateVisualDirtyFlags::Cities);
	}
}

void AConquestPlayerController::ServerRequestCheatGrantTech_Implementation(FName TechId)
{
	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS || !ConquestGS->ContentManager || TechId.IsNone() || !ConquestGS->ContentManager->FindTech(TechId))
	{
		return;
	}

	FConquestPlayerEmpireState& PlayerEmpire = ConquestGS->GetPlayerEmpireMutable(AssignedPlayerId);
	PlayerEmpire.ResearchedTechIds.AddUnique(TechId);
	PlayerEmpire.ResearchProgressCache.RemoveAll([TechId](const FConquestResearchProgressCacheEntry& Entry)
	{
		return Entry.TechId == TechId;
	});
	if (PlayerEmpire.CurrentResearchId == TechId)
	{
		PlayerEmpire.CurrentResearchId = NAME_None;
		PlayerEmpire.CurrentResearchProgress = 0.0f;
	}

	if (ConquestGS->CityManager)
	{
		ConquestGS->CityManager->RecalculateEmpireYields(AssignedPlayerId);
	}

	if (ConquestGS->TechManager)
	{
		ConquestGS->TechManager->OnResearchChanged.Broadcast();
	}

	ConquestGS->BroadcastStateChangedWithVisuals(EConquestStateVisualDirtyFlags::None);
}

void AConquestPlayerController::ServerRequestCheatGrantPhilosophy_Implementation(FName PhilosophyId)
{
	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS || !ConquestGS->ContentManager || PhilosophyId.IsNone() || !ConquestGS->ContentManager->FindPhilosophy(PhilosophyId))
	{
		return;
	}

	FConquestPlayerEmpireState& PlayerEmpire = ConquestGS->GetPlayerEmpireMutable(AssignedPlayerId);
	PlayerEmpire.AdoptedPhilosophyIds.AddUnique(PhilosophyId);
	PlayerEmpire.AdoptedPhilosophyTenets = PlayerEmpire.AdoptedPhilosophyIds.Num();

	if (ConquestGS->CityManager)
	{
		ConquestGS->CityManager->RecalculateEmpireYields(AssignedPlayerId);
	}

	if (ConquestGS->PhilosophyManager)
	{
		ConquestGS->PhilosophyManager->OnPhilosophiesChanged.Broadcast();
	}

	ConquestGS->BroadcastStateChangedWithVisuals(EConquestStateVisualDirtyFlags::None);
}

void AConquestPlayerController::ServerRequestCheatSpawnUnit_Implementation(FName UnitId, FIntPoint TileCoord)
{
	AConquestGameState* ConquestGS = GetWorld() ? GetWorld()->GetGameState<AConquestGameState>() : nullptr;
	if (!ConquestGS || !ConquestGS->CityManager)
	{
		return;
	}

	ConquestGS->CityManager->CheatSpawnUnitForPlayerAtTile(AssignedPlayerId, UnitId, TileCoord);
}
