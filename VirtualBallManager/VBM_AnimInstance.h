// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PoseMatchInfo.h"
#include "Animation/AnimInstance.h"
#include "VBM_AnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class VIRTUALBALLMANAGER_API UVBM_AnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void DrawPoseMatchInfo(int32 AnimIndx, int32 FrameStep = 1);

	void DrawPoseMatchInfo(const TArray<FPoseMatchInfo>& MatchInfos, int32 FrameStep);

public:
	
	/** Perform evaluation. Can be called from worker threads. */
	void ParallelEvaluateAnimation(
		bool bForceRefPose, const USkeletalMesh* InSkeletalMesh,
		TArray<FTransform>& OutBoneSpaceTransforms, FBlendedHeapCurve& OutCurve, FCompactPose& OutPose);
	
	// the below functions are the native overrides for each phase
	// Native initialization override point
	virtual void NativeInitializeAnimation();

	// Native update override point. It is usually a good idea to simply gather data in this step and 
	// for the bulk of the work to be done in NativeUpdateAnimation.
	virtual void NativeUpdateAnimation(float DeltaSeconds);

	// Native Post Evaluate override point
	virtual void NativePostEvaluateAnimation();

	void GeneratePoseMatchInfos(const UAnimSequence* pAnim);
	void GeneratePoseMatchInfo(FPoseMatchInfo& OutInfo, const FCompactPose& PrevPose, const FCompactPose& CurPose);

public:

	UPROPERTY(EditAnywhere, Category = Settings)
	TArray<UAnimSequence*> Anims;

	UPROPERTY(EditAnywhere, Category = Settings)
	float LimitVel = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool IdleState;

	TMap<const UAnimSequence*, TArray<int32>> AnimHitFrames;
	TMap<const UAnimSequence*, TArray<int32>> AnimMatchFrames;
	TMap<const UAnimSequence*, TArray<FHitSection>> AnimHitSections;
	TMap<const UAnimSequence*, TArray<FPoseMatchInfo>> AnimPoseInfos;

	TArray<FAnimPlayer> AnimPlayers;

	FCompactPose ResultPose;

private:

	TArray<FVector> NextHitVels;
	TArray<FVector> NextHitPoss;

	TArray<TArray<FVector>> BallTrajectories;
};
