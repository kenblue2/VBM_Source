// Fill out your copyright notice in the Description page of Project Settings.

#include "VirtualBallManagerGameModeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "VBM_AnimInstance.h"
#include "VBM_Pawn.h"


#pragma optimize("", off)


//-------------------------------------------------------------------------------------------------
AVirtualBallManagerGameModeBase::AVirtualBallManagerGameModeBase()
	: BallTime(0.f)
{
	PrimaryActorTick.bCanEverTick = true;
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::CalcBallTrajectory(const FVector& BeginPos, const FVector& EndPos, int32 NumBounds)
{
	BallTrajectory;
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::Tick(float DeltaSeconds)
{
	if (PassOrders.Num() == 0)
		return;

	//for (int32 IdxOrder = 1; IdxOrder < PassOrders.Num(); ++IdxOrder)
	//{
	//	FVector BeginPos = PassOrders[IdxOrder - 1]->PlayerPos;
	//	FVector EndPos = PassOrders[IdxOrder]->PlayerPos;

	//	BeginPos.Z = 1.f;
	//	EndPos.Z = 1.f;

	//	DrawDebugLine(GetWorld(), BeginPos, EndPos, FColor::Yellow, false, -1.0f, 0, 1.f);
	//}

	//for (auto& PassOrder : PassOrders)
	//{
	//	DrawDebugSphere(GetWorld(), PassOrder->PlayerPos, 10.f, 32, FColor::Red);
	//}

	if (PassOrders.Num() < 3)
		return;

	for (auto& PassOrder : PassOrders)
	{
		if (PassOrder == NULL)
			return;
	}

	//if (BallTrajectory.Num() == 0)
	//{
	//	const FVector& BeginPos = PassOrders[0]->HitPos;
	//	const FVector& EndPos = PassOrders[1]->HitPos;

	//	if (BeginPos != EndPos)
	//	{
	//		CalcBallTrajectory(BeginPos, EndPos, 1);
	//	}
	//}

	if (PassOrders[0]->IsPlaying == false)
	{
		PassOrders[0]->pDestPawn = PassOrders[1];
		PassOrders[1]->pDestPawn = PassOrders[2];

		PassOrders[0]->IsPlaying = true;
	}
	else if (PassOrders[0]->pDestPawn == NULL)
	{
		PassOrders[0]->IsPlaying = false;
		PassOrders.RemoveAt(0);
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