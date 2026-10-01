#include "Animation/RLEnemyAnimInstance.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemies/RLEnemyCharacter.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Enemies/RLEnemyPoolSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#endif

namespace
{
	struct FRLEnemyBlendNode : FAnimNode_TwoWayBlend
	{
		FRLEnemyBlendNode()
		{
			AlphaInputType = EAnimAlphaInputType::Bool;
			bAlphaBoolEnabled = false;
			bResetChildOnActivation = true;
		}
	};
}

// Native pose graph: looping Idle -> blend -> one-shot Shoot. UObject data is
// copied on the game thread; pose evaluation only touches the proxy's nodes.
struct FRLEnemyAnimInstanceProxy : FAnimInstanceProxy
{
	explicit FRLEnemyAnimInstanceProxy(UAnimInstance* Instance)
		: FAnimInstanceProxy(Instance)
	{
		Blend.A.SetLinkNode(&IdlePlayer);
		Blend.B.SetLinkNode(&ShootPlayer);
	}

	virtual void Initialize(UAnimInstance* Instance) override
	{
		FAnimInstanceProxy::Initialize(Instance);
		const URLEnemyAnimInstance* EnemyInstance = CastChecked<URLEnemyAnimInstance>(Instance);
		IdlePlayer.SetSequence(EnemyInstance->IdleAnimation);
		IdlePlayer.SetLoopAnimation(true);
		ShootPlayer.SetSequence(EnemyInstance->ShootAnimation);
		ShootPlayer.SetLoopAnimation(false);
		ShootPlayer.SetPlayRate(EnemyInstance->ShootPlayRate);
		Blend.AlphaBoolBlend.BlendInTime = EnemyInstance->BlendTime;
		Blend.AlphaBoolBlend.BlendOutTime = EnemyInstance->BlendTime;
	}

	virtual FAnimNode_Base* GetCustomRootNode() override { return &Blend; }

	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override
	{
		OutNodes.Add(&IdlePlayer);
		OutNodes.Add(&ShootPlayer);
		OutNodes.Add(&Blend);
	}

	virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
	{
		FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
		const URLEnemyAnimInstance* EnemyInstance = CastChecked<URLEnemyAnimInstance>(Instance);
		if (LastResetSerial != EnemyInstance->ResetSerial)
		{
			IdlePlayer.SetAccumulatedTime(0.0f);
			ShootPlayer.SetAccumulatedTime(0.0f);
			LastResetSerial = EnemyInstance->ResetSerial;
		}
		if (LastShotSerial != EnemyInstance->ShotSerial)
		{
			ShootPlayer.SetAccumulatedTime(0.0f);
			LastShotSerial = EnemyInstance->ShotSerial;
		}
		Blend.bAlphaBoolEnabled = EnemyInstance->bIsShooting;
		ShootPlayer.SetPlayRate(EnemyInstance->ShootPlayRate);
	}

	FAnimNode_SequencePlayer_Standalone IdlePlayer;
	FAnimNode_SequencePlayer_Standalone ShootPlayer;
	FRLEnemyBlendNode Blend;
	uint32 LastShotSerial = 0;
	uint32 LastResetSerial = 0;
};

URLEnemyAnimInstance::URLEnemyAnimInstance()
{
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleFinder(
		TEXT("/Game/ReflectionLab/Gameplay/Enemies/Animations/A_Enemy_Idle.A_Enemy_Idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> ShootFinder(
		TEXT("/Game/ReflectionLab/Gameplay/Enemies/Animations/A_Enemy_Shoot.A_Enemy_Shoot"));
	IdleAnimation = IdleFinder.Object;
	ShootAnimation = ShootFinder.Object;
}

void URLEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ResetCombatAnimation();
}

void URLEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const ARLEnemyCharacter* Enemy = Cast<ARLEnemyCharacter>(TryGetPawnOwner());
	ShootTimeRemaining = Enemy && Enemy->IsPoolActive()
		? FMath::Max(0.0f, ShootTimeRemaining - DeltaSeconds) : 0.0f;
	bIsShooting = ShootTimeRemaining > 0.0f;
}

void URLEnemyAnimInstance::PlayShootAnimation()
{
	if (!ShootAnimation)
	{
		return;
	}
	ShootTimeRemaining = ShootAnimation->GetPlayLength() / FMath::Max(0.1f, ShootPlayRate);
	bIsShooting = ShootTimeRemaining > 0.0f;
	++ShotSerial;
}

void URLEnemyAnimInstance::ResetCombatAnimation()
{
	ShootTimeRemaining = 0.0f;
	bIsShooting = false;
	++ResetSerial;
}

FAnimInstanceProxy* URLEnemyAnimInstance::CreateAnimInstanceProxy()
{
	return new FRLEnemyAnimInstanceProxy(this);
}

void URLEnemyAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete static_cast<FRLEnemyAnimInstanceProxy*>(InProxy);
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRLEnemyAnimationTest,
	"ReflectionLab.Animation.Enemy.IdleShootAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRLEnemyAnimationTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues WorldSettings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &WorldSettings);
	if (!TestNotNull(TEXT("Preview world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	for (const TCHAR* Path : {
		TEXT("/Game/ReflectionLab/Gameplay/Enemies/BP_RLEnemyCharacter.BP_RLEnemyCharacter_C"),
		TEXT("/Game/ReflectionLab/Gameplay/Enemies/BP_RLEnemyBurstShooter.BP_RLEnemyBurstShooter_C") })
	{
		UClass* EnemyClass = LoadClass<ARLEnemyCharacter>(nullptr, Path);
		if (!TestNotNull(TEXT("Enemy blueprint class"), EnemyClass))
		{
			continue;
		}
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ARLEnemyCharacter* Enemy = World->SpawnActor<ARLEnemyCharacter>(
			EnemyClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
		if (!TestNotNull(TEXT("Spawned enemy"), Enemy))
		{
			continue;
		}
		USkeletalMeshComponent* Mesh = Enemy->GetMesh();
		URLEnemyAnimInstance* Instance = Cast<URLEnemyAnimInstance>(Mesh->GetAnimInstance());
		if (!TestNotNull(TEXT("Enemy uses the native anim instance"), Instance))
		{
			continue;
		}
		Mesh->TickAnimation(0.2f, false);
		Mesh->RefreshBoneTransforms();
		const int32 HandIndex = Mesh->GetBoneIndex(TEXT("hand_r"));
		TestTrue(TEXT("Manny hand bone exists"), HandIndex != INDEX_NONE);
		const FTransform IdleHand = Mesh->GetBoneTransform(HandIndex);
		TestFalse(TEXT("Enemy starts in Idle"), Instance->IsShooting());
		Instance->PlayShootAnimation();
		TestTrue(TEXT("Shot activates Shoot"), Instance->IsShooting());
		for (int32 Frame = 0; Frame < 3; ++Frame)
		{
			Mesh->TickAnimation(0.1f, false);
			Mesh->RefreshBoneTransforms();
		}
		TestFalse(TEXT("Shoot evaluates a different hand pose"),
			IdleHand.Equals(Mesh->GetBoneTransform(HandIndex), 0.001f));
		Instance->NativeUpdateAnimation(120.0f);
		TestFalse(TEXT("Completed shot returns to Idle"), Instance->IsShooting());
		Instance->PlayShootAnimation();
		Enemy->ReturnToPool();
		TestFalse(TEXT("Returned enemy is inactive"), Enemy->IsPoolActive());
		TestFalse(TEXT("Pool return clears Shoot"), Instance->IsShooting());
		ARLEnemyCharacter* ReusedEnemy = World->GetSubsystem<URLEnemyPoolSubsystem>()
			->AcquireEnemy(EnemyClass, FTransform::Identity);
		TestTrue(TEXT("Pool reuses the same enemy"), ReusedEnemy == Enemy);
		TestFalse(TEXT("Reused enemy starts in Idle"), Instance->IsShooting());
		Enemy->Destroy();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
