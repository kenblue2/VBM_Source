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

/*
	TArray<FVector> HitPosList;

	for (auto PawnIt = GWorld->GetPawnIterator(); PawnIt; ++PawnIt)
	{
		APawn* pPawn = PawnIt->Get();
		if (pPawn == NULL)
			continue;

		TArray<UActorComponent*> ActorComps = pPawn->GetComponentsByClass(USkeletalMeshComponent::StaticClass());
		if (ActorComps.Num() == 0)
			continue;

		USkeletalMeshComponent* pSkelComp = Cast<USkeletalMeshComponent>(ActorComps[0]);
		if (pSkelComp == NULL)
			continue;

		UVBM_AnimInstance* pAnimInst = Cast<UVBM_AnimInstance>(pSkelComp->GetAnimInstance());
		if (pAnimInst == NULL)
			continue;

		if (pAnimInst->IdleState)
		{
			//pAnimInst->IdleState = false;
		}

		FVector HitPos = pSkelComp->GetSocketLocation(FName("Right_Hit_Socket"));
		DrawDebugSphere(GWorld, HitPos, 5.f, 8, FColor::Red);

		HitPosList.Add(HitPos);
	}

	if (HitPosList.Num() == 2)
	{
		FVector P0 = HitPosList[0];
		FVector P1 = HitPosList[1];

		float t1 = 2.f;

		FVector HorVel = P1 - P0;
		HorVel.Z = 0.f;
		HorVel /= t1;

		float v0 = (P1.Z - P0.Z) / t1 + 0.5f * 490.f * t1;

		int32 NumTimes = 20;
		float DeltaTime = t1 / NumTimes;

		TArray<FVector> MovePosList;
		for (int32 IdxTime = 0; IdxTime <= NumTimes; ++IdxTime)
		{
			float Time = DeltaTime * IdxTime;

			float x = HorVel.X * Time;
			float y = HorVel.Y * Time;
			float z = v0 * Time - 0.5f * 490.f * Time * Time;

			MovePosList.Add(P0 + FVector(x, y, z));
		}

		for (int32 IdxPos = 1; IdxPos < MovePosList.Num(); ++IdxPos)
		{
			DrawDebugLine(GWorld, MovePosList[IdxPos - 1], MovePosList[IdxPos], FColor::Magenta);
		}
	}
*/
}


#pragma optimize("", on)