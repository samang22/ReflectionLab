#include "Combat/RLExplosionVisual.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName IndicatorColorParameterName(TEXT("IndicatorColor"));
	const FName PerfectIndicatorColorParameterName(TEXT("PerfectIndicatorColor"));
	const FName IndicatorOpacityParameterName(TEXT("IndicatorOpacity"));
	const FName ConeSlopeParameterName(TEXT("ConeSlope"));
	const FName PerfectBandInnerRadiusParameterName(TEXT("PerfectBandInnerRadiusUV"));
}

ARLExplosionVisual::ARLExplosionVisual()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ExplosionRadiusDecalFront = CreateDefaultSubobject<UDecalComponent>(
		TEXT("ExplosionRadiusDecalFront"));
	ExplosionRadiusDecalFront->SetupAttachment(SceneRoot);
	ExplosionRadiusDecalFront->SetRelativeRotation(FRotator(-90.0f, -90.0f, 0.0f));
	ExplosionRadiusDecalFront->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	ExplosionRadiusDecalFront->SetSortOrder(7);
	ExplosionRadiusDecalFront->FadeScreenSize = 0.0f;

	ExplosionRadiusDecalBack = CreateDefaultSubobject<UDecalComponent>(
		TEXT("ExplosionRadiusDecalBack"));
	ExplosionRadiusDecalBack->SetupAttachment(SceneRoot);
	ExplosionRadiusDecalBack->SetRelativeRotation(FRotator(-90.0f, 90.0f, 0.0f));
	ExplosionRadiusDecalBack->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	ExplosionRadiusDecalBack->SetSortOrder(7);
	ExplosionRadiusDecalBack->FadeScreenSize = 0.0f;

	ExplosionRadiusDecalLeft = CreateDefaultSubobject<UDecalComponent>(
		TEXT("ExplosionRadiusDecalLeft"));
	ExplosionRadiusDecalLeft->SetupAttachment(SceneRoot);
	ExplosionRadiusDecalLeft->SetRelativeRotation(FRotator(-90.0f, 180.0f, 0.0f));
	ExplosionRadiusDecalLeft->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	ExplosionRadiusDecalLeft->SetSortOrder(7);
	ExplosionRadiusDecalLeft->FadeScreenSize = 0.0f;

	ExplosionRadiusDecalRight = CreateDefaultSubobject<UDecalComponent>(
		TEXT("ExplosionRadiusDecalRight"));
	ExplosionRadiusDecalRight->SetupAttachment(SceneRoot);
	ExplosionRadiusDecalRight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	ExplosionRadiusDecalRight->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	ExplosionRadiusDecalRight->SetSortOrder(7);
	ExplosionRadiusDecalRight->FadeScreenSize = 0.0f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ExplosionDecalMaterialFinder(
		TEXT("/Game/ReflectionLab/Art/Materials/M_ParryRangeIndicator.M_ParryRangeIndicator"));
	if (ExplosionDecalMaterialFinder.Succeeded())
	{
		ExplosionRadiusDecalFront->SetDecalMaterial(ExplosionDecalMaterialFinder.Object);
		ExplosionRadiusDecalBack->SetDecalMaterial(ExplosionDecalMaterialFinder.Object);
		ExplosionRadiusDecalLeft->SetDecalMaterial(ExplosionDecalMaterialFinder.Object);
		ExplosionRadiusDecalRight->SetDecalMaterial(ExplosionDecalMaterialFinder.Object);
	}
}

void ARLExplosionVisual::Initialize(
	UStaticMesh* ShardMesh,
	UMaterialInterface* ShardMaterial,
	const FVector& ShardBaseScale,
	float BlastRadius,
	const FLinearColor& DecalColor)
{
	CachedBlastRadius = FMath::Max(1.0f, BlastRadius);
	CachedShardBaseScale = ShardBaseScale;
	ElapsedTime = 0.0f;

	ExplosionDecalMaterialInstances.Reset();
	for (UDecalComponent* ExplosionDecal : {
		ExplosionRadiusDecalFront.Get(),
		ExplosionRadiusDecalBack.Get(),
		ExplosionRadiusDecalLeft.Get(),
		ExplosionRadiusDecalRight.Get()})
	{
		if (!ExplosionDecal)
		{
			continue;
		}

		const float DecalRadius = CachedBlastRadius * DecalRadiusMultiplier;
		ExplosionDecal->DecalSize = FVector(96.0f, DecalRadius, DecalRadius);
		UMaterialInstanceDynamic* MaterialInstance =
			ExplosionDecal->CreateDynamicMaterialInstance();
		if (MaterialInstance)
		{
			MaterialInstance->SetVectorParameterValue(IndicatorColorParameterName, DecalColor);
			MaterialInstance->SetVectorParameterValue(
				PerfectIndicatorColorParameterName,
				DecalColor);
			MaterialInstance->SetScalarParameterValue(IndicatorOpacityParameterName, DecalOpacity);
			MaterialInstance->SetScalarParameterValue(ConeSlopeParameterName, 1.0f);
			MaterialInstance->SetScalarParameterValue(
				PerfectBandInnerRadiusParameterName,
				0.5f);
			ExplosionDecalMaterialInstances.Add(MaterialInstance);
		}
	}

	ShardMeshes.Reset();
	ShardDirections.Reset();
	if (ShardMesh && ShardMaterial)
	{
		const int32 RayCount = FMath::Clamp(ShardRayCount, 4, 24);
		const int32 SegmentCount = FMath::Clamp(TrailSegmentsPerRay, 1, 6);
		ShardDirections.Reserve(RayCount);
		ShardMeshes.Reserve(RayCount * SegmentCount);

		const float AngleOffset = FMath::FRandRange(0.0f, 360.0f);
		for (int32 RayIndex = 0; RayIndex < RayCount; ++RayIndex)
		{
			const float AngleDegrees = AngleOffset + 360.0f *
				static_cast<float>(RayIndex) / static_cast<float>(RayCount);
			const FVector Direction = FVector::ForwardVector.RotateAngleAxis(
				AngleDegrees,
				FVector::UpVector);
			ShardDirections.Add(Direction);

			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const FName ComponentName(*FString::Printf(
					TEXT("ExplosionShard_%d_%d"),
					RayIndex,
					SegmentIndex));
				UStaticMeshComponent* ShardComponent = NewObject<UStaticMeshComponent>(
					this,
					ComponentName);
				if (!ShardComponent)
				{
					continue;
				}

				ShardComponent->SetupAttachment(SceneRoot);
				ShardComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				ShardComponent->SetCanEverAffectNavigation(false);
				ShardComponent->SetCastShadow(false);
				ShardComponent->SetReceivesDecals(false);
				ShardComponent->SetStaticMesh(ShardMesh);
				ShardComponent->SetMaterial(0, ShardMaterial);
				ShardComponent->SetVisibility(false, true);
				ShardComponent->RegisterComponent();
				ShardMeshes.Add(ShardComponent);
			}
		}
	}

	bInitialized = true;
}

void ARLExplosionVisual::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized)
	{
		return;
	}

	ElapsedTime += FMath::Max(0.0f, DeltaTime);
	const float Duration = FMath::Max(0.1f, EffectDuration);
	const float EffectAlpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);

	const float DecalGrowAlpha = FMath::Clamp(EffectAlpha / 0.18f, 0.0f, 1.0f);
	const float DecalRadiusScale = FMath::InterpEaseOut(0.2f, 1.0f, DecalGrowAlpha, 2.0f);
	const float DecalRadius = CachedBlastRadius * DecalRadiusMultiplier * DecalRadiusScale;
	for (UDecalComponent* ExplosionDecal : {
		ExplosionRadiusDecalFront.Get(),
		ExplosionRadiusDecalBack.Get(),
		ExplosionRadiusDecalLeft.Get(),
		ExplosionRadiusDecalRight.Get()})
	{
		if (ExplosionDecal)
		{
			ExplosionDecal->DecalSize = FVector(
				96.0f,
				DecalRadius,
				DecalRadius);
		}
	}
	for (UMaterialInstanceDynamic* MaterialInstance : ExplosionDecalMaterialInstances)
	{
		if (MaterialInstance)
		{
			MaterialInstance->SetScalarParameterValue(
				IndicatorOpacityParameterName,
			DecalOpacity * FMath::Square(1.0f - EffectAlpha));
		}
	}

	const int32 SegmentCount = FMath::Clamp(TrailSegmentsPerRay, 1, 6);
	for (int32 MeshIndex = 0; MeshIndex < ShardMeshes.Num(); ++MeshIndex)
	{
		UStaticMeshComponent* ShardComponent = ShardMeshes[MeshIndex];
		if (!ShardComponent)
		{
			continue;
		}

		const int32 RayIndex = MeshIndex / SegmentCount;
		const int32 SegmentIndex = MeshIndex % SegmentCount;
		if (!ShardDirections.IsValidIndex(RayIndex))
		{
			ShardComponent->SetVisibility(false, true);
			continue;
		}

		const float SegmentTime = ElapsedTime - TrailSegmentDelay * SegmentIndex;
		if (SegmentTime <= 0.0f)
		{
			ShardComponent->SetVisibility(false, true);
			continue;
		}

		const float SegmentAlpha = FMath::Clamp(SegmentTime / Duration, 0.0f, 1.0f);
		const float TravelAlpha = FMath::InterpEaseOut(0.0f, 1.0f, SegmentAlpha, 2.0f);
		const FVector Direction = ShardDirections[RayIndex];
		const float TravelDistance = CachedBlastRadius * ShardTravelRadiusMultiplier * TravelAlpha;
		const float ArcOffset = FMath::Sin(SegmentAlpha * PI) * ShardArcHeight;
		ShardComponent->SetRelativeLocation(
			Direction * TravelDistance + FVector(0.0f, 0.0f, 18.0f + ArcOffset));
		ShardComponent->SetRelativeRotation(Direction.Rotation());

		const float SegmentScaleFalloff = 1.0f - 0.18f * SegmentIndex;
		const float FadeScale = FMath::Max(0.02f, 1.0f - SegmentAlpha);
		ShardComponent->SetRelativeScale3D(
			CachedShardBaseScale *
			ShardScaleMultiplier *
			SegmentScaleFalloff *
			FadeScale);
		ShardComponent->SetVisibility(true, true);
	}

	if (EffectAlpha >= 1.0f)
	{
		Destroy();
	}
}
