// Fill out your copyright notice in the Description page of Project Settings.

#include "VBM_AnimInstance.h"
#include "DrawDebugHelpers.h"


#pragma optimize("", off)


//-------------------------------------------------------------------------------------------------
void UVBM_AnimInstance::NativeInitializeAnimation()
{
	if (GetWorld()->IsPlayInEditor() == false)
		return;

	if (Anims.Num() != AnimPoseInfos.Num())
	{
		for (const UAnimSequence* pAnim : Anims)
		{
			if (pAnim != NULL)
			{
				GeneratePoseMatchInfos(pAnim);
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
void UVBM_AnimInstance::ParallelEvaluateAnimation(
	bool bForceRefPose, const USkeletalMesh* InSkeletalMesh,
	TArray<FTransform>& OutBoneSpaceTransforms, FBlendedHeapCurve& OutCurve, FCompactPose& OutPose)
{
	Super::ParallelEvaluateAnimation(bForceRefPose, InSkeletalMesh, OutBoneSpaceTransforms, OutCurve, OutPose);
}


void UVBM_AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
}

void UVBM_AnimInstance::NativePostEvaluateAnimation()
{
	
}

//-------------------------------------------------------------------------------------------------
void UVBM_AnimInstance::GeneratePoseMatchInfos(const UAnimSequence* pAnim)
{
	const FBoneContainer& BoneContainer = GetRequiredBones();
	if (BoneContainer.IsValid() == false)
		return;

	FCompactPose PrevAnimPose;
	PrevAnimPose.SetBoneContainer(&BoneContainer);

	FCompactPose CurAnimPose;
	CurAnimPose.SetBoneContainer(&BoneContainer);

	FBlendedCurve EmptyCurve;

	TArray<FPoseMatchInfo> MatchInfos;

	int32 NumFrames = pAnim->NumFrames;
	for (int32 IdxFrame = 1; IdxFrame < NumFrames; ++IdxFrame)
	{
		float PrevAnimTime = pAnim->GetTimeAtFrame(IdxFrame - 1);
		float CurAnimTime = pAnim->GetTimeAtFrame(IdxFrame);

		pAnim->GetBonePose(PrevAnimPose, EmptyCurve, FAnimExtractContext(PrevAnimTime));
		pAnim->GetBonePose(CurAnimPose, EmptyCurve, FAnimExtractContext(CurAnimTime));

		FPoseMatchInfo MatchInfo;
		GeneratePoseMatchInfo(MatchInfo, PrevAnimPose, CurAnimPose);

		MatchInfos.Add(MatchInfo);
	}

	if (MatchInfos.Num() > 0)
	{
		AnimPoseInfos.Add(pAnim, MatchInfos);
	}
}

//-------------------------------------------------------------------------------------------------
void UVBM_AnimInstance::GeneratePoseMatchInfo(
	FPoseMatchInfo& OutInfo, const FCompactPose& PrevPose, const FCompactPose& CurPose)
{
	const FReferenceSkeleton& RefSkel = CurrentSkeleton->GetReferenceSkeleton();

	int32 RootIndex = RefSkel.FindBoneIndex(FName("Root"));
	int32 LFootIndex = RefSkel.FindBoneIndex(FName("Left_Ankle_Joint_01"));
	int32 RFootIndex = RefSkel.FindBoneIndex(FName("Right_Ankle_Joint_01"));
	int32 LHandIndex = RefSkel.FindBoneIndex(FName("Left_Wrist_Joint_01"));
	int32 RHandIndex = RefSkel.FindBoneIndex(FName("Right_Wrist_Joint_01"));

	FCompactPoseBoneIndex RootPoseIndex = GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(RootIndex);
	FCompactPoseBoneIndex LFootPoseIndex = GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(LFootIndex);
	FCompactPoseBoneIndex RFootPoseIndex = GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(RFootIndex);
	FCompactPoseBoneIndex LHandPoseIndex = GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(LHandIndex);
	FCompactPoseBoneIndex RHandPoseIndex = GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(RHandIndex);

	FCSPose<FCompactPose> PrevCSPose;
	PrevCSPose.InitPose(PrevPose);

	FCSPose<FCompactPose> CurCSPose;
	CurCSPose.InitPose(CurPose);

	FVector PrevRootPos = PrevCSPose.GetComponentSpaceTransform(RootPoseIndex).GetTranslation();
	FVector PrevLFootPos = PrevCSPose.GetComponentSpaceTransform(LFootPoseIndex).GetTranslation();
	FVector PrevRFootPos = PrevCSPose.GetComponentSpaceTransform(RFootPoseIndex).GetTranslation();
	FVector PrevLHandPos = PrevCSPose.GetComponentSpaceTransform(LHandPoseIndex).GetTranslation();
	FVector PrevRHandPos = PrevCSPose.GetComponentSpaceTransform(RHandPoseIndex).GetTranslation();

	FVector CurRootPos = CurCSPose.GetComponentSpaceTransform(RootPoseIndex).GetTranslation();
	FVector CurLFootPos = CurCSPose.GetComponentSpaceTransform(LFootPoseIndex).GetTranslation();
	FVector CurRFootPos = CurCSPose.GetComponentSpaceTransform(RFootPoseIndex).GetTranslation();
	FVector CurLHandPos = CurCSPose.GetComponentSpaceTransform(LHandPoseIndex).GetTranslation();
	FVector CurRHandPos = CurCSPose.GetComponentSpaceTransform(RHandPoseIndex).GetTranslation();

	FVector CurFrontDir;
	{
		int32 HipIndex = RefSkel.FindBoneIndex(FName("Hip"));
		int32 LThighIndex = RefSkel.FindBoneIndex(FName("Left_Thigh_Joint_01"));
		int32 RThighIndex = RefSkel.FindBoneIndex(FName("Right_Thigh_Joint_01"));

		FCompactPoseBoneIndex HipPoseIndex = GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(HipIndex);
		FCompactPoseBoneIndex LThighPoseIndex = GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(LThighIndex);
		FCompactPoseBoneIndex RThighPoseIndex = GetRequiredBones().GetCompactPoseIndexFromSkeletonIndex(RThighIndex);

		FVector HipPos = CurCSPose.GetComponentSpaceTransform(HipPoseIndex).GetTranslation();
		FVector LThighPos = CurCSPose.GetComponentSpaceTransform(LThighPoseIndex).GetTranslation();
		FVector RThighPos = CurCSPose.GetComponentSpaceTransform(RThighPoseIndex).GetTranslation();

		FVector Dir1 = LThighPos - HipPos;
		FVector Dir2 = RThighPos - HipPos;

		CurFrontDir = (Dir1 ^ Dir2).GetSafeNormal2D();
	}
	
	float AlignedDegree;
	FTransform AlignTransform;
	{
		FVector AlignPos = CurPose[RootPoseIndex].GetTranslation();
		AlignPos.Z = 0.f;

		FQuat AlignQuat = FQuat::FindBetween(CurFrontDir, FVector::ForwardVector);

		AlignedDegree = -AlignQuat.Rotator().Yaw;
		AlignTransform = FTransform(-AlignPos) * FTransform(AlignQuat);
	}
	
	FPoseMatchInfo PoseMatchInfo;
	{
		PoseMatchInfo.AlignedDegree = AlignedDegree;

		PoseMatchInfo.RootPos = AlignTransform.TransformPosition(CurRootPos);
		PoseMatchInfo.LeftFootPos = AlignTransform.TransformPosition(CurLFootPos);
		PoseMatchInfo.RightFootPos = AlignTransform.TransformPosition(CurRFootPos);
		PoseMatchInfo.LeftHandPos = AlignTransform.TransformPosition(CurLHandPos);
		PoseMatchInfo.RightHandPos = AlignTransform.TransformPosition(CurRHandPos);

		PoseMatchInfo.RootVel = AlignTransform.TransformVector(CurRootPos - PrevRootPos);
		PoseMatchInfo.LeftFootVel = AlignTransform.TransformVector(CurLFootPos - PrevLFootPos);
		PoseMatchInfo.RightFootVel = AlignTransform.TransformVector(CurRFootPos - PrevRFootPos);
		PoseMatchInfo.LeftHandVel = AlignTransform.TransformVector(CurLHandPos - PrevLHandPos);
		PoseMatchInfo.RightHandVel = AlignTransform.TransformVector(CurRHandPos - PrevRHandPos);
	}

	OutInfo = PoseMatchInfo;
}

//-------------------------------------------------------------------------------------------------
void UVBM_AnimInstance::DrawPoseMatchInfo(int32 AnimIndx, int32 FrameStep)
{
	TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(Anims[AnimIndx]);
	if (pMatchInfos == NULL)
		return;

	DrawPoseMatchInfo(*pMatchInfos, FrameStep);
}

//-------------------------------------------------------------------------------------------------
void UVBM_AnimInstance::DrawPoseMatchInfo(const TArray<FPoseMatchInfo>& MatchInfos, int32 FrameStep)
{
	int32 Frame = 0;

	for (auto& MatchInfo : MatchInfos)
	{
		++Frame;
		if (Frame % FrameStep > 0)
			continue;

		DrawDebugLine(GWorld, MatchInfo.RootPos, MatchInfo.RootPos + MatchInfo.RootVel, FColor::Black);
		DrawDebugLine(GWorld, MatchInfo.LeftFootPos, MatchInfo.LeftFootPos + MatchInfo.LeftFootVel, FColor::Magenta);
		DrawDebugLine(GWorld, MatchInfo.RightFootPos, MatchInfo.RightFootPos + MatchInfo.RightFootVel, FColor::Cyan);
		DrawDebugLine(GWorld, MatchInfo.LeftHandPos, MatchInfo.LeftHandPos + MatchInfo.LeftHandVel, FColor::Red);
		DrawDebugLine(GWorld, MatchInfo.RightHandPos, MatchInfo.RightHandPos + MatchInfo.RightHandVel, FColor::Blue);

		DrawDebugPoint(GWorld, MatchInfo.RootPos, 5, FColor::Black);
		DrawDebugPoint(GWorld, MatchInfo.LeftFootPos, 5, FColor::Magenta);
		DrawDebugPoint(GWorld, MatchInfo.RightFootPos, 5, FColor::Cyan);
		DrawDebugPoint(GWorld, MatchInfo.LeftHandPos, 5, FColor::Red);
		DrawDebugPoint(GWorld, MatchInfo.RightHandPos, 5, FColor::Blue);
	}
}


#pragma optimize("", on)