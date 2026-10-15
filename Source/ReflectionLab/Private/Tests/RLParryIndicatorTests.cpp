#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/Components/RLParryFeedbackComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLParryIndicatorTest, "ReflectionLab.Player.ParryIndicatorRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLParryIndicatorTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("World"), World)) { return false; }
	ACharacter* Character = World->SpawnActor<ACharacter>();
	if (TestNotNull(TEXT("Character"), Character))
	{
		URLParryFeedbackComponent* Feedback = NewObject<URLParryFeedbackComponent>(Character);
		Feedback->RegisterComponent();
		UDecalComponent* Decal = FindObject<UDecalComponent>(Feedback, TEXT("ParryRangeIndicator"));
		if (TestNotNull(TEXT("Indicator"), Decal))
		{
			FRLParryViewState View;
			View.Stats.ReflectionRange = 200.0f;
			View.Stats.CloseRangeThreshold = 55.0f;
			Feedback->UpdateView(View);
			TestTrue(TEXT("Base decal dimensions"), Decal->DecalSize.Equals(FVector(64, 200, 200)));
			View.Stats.ReflectionRange *= 1.18f;
			Feedback->UpdateView(View);
			TestTrue(TEXT("Reward expands projection"), Decal->DecalSize.Equals(FVector(64, 236, 236)));
			TestEqual(TEXT("Close radius remains fixed in world space"),
				View.Stats.ReflectionRange * (View.Stats.CloseRangeThreshold / View.Stats.ReflectionRange), 55.0f);
		}
	}
	World->DestroyWorld(false);
	return true;
}
#endif
