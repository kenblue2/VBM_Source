// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VirtualBallManagerGameModeBase.generated.h"


struct FBallController
{
	FBallController() : BallTime(0.f) {}

	void Update(float DeltaTime)
	{
		BallTime += DeltaTime;
	}

	FVector GetBallPos()
	{
		if (BallTrajectory.Num() == 0)
			return FVector::ZeroVector;

		if (BallTime < 0.f)
			return BallTrajectory[0];

		float fBallFrame = BallTime / 0.033f;
		int32 iBallFrame = (int32)fBallFrame;
		float Ratio = fBallFrame - iBallFrame;

		if (BallTrajectory.IsValidIndex(iBallFrame) &&
			BallTrajectory.IsValidIndex(iBallFrame + 1))
		{
			return FMath::Lerp(BallTrajectory[iBallFrame], BallTrajectory[iBallFrame + 1], Ratio);
		}

		return BallTrajectory.Last();
	}

public:

	float BallTime;

	FVector BallAxisAng;

	TArray<FVector> BallTrajectory;
};


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

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Overridable function called whenever this actor is being removed from a level */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "VBM_GameMode")
	FVector GetBallPos();

	UFUNCTION(BlueprintCallable, Category = "VBM_GameMode")
	FRotator GetBallRot(float DeltaTime);

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VBM_Pawn)
	TArray<class AVBM_Pawn*> PassOrders;

	class AVBM_Pawn* pPrevPawn;

	TArray<FBallController> BallCtrls;
};
