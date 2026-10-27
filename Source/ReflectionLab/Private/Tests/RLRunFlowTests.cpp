#if WITH_DEV_AUTOMATION_TESTS
#include "Framework/GameMode/RLGameModeBase.h"
#include "Framework/RLArenaExperimentComponent.h"
#include "Framework/RLWaveDefinitionResolver.h"
#include "Data/RLArenaExperimentDataAsset.h"

#include "Combat/RLProjectile.h"
#include "Combat/RLExpandingRingAttack.h"
#include "Data/RLDifficultyScheduleDataAsset.h"
#include "Data/RLRunDefinitionDataAsset.h"
#include "Data/RLRunRewardDataAsset.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Enemies/RLEnemySpawner.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/PlayerController/RLPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RLPlayerCharacter.h"
#include "Player/Components/RLHealthComponent.h"
#include "Player/Components/RLRunRewardComponent.h"
#include "Player/Components/RLParryProgressionComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "UI/RLRunStatusWidget.h"
#include "UI/RLRewardSelectionWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLTutorialRewardTest, "ReflectionLab.Run.TutorialReward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLTutorialRewardTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Settings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARLGameModeBase* Mode = World->SpawnActor<ARLGameModeBase>();
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ARLPlayerCharacter* Player = World->SpawnActor<ARLPlayerCharacter>(
		ARLPlayerCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (TestNotNull(TEXT("Game mode"), Mode) && TestNotNull(TEXT("Player controller"), Controller)
		&& TestNotNull(TEXT("Player"), Player))
	{
		Controller->Possess(Player);
		URLRunRewardComponent* TestRewards = Player->GetRunRewardComponent();
		TestRewards->Initialize(Player->GetHealthComponent());
		auto ApplyTestReward = [Mode, TestRewards](ERLRunRewardType Type, float Amount)
		{
			URLRunRewardDataAsset* Reward = NewObject<URLRunRewardDataAsset>(Mode);
			Reward->RewardType = Type;
			Reward->Amount = Amount;
			return TestRewards->TryApplyReward(Reward);
		};
		// This isolated world has not begun play, so register its player explicitly.
		World->AddController(Controller);
		TestTrue(TEXT("Player available to gameplay lookup"),
			UGameplayStatics::GetPlayerCharacter(Mode, 0) == Player);
		Mode->RunDefinition = NewObject<URLRunDefinitionDataAsset>(Mode);
		for (ERLRunRewardType Type : {ERLRunRewardType::WiderArc,
			ERLRunRewardType::ExtendedRange, ERLRunRewardType::PiercingReturn})
		{
			URLRunRewardDataAsset* Reward = NewObject<URLRunRewardDataAsset>(Mode->RunDefinition);
			Reward->RewardType = Type;
			Reward->Amount = 1.0f;
			Reward->Title = FText::FromString(TEXT("Test reward"));
			Mode->RunDefinition->RewardPool.Add(Reward);
		}
		Mode->RunDefinition->Rounds.AddDefaulted_GetRef().bIsTutorial = true;
		Mode->CurrentRoundIndex = 0;
		Mode->bTutorialOnlyMode = true;
		Mode->RunState = ERLRunState::PlayingRound;
		Mode->FinishRound();
		TestTrue(TEXT("Tutorial ends in reward selection"), Mode->IsChoosingReward());
		TestEqual(TEXT("Three practice cards"), Mode->GetRewardChoiceCount(), 3);
		TestFalse(TEXT("Invalid reward rejected"), Mode->SelectReward(-1));
		// Drive the real nested widget through Slate ticks. The child only reports
		// selection; the parent retains authority to apply it through the mode.
		UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
		World->SetGameInstance(Instance);
		ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
		LocalPlayer->PlayerController = Controller;
		Controller->Player = LocalPlayer;
		URLRunStatusWidget* HUD = CreateWidget<URLRunStatusWidget>(Controller);
		if (TestNotNull(TEXT("Run HUD"), HUD))
		{
			HUD->BindToGameMode(Mode);
			URLRewardSelectionWidget* Selection = nullptr;
			HUD->WidgetTree->ForEachWidget([&Selection](UWidget* Widget)
			{
				if (auto* Child = Cast<URLRewardSelectionWidget>(Widget)) { Selection = Child; }
			});
			if (TestNotNull(TEXT("HUD owns a separate reward widget"), Selection))
			{
				UButton* Button = Cast<UButton>(Selection->WidgetTree->FindWidget(TEXT("RewardChoiceButton0")));
				if (TestNotNull(TEXT("Reward button built"), Button))
				{
					const TSharedRef<SWidget> Slate = Selection->TakeWidget();
					Button->OnClicked.Broadcast();
					Slate->Tick(FGeometry(), 0, 0.2f);
					TestTrue(TEXT("Reward waits for selection animation"), Mode->IsChoosingReward());
					Button->OnClicked.Broadcast();
					Slate->Tick(FGeometry(), 0.2, 0.5f);
					TestTrue(TEXT("Animated selection completes tutorial through parent"), Mode->GetRunState() == ERLRunState::TutorialCompleted);
					HUD->BindToGameMode(nullptr);
					Button->OnClicked.Broadcast();
					Slate->Tick(FGeometry(), 0.7, 1.0f);
					TestTrue(TEXT("Unbound reward UI is hidden"), Selection->GetVisibility() == ESlateVisibility::Collapsed);
				}
			}
		}

		TestTrue(TEXT("Selection completes tutorial"), Mode->GetRunState() == ERLRunState::TutorialCompleted);
		TestFalse(TEXT("Duplicate selection rejected"), Mode->SelectReward(0));
		TestRewards->ResetRunRewards();
		const float BaseHealth = Player->GetMaxHealth();
		TestEqual(TEXT("Default max HP"), BaseHealth, 10.0f);
		Player->GetHealthComponent()->InitializeHealth(Player->GetMaxHealth());
		Player->GetHealthComponent()->ApplyDamage(Player->GetMaxHealth() - 5.0f);
		TestTrue(TEXT("Vitality applies through component"), ApplyTestReward(ERLRunRewardType::Vitality, 2.0f));
		TestEqual(TEXT("Vitality increases max HP"), Player->GetMaxHealth(), BaseHealth + 2.0f);
		TestEqual(TEXT("Vitality restores HP"), Player->GetCurrentHealth(), 7.0f);
		auto RegisterSuccessfulParry = [Player](int32 Count, bool bPerfect, bool bClose, bool bOverdrive)
		{
			const FRLParryResult Result{Count, bPerfect, bClose, bOverdrive};
			Player->GetRunRewardComponent()->ApplyPerfectRecovery(Result);
			Player->GetParryProgressionComponent()->RegisterSuccess(Result);
		};
		TestTrue(TEXT("Healing reward applies"), ApplyTestReward(ERLRunRewardType::PerfectRecovery, 1.0f));
		RegisterSuccessfulParry(3, true, false, false);
		TestEqual(TEXT("Multi perfect parry heals once"), Player->GetCurrentHealth(), 8.0f);
		RegisterSuccessfulParry(1, false, false, false);
		TestEqual(TEXT("Normal parry does not heal"), Player->GetCurrentHealth(), 8.0f);
		TestTrue(TEXT("Healing reward stacks"), ApplyTestReward(ERLRunRewardType::PerfectRecovery, 1.0f));
		RegisterSuccessfulParry(1, true, false, false);
		TestEqual(TEXT("Second recovery reward heals two HP"), Player->GetCurrentHealth(), 10.0f);
		TestTrue(TEXT("Healing reward stacks again"), ApplyTestReward(ERLRunRewardType::PerfectRecovery, 1.0f));
		Player->GetHealthComponent()->ApplyDamage(Player->GetCurrentHealth() - 5.0f);
		RegisterSuccessfulParry(1, true, false, false);
		TestEqual(TEXT("Third recovery reward heals three HP"), Player->GetCurrentHealth(), 8.0f);
		Player->RestoreHealth(Player->GetMaxHealth());
		RegisterSuccessfulParry(1, true, false, false);
		TestEqual(TEXT("Healing respects max HP"), Player->GetCurrentHealth(), Player->GetMaxHealth());
		TestRewards->ResetRunRewards();
		TestEqual(TEXT("Reset restores base max HP"), Player->GetMaxHealth(), BaseHealth);
		TestFalse(TEXT("Reset clears recovery reward"), Player->HasPerfectRecoveryReward());
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLShortRoundsTest, "ReflectionLab.Run.ShortRounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLShortRoundsTest::RunTest(const FString& Parameters)
{
	ARLGameModeBase* Mode = GetMutableDefault<ARLGameModeBase>();
	URLRunDefinitionDataAsset* Original = Mode->RunDefinition;
	const bool bWasPrepared = Mode->bShortRoundsPrepared;
	Mode->RunDefinition = DuplicateObject<URLRunDefinitionDataAsset>(Original, GetTransientPackage());
	Mode->bShortRoundsPrepared = false;
	const TArray<FRLRoundDefinition> Before = Mode->RunDefinition->Rounds;
	const bool bAlreadyExplicit = Mode->RunDefinition->bExplicitRoundLayout;
	Mode->PrepareShortRounds();
	int32 OutputIndex = 0;
	int32 MainRounds = 0;
	for (const FRLRoundDefinition& Round : Before)
	{
		const int32 Parts = bAlreadyExplicit || Round.bIsTutorial || Round.DifficultySchedule->Waves.Num() < 2 ? 1 : 2;
		int32 WaveIndex = 0;
		for (int32 Part = 0; Part < Parts; ++Part)
		{
			if (!TestTrue(TEXT("Split round exists"), Mode->RunDefinition->Rounds.IsValidIndex(OutputIndex)))
			{
				break;
			}
			const FRLRoundDefinition& Result = Mode->RunDefinition->Rounds[OutputIndex++];
			MainRounds += Result.bIsTutorial ? 0 : 1;
			for (const FRLWaveDefinition& Wave : Result.DifficultySchedule->Waves)
			{
				TestEqual(TEXT("Wave order preserved"), Wave.WaveName,
					Round.DifficultySchedule->Waves[WaveIndex++].WaveName);
			}
		}
		TestEqual(TEXT("All waves preserved"), WaveIndex, Round.DifficultySchedule->Waves.Num());
	}
	TestEqual(TEXT("Ten main rounds"), MainRounds, 10);
	const int32 Count = Mode->RunDefinition->Rounds.Num();
	Mode->PrepareShortRounds();
	TestEqual(TEXT("Restart does not split again"), Mode->RunDefinition->Rounds.Num(), Count);
	const bool bWasTutorialOnly = Mode->bTutorialOnlyMode;
	Mode->bTutorialOnlyMode = false;
	Mode->RunDefinition->StartingRoundNumber = 0;
	const int32 NormalStart = Mode->ResolveStartingRoundIndex();
	Mode->RunDefinition->StartingRoundNumber = 10;
	const int32 TenthRoundIndex = Mode->ResolveStartingRoundIndex();
	int32 MainRoundNumber = 0;
	for (int32 Index = 0; Index <= TenthRoundIndex; ++Index)
	{
		MainRoundNumber += Mode->RunDefinition->Rounds[Index].bIsTutorial ? 0 : 1;
	}
	TestEqual(TEXT("Start override selects displayed round ten after splitting"), MainRoundNumber, 10);
	Mode->bTutorialOnlyMode = true;
	const int32 TutorialIndex = Mode->RunDefinition->Rounds.IndexOfByPredicate(
		[](const FRLRoundDefinition& Round) { return Round.bIsTutorial; });
	TestEqual(TEXT("Tutorial mode ignores the main-round override"), Mode->ResolveStartingRoundIndex(), TutorialIndex);
	Mode->bTutorialOnlyMode = false;
	Mode->RunDefinition->StartingRoundNumber = 999;
	AddExpectedError(TEXT("Starting round 999 does not exist"), EAutomationExpectedErrorFlags::Contains, 1);
	TestEqual(TEXT("Invalid start override falls back to normal entry"), Mode->ResolveStartingRoundIndex(), NormalStart);
	Mode->bTutorialOnlyMode = bWasTutorialOnly;
	Mode->RunDefinition = Original;
	Mode->bShortRoundsPrepared = bWasPrepared;
	return true;
}
#endif
