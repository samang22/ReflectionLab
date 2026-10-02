#include "Combat/RLExpandingRingAttack.h"

#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Framework/GameMode/RLGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/RLPlayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARLExpandingRingAttack::ARLExpandingRingAttack()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	RingVisual = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RingVisual"));
	SetRootComponent(RingVisual);
	RingVisual->SetMobility(EComponentMobility::Movable);
	RingVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingVisual->SetGenerateOverlapEvents(false);
	RingVisual->SetCanEverAffectNavigation(false);
	RingVisual->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	RingVisual->SetStaticMesh(Mesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Game/ReflectionLab/Art/Materials/Projectiles/MI_Projectile_Hostile.MI_Projectile_Hostile"));
	RingVisual->SetMaterial(0, Material.Object);
}

void ARLExpandingRingAttack::BeginPlay()
{
	Super::BeginPlay();
	// The projectile material is translucent. Nanite would replace it with an
	// incompatible fallback and prevent the opacity fade from being rendered.
	RingVisual->SetForceDisableNanite(true);
	// This actor is anchored at its spawn location; it does not follow its shooter.
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorRotation(FRotator::ZeroRotator);
	SetActorScale3D(FVector::OneVector);
	RingMaterial = RingVisual->CreateDynamicMaterialInstance(0);
	if (RingMaterial)
	{
		RingMaterial->SetVectorParameterValue(TEXT("ProjectileColor"), FLinearColor(1.0f, 0.025f, 0.005f));
		RingMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), 20.0f);
		RingMaterial->SetScalarParameterValue(TEXT("FadeOpacity"), 1.0f);
	}
	RingVisual->SetVisibility(false);
	if (bAutoStart) { StartAttack(); }
}

bool ARLExpandingRingAttack::IsCombatActive() const
{
	const ARLGameModeBase* GameMode = Cast<ARLGameModeBase>(UGameplayStatics::GetGameMode(this));
	return !GameMode || GameMode->GetRunState() == ERLRunState::PlayingRound;
}

bool ARLExpandingRingAttack::StartAttack()
{
	if (bActive || bFadingOut || !HasActorBegunPlay() || !IsCombatActive()) { return false; }
	ActiveSettings = AttackData ? AttackData->Settings : FRLRingAttackSettings();
	if (!FMath::IsFinite(ActiveSettings.StartRadius) ||
		!FMath::IsFinite(ActiveSettings.ExpansionSpeed) || !FMath::IsFinite(ActiveSettings.RingThickness) ||
		!FMath::IsFinite(ActiveSettings.AttackHeight) || !FMath::IsFinite(ActiveSettings.Damage) ||
		!FMath::IsFinite(ActiveSettings.ActiveDuration) || !FMath::IsFinite(ActiveSettings.FadeOutDuration))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid ring attack settings on %s."), *GetName());
		Destroy();
		return false;
	}
	ActiveSettings.StartRadius = FMath::Max(0.0f, ActiveSettings.StartRadius);
	ActiveSettings.ExpansionSpeed = FMath::Max(1.0f, ActiveSettings.ExpansionSpeed);
	ActiveSettings.RingThickness = FMath::Max(1.0f, ActiveSettings.RingThickness);
	ActiveSettings.AttackHeight = FMath::Max(1.0f, ActiveSettings.AttackHeight);
	ActiveSettings.Damage = FMath::Max(0.0f, ActiveSettings.Damage);
	ActiveSettings.ActiveDuration = FMath::Max(0.1f, ActiveSettings.ActiveDuration);
	ActiveSettings.FadeOutDuration = FMath::Max(0.01f, ActiveSettings.FadeOutDuration);
	ActiveElapsedTime = 0.0f;
	FadeOutElapsedTime = 0.0f;
	// Back up Tick-driven fading with an engine timer so the actor cannot linger
	// indefinitely if actor ticking is disabled by an external gameplay system.
	SetLifeSpan(ActiveSettings.ActiveDuration + ActiveSettings.FadeOutDuration);
	CurrentRadius = ActiveSettings.StartRadius;
	bActive = true;
	bPlayerHit = false;
	bPlayerDodged = false;
	TrackedPlayer = Cast<ARLPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (TrackedPlayer.IsValid()) { PreviousPlayerLocation = TrackedPlayer->GetActorLocation(); }
	RingVisual->ClearInstances();
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		RingVisual->AddInstance(FTransform::Identity);
	}
	RingVisual->SetVisibility(true);
	UpdateVisual();
	CheckPlayerHit(CurrentRadius);
	return true;
}

void ARLExpandingRingAttack::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float DeltaTime = FMath::Max(0.0f, DeltaSeconds);
	if (bFadingOut)
	{
		FadeOutElapsedTime += DeltaTime;
		const float Alpha = FMath::Clamp(FadeOutElapsedTime / ActiveSettings.FadeOutDuration, 0.0f, 1.0f);
		if (RingMaterial)
		{
			RingMaterial->SetScalarParameterValue(TEXT("FadeOpacity"), 1.0f - Alpha);
			RingMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), 20.0f * (1.0f - Alpha));
		}
		if (Alpha >= 1.0f) { Destroy(); }
		return;
	}
	if (!bActive)
	{
		if (bAutoStart) { StartAttack(); }
		return;
	}
	if (!IsCombatActive())
	{
		BeginFadeOut();
		return;
	}
	ActiveElapsedTime += DeltaTime;
	if (ActiveElapsedTime >= ActiveSettings.ActiveDuration)
	{
		BeginFadeOut();
		return;
	}
	const float PreviousRadius = CurrentRadius;
	CurrentRadius += ActiveSettings.ExpansionSpeed * DeltaTime;
	UpdateVisual();
	CheckPlayerHit(PreviousRadius);
}

void ARLExpandingRingAttack::BeginFadeOut()
{
	if (bFadingOut || IsActorBeingDestroyed()) { return; }
	bFadingOut = true;
	bActive = false;
	FadeOutElapsedTime = 0.0f;
	SetLifeSpan(FMath::Max(0.01f, ActiveSettings.FadeOutDuration));
}

void ARLExpandingRingAttack::UpdateVisual()
{
	const float HalfThickness = ActiveSettings.RingThickness * 0.5f;
	const float InnerRadius = FMath::Max(0.0f, CurrentRadius - HalfThickness);
	const float OuterRadius = CurrentRadius + HalfThickness;
	const float VisualRadius = (InnerRadius + OuterRadius) * 0.5f;
	const float HalfAngle = PI / SegmentCount;
	const float TangentLength = 2.0f * OuterRadius * FMath::Tan(HalfAngle);
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		const float Angle = 2.0f * PI * Index / SegmentCount;
		const FVector Location(VisualRadius * FMath::Cos(Angle), VisualRadius * FMath::Sin(Angle),
			ActiveSettings.AttackHeight * 0.5f);
		const FTransform Transform(FRotator(0.0f, FMath::RadiansToDegrees(Angle), 0.0f), Location,
			FVector((OuterRadius - InnerRadius) / 100.0f, TangentLength / 100.0f,
				ActiveSettings.AttackHeight / 100.0f));
		RingVisual->UpdateInstanceTransform(Index, Transform, false, Index == SegmentCount - 1, true);
	}
}

bool ARLExpandingRingAttack::IntersectsSweptRing(const FVector2D& Start, const FVector2D& End,
	float PreviousRadius, float NextRadius, float Padding)
{
	// Solve the inner/outer boundary crossings in relative XY space. This also
	// catches a fast player crossing through the center between two frames.
	TArray<double, TInlineAllocator<8>> Times{0.0, 1.0};
	const FVector2D Movement = End - Start;
	const double RadiusDelta = NextRadius - PreviousRadius;
	const double A = Movement.SizeSquared() - RadiusDelta * RadiusDelta;
	for (double Offset : {-static_cast<double>(Padding), static_cast<double>(Padding)})
	{
		const double Radius = PreviousRadius + Offset;
		const double B = 2.0 * (FVector2D::DotProduct(Start, Movement) - Radius * RadiusDelta);
		const double C = Start.SizeSquared() - Radius * Radius;
		auto AddTime = [&Times](double Time)
		{
			if (Time > 0.0 && Time < 1.0) { Times.Add(Time); }
		};
		if (FMath::Abs(A) < UE_DOUBLE_SMALL_NUMBER)
		{
			if (FMath::Abs(B) >= UE_DOUBLE_SMALL_NUMBER) { AddTime(-C / B); }
		}
		else
		{
			const double Discriminant = B * B - 4.0 * A * C;
			if (Discriminant >= 0.0)
			{
				const double Root = FMath::Sqrt(Discriminant);
				AddTime((-B - Root) / (2.0 * A));
				AddTime((-B + Root) / (2.0 * A));
			}
		}
	}
	Times.Sort();
	auto IsInside = [&](double Time)
	{
		const double Distance = (Start + Movement * Time).Size();
		const double Radius = PreviousRadius + RadiusDelta * Time;
		return Distance >= FMath::Max(0.0, Radius - Padding) - 0.001 &&
			Distance <= Radius + Padding + 0.001;
	};
	for (int32 Index = 0; Index < Times.Num(); ++Index)
	{
		if (IsInside(Times[Index]) || (Index > 0 && IsInside((Times[Index - 1] + Times[Index]) * 0.5)))
		{
			return true;
		}
	}
	return false;
}

void ARLExpandingRingAttack::CheckPlayerHit(float PreviousRadius)
{
	ARLPlayerCharacter* Player = Cast<ARLPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!IsValid(Player)) { TrackedPlayer.Reset(); return; }
	const FVector PlayerLocation = Player->GetActorLocation();
	if (TrackedPlayer.Get() != Player)
	{
		TrackedPlayer = Player;
		PreviousPlayerLocation = PlayerLocation;
	}
	const FVector PreviousRelative = PreviousPlayerLocation - GetActorLocation();
	const FVector CurrentRelative = PlayerLocation - GetActorLocation();
	PreviousPlayerLocation = PlayerLocation;
	if (!bActive || bFadingOut || bPlayerHit || Player->IsDead()) { return; }
	const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	if (CurrentRelative.Z - HalfHeight > ActiveSettings.AttackHeight || CurrentRelative.Z + HalfHeight < 0.0f)
	{
		return;
	}
	const float Padding = ActiveSettings.RingThickness * 0.5f + Capsule->GetScaledCapsuleRadius();
	if (!IntersectsSweptRing(FVector2D(PreviousRelative.X, PreviousRelative.Y),
		FVector2D(CurrentRelative.X, CurrentRelative.Y), PreviousRadius, CurrentRadius, Padding))
	{
		return;
	}
	if (Player->IsRolling())
	{
		if (!bPlayerDodged)
		{
			bPlayerDodged = true;
			OnPlayerDodged.Broadcast(Player);
		}
		return;
	}
	// Rolling contacts above never consume this attack. Other immunity still
	// passes through the damage gateway, so ending a roll inside the ring is unsafe.
	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		Player, ActiveSettings.Damage, GetInstigatorController(), this, nullptr);
	if (AppliedDamage > 0.0f) { bPlayerHit = true; }
}
