// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VBM_Pawn.generated.h"

struct FPoseMatchInfo;

UCLASS()
class VIRTUALBALLMANAGER_API AVBM_Pawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVBM_Pawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void CreateNextPlayer();
	void CreateNextPlayer(const TArray<FVector>& Trajectory);

	void PlayHitMotion();

	FVector GetBallPos();

public:

	bool bHitBall;
	bool bHaveBall;
	bool bBeginNextMotion;

	AVBM_Pawn* pDestPawn;
	AVBM_Pawn* pPrevPawn;

	FVector PlayerPos;
	FVector HitBallPos;
	FVector HitBallVel;
	FVector HitAxisAng;

	float HitBallTime;

	struct FAnimNode_VBM* pAnimNode;

	float TimeError;
};
