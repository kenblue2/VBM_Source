// Fill out your copyright notice in the Description page of Project Settings.

#include "VirtualBallManagerGameModeBase.h"
#include "DrawDebugHelpers.h"

#pragma optimize("", off)


//-------------------------------------------------------------------------------------------------
AVirtualBallManagerGameModeBase::AVirtualBallManagerGameModeBase()
	: BallTime(0.f)
{
	PrimaryActorTick.bCanEverTick = true;
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::Tick(float DeltaSeconds)
{
	TArray<FVector> HitPosList;

	//for (auto PawnIt = GWorld->GetPawnIterator(); PawnIt; ++PawnIt)
	//{
	//	APawn* pPawn = PawnIt->Get();
	//	if (pPawn == NULL)
	//		continue;

	//	TArray<UActorComponent*> Components = pPawn->GetComponentsByClass(USkeletalMeshComponent::StaticClass());
	//	if (Components.Num() == 0)
	//		continue;

	//	USkeletalMeshComponent* pSkelComp = Cast<USkeletalMeshComponent>(Components[0]);
	//	if (pSkelComp == NULL)
	//		continue;

	//	FVector HitPos = pSkelComp->GetBoneLocation(FName("Right_Ankle_Joint_01"));
	//	DrawDebugSphere(GWorld, HitPos, 5.f, 8, FColor::Red);

	//	HitPosList.Add(HitPos);
	//}

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
}


#pragma optimize("", on)