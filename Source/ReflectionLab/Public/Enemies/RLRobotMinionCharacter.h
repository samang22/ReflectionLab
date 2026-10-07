#pragma once

#include "CoreMinimal.h"
#include "Enemies/RLEnemyCharacter.h"
#include "RLRobotMinionCharacter.generated.h"

class URLRobotMinionDataAsset;
class URLBossLaserComponent;
class URLBossAimComponent;
class UStaticMeshComponent;
class UDecalComponent;

UCLASS()
class REFLECTIONLAB_API ARLRobotMinionCharacter : public ARLEnemyCharacter
{
	GENERATED_BODY()
public:
	ARLRobotMinionCharacter();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minion") TObjectPtr<URLRobotMinionDataAsset> Settings;
protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Minion") TObjectPtr<URLBossAimComponent> AimComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Minion") TObjectPtr<URLBossLaserComponent> LaserComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Minion") TObjectPtr<UStaticMeshComponent> LaserBeam;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Minion") TObjectPtr<UDecalComponent> LaserDecal;
};
