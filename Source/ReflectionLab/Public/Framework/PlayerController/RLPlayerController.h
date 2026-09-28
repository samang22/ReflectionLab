// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RLPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class URLParryComboWidget;
class URLPlayerHealthWidget;
class URLRunStatusWidget;

UCLASS()
class REFLECTIONLAB_API ARLPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARLPlayerController();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

private:
	void MoveForward();
	void MoveBackward();
	void MoveLeft();
	void MoveRight();
	void Move(const FVector2D& Direction);
	void ActivateParry();
	void UpdateAimRotation();
	bool IsGameplayInputAllowed() const;
	void CreateOrBindParryComboWidget();
	void CreateOrBindPlayerHealthWidget();
	void CreateOrBindRunStatusWidget();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveForwardAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveBackwardAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveLeftAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ReflectAction;

	UPROPERTY(Transient)
	TObjectPtr<URLParryComboWidget> ParryComboWidget;

	UPROPERTY(Transient)
	TObjectPtr<URLPlayerHealthWidget> PlayerHealthWidget;

	UPROPERTY(Transient)
	TObjectPtr<URLRunStatusWidget> RunStatusWidget;
};
