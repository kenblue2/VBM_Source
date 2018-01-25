// Fill out your copyright notice in the Description page of Project Settings.

#include "VirtualBallManagerGameModeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "VBM_AnimInstance.h"
#include "AnimNode_VBM.h"
#include "VBM_Pawn.h"


#pragma optimize("", off)


//-------------------------------------------------------------------------------------------------
AVirtualBallManagerGameModeBase::AVirtualBallManagerGameModeBase()
	: pPrevPawn(NULL)
{
	PrimaryActorTick.bCanEverTick = true;
}

//-------------------------------------------------------------------------------------------------
FVector AVirtualBallManagerGameModeBase::GetBallPos()
{
	FVector BallPos = FVector::ZeroVector;

	for (auto& BallCtrl : BallCtrls)
	{
		if (BallCtrl.BallTime > 0.f)
		{
			BallPos = BallCtrl.GetBallPos();
		}
	}

	return BallPos;
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::Tick(float DeltaSeconds)
{
	for (auto& BallCtrl : BallCtrls)
	{
		BallCtrl.Update(DeltaSeconds);
	}

	if (BallCtrls.Num() >= 2 && BallCtrls.Last().BallTime > 0.f)
	{
		BallCtrls.RemoveAt(0);
	}

	if (PassOrders.Num() < 3)
		return;

	if (PassOrders[0]->pDestPawn == NULL)
	{
		PassOrders[0]->pDestPawn = PassOrders[1];
		PassOrders[0]->CreateNextPlayer();
	}

	if (PassOrders[1]->pDestPawn == NULL)
	{
		PassOrders[1]->pPrevPawn = PassOrders[0];
		PassOrders[1]->pDestPawn = PassOrders[2];
		PassOrders[1]->CreateNextPlayer();

		PassOrders[0]->TimeError += DeltaSeconds;
		PassOrders[0]->PlayHitMotion();

		FBallController BallCtrl;
		{
			BallCtrl.BallTime = PassOrders[0]->pAnimNode->BallTime;
			BallCtrl.BallTrajectory = PassOrders[0]->pAnimNode->PassTrajectory2;
		}
		BallCtrls.Add(BallCtrl);
	}

	if (PassOrders[0]->bBeginNextMotion)
	{
		PassOrders[0]->pDestPawn = NULL;
		PassOrders.RemoveAt(0);
	}
}


#pragma optimize("", on)