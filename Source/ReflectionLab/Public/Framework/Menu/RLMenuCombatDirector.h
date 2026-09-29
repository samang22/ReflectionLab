#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLMenuCombatDirector.generated.h"

class ARLEnemyCharacter;
class ARLProjectile;
class ATargetPoint;
class UCameraComponent;
class USceneComponent;
class USpringArmComponent;
class URLProjectileDefinitionDataAsset;

UCLASS()
class REFLECTIONLAB_API ARLMenuCombatDirector : public AActor
{
	GENERATED_BODY()

public:
	ARLMenuCombatDirector();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SpawnMenuCombatants();
	void FireNextPresentationShot();
	bool SpawnPresentationProjectile(
		ARLEnemyCharacter* SourceEnemy,
		ARLEnemyCharacter* TargetEnemy,
		AActor* ExitTarget);

	UPROPERTY(VisibleAnywhere, Category = "Menu Presentation")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Menu Presentation")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Menu Presentation")
	TObjectPtr<UCameraComponent> MenuCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Menu Presentation")
	TSubclassOf<ARLEnemyCharacter> EnemyClass;

	UPROPERTY(EditDefaultsOnly, Category = "Menu Presentation")
	TSubclassOf<ARLProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Menu Presentation")
	TObjectPtr<URLProjectileDefinitionDataAsset> RallyProjectileDefinition;

	UPROPERTY(EditDefaultsOnly, Category = "Menu Presentation", meta = (ClampMin = "0.1"))
	float ShotInterval = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Menu Presentation")
	TArray<FVector> EnemyOffsets = {
		FVector(-650.0f, -420.0f, 96.0f),
		FVector(-650.0f, 420.0f, 96.0f),
		FVector(650.0f, -420.0f, 96.0f),
		FVector(650.0f, 420.0f, 96.0f)};

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARLEnemyCharacter>> MenuEnemies;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATargetPoint>> RallyExitTargets;

	FTimerHandle ShotTimerHandle;
	int32 NextShooterIndex = 0;
};
