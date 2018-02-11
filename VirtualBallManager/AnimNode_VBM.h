#pragma once

#include "CoreMinimal.h"
#include "PoseMatchInfo.h"
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
	float LimitMaxHeight = 40.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float LimitMinHeight = 15.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	bool ShowDebugInfo = false;

	UPROPERTY(EditAnywhere, Category = Settings)
	float AirReistance = 0.1f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float BallElasicity = 0.8f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float Threshold = 1.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float RotRatio = 0.5f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float BallRadius = 17.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float MoveWeight = 1.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float HitAngleWeight = 100.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float MoveAngleWeight = 100.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float DistWeight = 1.f;
	

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

	void CreateNextPlayer(class AVBM_Pawn* pPawn, int32 MotionType);
	void CreateNextPlayer(class AVBM_Pawn* pPawn, bool bUseLeftFoot, int32 MotionType);
	void CreateNextPlayer(class AVBM_Pawn* pPawn, const TArray<FVector>& FootTrajectory, bool bUseLeftFoot);

	void PlayHitMotion(class AVBM_Pawn* pPawn);

protected:

	void AlignPose(USkeleton* pSkel, FCompactPose& AnimPose, FQuat* pOutQuat = NULL);

	FVector CalcFrontDir(const FCompactPose& AnimPose, FVector* pOutHipPos = NULL);

	void DrawPose(const FCompactPose& Pose, const FColor& Color);

	void DrawAnimPoses(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont);
	void DrawAnimPoses(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont, const FTransform& Align);

	void GeneratePoseMatchInfos(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont);

	void GeneratePoseMatchInfo(FCompactPose& PrevPose, FCompactPose& CurPose, FPoseMatchInfo& OutInfo);

	void DrawPoseMatchInfo(UAnimSequence* pAnimSeq);
	void DrawPoseMatchInfo(const TArray<FPoseMatchInfo>& MatchInfos);

	int32 SearchBestMatchFrame(const UAnimSequence* pNextAnim);

	float CalcMatchCost(const FPoseMatchInfo& CurMatchInfo, const FPoseMatchInfo& NextMatchInfo);

	void AnalyzeMotion(const FPoseMatchInfo& IdleMatchInfo, const FBoneContainer& RequiredBones);

	bool CanBeTransition();

	bool CreateNextPlayer(FAnimPlayer& OutPlayer, const FBoneContainer& RequiredBones);
	bool CreateNextPlayer(FAnimPlayer& OutPlayer, const FBoneContainer& RequiredBones, const class AVBM_Pawn* pPawn);

	void CalcHitDir(UAnimSequence* pAnim, int32 BeginFrame, int32 EndFrame, const FBoneContainer& RequiredBones);

	FVector CalcHitDir(UAnimSequence* pAnim, const FMotionClip& Clip, const FBoneContainer& RequiredBones);

	FVector CalcBoneCSLocation(const UAnimSequence* pAnimSeq, float AnimTime, const FName& BoneName, const FBoneContainer& BoneCont);
	FTransform CalcBoneCSTransform(const UAnimSequence* pAnimSeq, float AnimTime, const FName& BoneName, const FBoneContainer& BoneCont);

	void GenerateBallTrajectory(TArray<FVector>& OutTrajectory, const FVector& BeginPos, const FVector& BeginVe);
	void GenerateBallTrajectory(TArray<FVector>& OutTrajectory, const FVector& BeginPos, const FVector& BeginVel, const FVector& EndPos);

	void CreateNextHitInfo(const FAnimPlayer& NextPlayer, AVBM_Pawn* pPawn, const FBoneContainer& RequiredBones);

	FTransform CalcAlignTransoform(const FAnimPlayer& NextPlayer, const FBoneContainer& RequiredBones);

	void AdjustBallTrajectory(TArray<FVector>& OutTrajectory, const FVector& BeginVel, const FVector& EndPos, int32 LimitFrame);

	FVector GetMaxHitVel(UAnimSequence* pAnim, const FMotionClip& Clip, const FBoneContainer& RequiredBones);

	//void SavePoseMatchInfos(const FString& FilePath);
	//void LoadPoseMatchInfos(const FString& FilePath);

protected:

	TArray<FAnimPlayer> AnimPlayers;

	FCompactPose ResultPose;

	TArray<FVector> NextHitVels;
	TArray<FVector> NextHitPoss;
	TArray<FVector> NextAxisAngs;

	TArray<TArray<FVector>> BallTrajectories;

	FAnimSection SelectedHitSec;

	FAnimPlayer NextAnimPlayer;

	FBoneContainer BoneContainer;

public:

	TMap<UAnimSequence*, TArray<int32>> AnimMatchFrames;
	TMap<UAnimSequence*, TArray<FAnimSection>> AnimHitSections;
	TMap<UAnimSequence*, TArray<FPoseMatchInfo>> AnimPoseInfos;
	TMap<UAnimSequence*, TArray<FMotionClip>> AnimMotionClips;

	bool bMoveBall;
	bool bIdleState;

	float BallTime;
	float BallEndTime;
	float BallBeginTime;
	float BallRestTime;

	FVector BallPos;

	TArray<FVector> PassTrajectory;
	TArray<FVector> PassTrajectory2;

	TArray<FVector> UserTrajectory;
	TArray<FVector> BestTrajectory;
	
	TArray<FPoseMatchInfo> BestPoseMatchInfos;

private:

	FVector NextHitFrontDir;

	FVector FootMoveDir1;
	FVector FootMoveDir2;

	FVector UserMoveDir1;
	FVector UserMoveDir2;
};

