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
	for (auto PawnIt = GWorld->GetPawnIterator(); PawnIt; ++PawnIt)
	{
		AVBM_Pawn* pPawn = Cast<AVBM_Pawn>(PawnIt->Get());
		if (pPawn == NULL)
			continue;

		FVector BallPos = pPawn->GetBallPos();
		if (BallPos != FVector::ZeroVector)
		{
			return BallPos;
		}
	}

	return FVector::ZeroVector;
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::Tick(float DeltaSeconds)
{
	if (PassOrders.Num() < 3)
		return;

	if (PassOrders[0]->pDestPawn == NULL)
	{
		PassOrders[0]->pDestPawn = PassOrders[1];
		PassOrders[0]->CreateNextPlayer();
	}

	if (PassOrders[1]->pDestPawn == NULL)
	{
		PassOrders[1]->pDestPawn = PassOrders[2];
		PassOrders[1]->CreateNextPlayer();

		PassOrders[0]->TimeError += DeltaSeconds;
		PassOrders[0]->PlayHitMotion();
	}

	if (PassOrders[0]->bBeginNextMotion)
	{
		pPrevPawn = PassOrders[0];
		PassOrders[0]->pDestPawn = NULL;
		PassOrders.RemoveAt(0);

		if (PassOrders.Num() >= 3)
		{
			PassOrders[1]->pDestPawn = PassOrders[2];
			PassOrders[1]->CreateNextPlayer();

			PassOrders[0]->TimeError += DeltaSeconds;
			PassOrders[0]->PlayHitMotion();
		}
	}

	if (PassOrders.Num() >= 3 && PassOrders[0]->bHitBall)
	{
		if (pPrevPawn != NULL && pPrevPawn->pAnimNode != NULL)
		{
			pPrevPawn->pAnimNode->PassTrajectory2.Empty();
		}
	}
}


#pragma optimize("", on)