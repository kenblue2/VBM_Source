// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VirtualBallManagerGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class VIRTUALBALLMANAGER_API AVirtualBallManagerGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	AVirtualBallManagerGameModeBase();

	/** 
	 *	Function called every frame on this Actor. Override this function to implement custom logic to be executed every frame.
	 *	Note that Tick is disabled by default, and you will need to check PrimaryActorTick.bCanEverTick is set to true to enable it.
	 *
	 *	@param	DeltaSeconds	Game time elapsed during last frame modified by the time dilation
	 */
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION(BlueprintCallable, Category = "VBM_GameMode")
	FVector GetBallPos();

public:

	FVector BallPos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VBM_Pawn)
	TArray<class AVBM_Pawn*> PassOrders;

	TArray<FVector> BallTrajectory;

	class AVBM_Pawn* pPrevPawn;
};
