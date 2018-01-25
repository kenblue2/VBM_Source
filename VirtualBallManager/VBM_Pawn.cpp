// Fill out your copyright notice in the Description page of Project Settings.

#include "VBM_Pawn.h"
#include "AnimNode_VBM.h"


//-------------------------------------------------------------------------------------------------
// Sets default values
AVBM_Pawn::AVBM_Pawn()
	: pDestPawn(NULL)
	, pAnimNode(NULL)
	, pPrevPawn(NULL)
	, bHitBall(false)
	, bHaveBall(false)
	, bBeginNextMotion(false)
	, HitBallTime(0.f)
	, PlayerPos(FVector::ZeroVector)
	, HitBallPos(FVector::ZeroVector)
	, HitBallVel(FVector::ZeroVector)
	, HitAxisAng(FVector::ZeroVector)
	, TimeError(0.f)
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

//-------------------------------------------------------------------------------------------------
// Called when the game starts or when spawned
void AVBM_Pawn::BeginPlay()
{
	Super::BeginPlay();
}

//-------------------------------------------------------------------------------------------------
// Called every frame
void AVBM_Pawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<UActorComponent*> ActorComps = GetComponentsByClass(USkeletalMeshComponent::StaticClass());
	if (ActorComps.Num() == 0)
		return;

	USkeletalMeshComponent* pSkelComp = Cast<USkeletalMeshComponent>(ActorComps[0]);
	if (pSkelComp == NULL)
		return;

	PlayerPos = pSkelComp->GetBoneLocation(FName("Root"));
	PlayerPos.Z = 2.f;
}

//-------------------------------------------------------------------------------------------------
// Called to bind functionality to input
void AVBM_Pawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

//-------------------------------------------------------------------------------------------------
void AVBM_Pawn::CreateNextPlayer()
{
	if (pAnimNode != NULL)
	{
		pAnimNode->CreateNextPlayer(this, TimeError);
	}
}

//-------------------------------------------------------------------------------------------------
void AVBM_Pawn::PlayHitMotion()
{
	if (pAnimNode != NULL)
	{
		bHitBall = false;
		bBeginNextMotion = false;
		pAnimNode->PlayHitMotion(this, TimeError);
	}
}

//-------------------------------------------------------------------------------------------------
FVector AVBM_Pawn::GetBallPos()
{
	if (pAnimNode != NULL && pAnimNode->bMoveBall)
	{
		return pAnimNode->BallPos;
	}

	return FVector::ZeroVector;
}