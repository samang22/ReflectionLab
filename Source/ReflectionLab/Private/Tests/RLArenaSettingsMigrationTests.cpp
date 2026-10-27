#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Data/RLArenaExperimentDataAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLArenaSettingsMigrationTest, "ReflectionLab.Experiments.SettingsMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRLArenaSettingsMigrationTest::RunTest(const FString& Parameters)
{
	auto* Data = NewObject<URLArenaExperimentDataAsset>();
	TestTrue(TEXT("New assets start with grouped settings"), Data->bGroupedSettings);
	Data->bGroupedSettings = false;
	Data->ShotBarrierVisualHeight = 120;
	Data->PhaseDuration = 8;
	Data->WarningDuration = 2.5f;
	Data->BaselineHostileSpeedMultiplier = 0.65f;
	Data->FieldCenters = {FVector2D(123, 456)};
	Data->ZoneDamage = 3;
	Data->SupportShieldColor = FLinearColor::Yellow;
	Data->MigrateLegacySettings();
	TestTrue(TEXT("Migration flag set"), Data->bGroupedSettings);
	for (const FName GroupName : {TEXT("Crossfire"), TEXT("FieldLayout"), TEXT("Acceleration"), TEXT("Refraction"), TEXT("Support"), TEXT("Zones"), TEXT("Mirror")})
	{
		const auto* Group = FindFProperty<FStructProperty>(Data->GetClass(), GroupName);
		if (!TestNotNull(TEXT("Grouped struct property exists"), Group)) { continue; }
		const void* Values = Group->ContainerPtrToValuePtr<void>(Data);
		for (TFieldIterator<FProperty> It(Group->Struct); It; ++It)
		{
			const auto* Legacy = FindFProperty<FProperty>(Data->GetClass(), It->GetFName());
			if (!Legacy) { continue; } // New, non-legacy settings keep their authored defaults.
			TestTrue(*FString::Printf(TEXT("%s.%s preserves legacy value"), *GroupName.ToString(), *It->GetName()),
				It->Identical(It->ContainerPtrToValuePtr<void>(Values), Legacy->ContainerPtrToValuePtr<void>(Data)));
			TestFalse(TEXT("Legacy property hidden from editor"), Legacy->HasAnyPropertyFlags(CPF_Edit));
		}
#if WITH_EDITORONLY_DATA
		TestTrue(TEXT("Only matching type displays the group"), Group->HasMetaData(TEXT("EditConditionHides")));
#endif
	}
	TestEqual(TEXT("Mirror height preserved"), Data->Mirror.ShotBarrierVisualHeight, 120.0f);
	TestEqual(TEXT("Zone timing separated"), Data->Zones.PhaseDuration, 8.0f);
	Data->Mirror.ShotBarrierVisualHeight = 130;
	Data->MigrateLegacySettings();
	TestEqual(TEXT("Repeated migration cannot overwrite new tuning"), Data->Mirror.ShotBarrierVisualHeight, 130.0f);
	for (const FName Name : {TEXT("MirrorDistance"), TEXT("MirrorHalfWidth")})
	{
		const auto* Legacy = FindFProperty<FProperty>(Data->GetClass(), Name);
		TestTrue(TEXT("Retired dimensions remain hidden"), Legacy && !Legacy->HasAnyPropertyFlags(CPF_Edit));
	}
	return true;
}
#endif
