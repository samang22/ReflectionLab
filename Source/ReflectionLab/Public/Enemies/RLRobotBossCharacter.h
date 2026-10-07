#pragma once

#include "CoreMinimal.h"
#include "Enemies/RLEnemyCharacter.h"
#include "RLRobotBossCharacter.generated.h"

class URLBossOverloadComponent;
class USphereComponent;
class UDecalComponent;
class URLBossLaserComponent;
class UStaticMeshComponent;
class URLBossAimComponent;

UCLASS()
class REFLECTIONLAB_API ARLRobotBossCharacter : public ARLEnemyCharacter
{
	GENERATED_BODY()
public:
	ARLRobotBossCharacter();
protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<URLBossOverloadComponent> OverloadComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<USphereComponent> AbsorptionVolume;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UDecalComponent> AbsorptionDecal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<URLBossLaserComponent> LaserComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UStaticMeshComponent> LaserBeam;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UDecalComponent> LaserDecal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<URLBossAimComponent> AimComponent;
};
