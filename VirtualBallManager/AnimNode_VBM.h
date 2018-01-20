#pragma once

#include "CoreMinimal.h"
#include "AnimNodes/PoseMatchInfo.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_VBM.generated.h"


USTRUCT(BlueprintInternalUseOnly)
struct VIRTUALBALLMANAGER_API FAnimNode_VBM : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink Result;

	UPROPERTY(EditAnywhere, Category = Settings)
	TArray<UAnimSequence*> Anims;

	UPROPERTY(EditAnywhere, Category = Settings)
	int32 AnimIndex = -1;

	UPROPERTY(EditAnywhere, Category = Settings)
	int32 BeginFrame = 0;

	UPROPERTY(EditAnywhere, Category = Settings)
	int32 EndFrame = 0;

	UPROPERTY(EditAnywhere, Category = Settings)
	int32 FrameStep = 1;

	UPROPERTY(EditAnywhere, Category = Settings)
	FColor BoneColor = FColor::Black;

	UPROPERTY(EditAnywhere, Category = Settings)
	float LimitVel = 200.f;

public:

	FAnimNode_VBM();

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	//virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	//virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	/**
	* Override this to indicate that PreUpdate() should be called on the game thread (usually to
	* This is called on the game thread.
	* gather non-thread safe data) before Update() is called.
	*/
	virtual bool HasPreUpdate() const override { return true; }

	/** Override this to perform game-thread work prior to non-game thread Update() being called */
	virtual void PreUpdate(const UAnimInstance* InAnimInstance);

protected:

	void AlignPose(USkeleton* pSkel, FCompactPose& AnimPose, FQuat* pOutQuat = NULL);

	FVector CalcFrontDir(USkeleton* pSkel, const FCompactPose& AnimPose, FVector* pOutHipPos = NULL);

	void DrawPose(const FCompactPose& Pose, const FColor& Color);

	void DrawAnimPoses(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont);
	void DrawAnimPoses(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont, const FTransform& Align);

	void GeneratePoseMatchInfos(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont);

	void GeneratePoseMatchInfo(
		USkeleton* pSkel, FCompactPose& PrevPose, FCompactPose& CurPose, FPoseMatchInfo& OutInfo);

	void DrawPoseMatchInfo(UAnimSequence* pAnimSeq);
	void DrawPoseMatchInfo(const TArray<FPoseMatchInfo>& MatchInfos);

	int32 SearchBestMatchFrame(const UAnimSequence* pNextAnim);

	float CalcMatchCost(const FPoseMatchInfo& CurMatchInfo, const FPoseMatchInfo& NextMatchInfo);

	void AnalyzeMotion(const FPoseMatchInfo& MatchInfo);

	bool CanBeTransition(float HalfBlendTime);

	bool CreateNextPlayer(FAnimPlayer& OutPlayer, const FBoneContainer& RequiredBones);

	void CalcHitDir(UAnimSequence* pAnim, int32 BeginFrame, int32 EndFrame, const FBoneContainer& RequiredBones);

	FVector CalcBoneCSLocation(const UAnimSequence* pAnimSeq, float AnimTime, const FName& BoneName, const FBoneContainer& BoneCont);

	void GenerateBallTrajectory(TArray<FVector>& OutTrajectory, const FVector& BeginPos, const FVector& BeginVe);

	//void SavePoseMatchInfos(const FString& FilePath);
	//void LoadPoseMatchInfos(const FString& FilePath);

protected:

	TMap<UAnimSequence*, TArray<int32>> AnimHitFrames;
	TMap<UAnimSequence*, TArray<int32>> AnimMatchFrames;
	TMap<UAnimSequence*, TArray<FHitSection>> AnimHitSections;
	TMap<UAnimSequence*, TArray<FPoseMatchInfo>> AnimPoseInfos;

	TArray<FAnimPlayer> AnimPlayers;

	FCompactPose ResultPose;

	TArray<FVector> NextHitVels;
	TArray<FVector> NextHitPoss;

	TArray<TArray<FVector>> BallTrajectories;

	FHitSection SelectedHitSec;

	bool bIdleState;
};

