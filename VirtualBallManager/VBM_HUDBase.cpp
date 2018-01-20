// Fill out your copyright notice in the Description page of Project Settings.

#include "VBM_HUDBase.h"
#include "Animation/AnimInstance.h"


#pragma optimize("", off)


//-------------------------------------------------------------------------------------------------
void AVBM_HUDBase::DrawHUD()
{
	Super::DrawHUD();

	if (BoneVels.Num() > 0)
	{
		for (int32 IdxVel = 1; IdxVel < BoneVels.Num(); ++IdxVel)
		{
			float X1 = (IdxVel - 1) * 2.f;
			float X2 = (IdxVel    ) * 2.f;

			float Y1 = GraphHeight - BoneVels[IdxVel - 1] * ScaleY;
			float Y2 = GraphHeight - BoneVels[IdxVel    ] * ScaleY;

			DrawLine(X1, Y1, X2, Y2, FLinearColor::Black, 1.f);

			float LimitY = GraphHeight - LimitVel * ScaleY;

			DrawLine(0, LimitY, 10000, LimitY, FLinearColor::Gray);
		}

		for (int32 HitFrame : HitFrames)
		{
			float X = (HitFrame - 1) * 2.f;
			float Y = GraphHeight - BoneVels[HitFrame - 1] * ScaleY;

			DrawRect(FLinearColor::Red, X - 3, Y - 3, 7, 7);
		}
	}
}

//-------------------------------------------------------------------------------------------------
void AVBM_HUDBase::AnalyzeMotion(const UAnimInstance* pAnimInst)
{
	if (BoneVels.Num() > 0 || HitFrames.Num() > 0)
		return;

	if (Anim == NULL || pAnimInst == NULL)
		return;

	const FReferenceSkeleton& RefSkel = pAnimInst->CurrentSkeleton->GetReferenceSkeleton();

	//int32 LFootIndex = RefSkel.FindBoneIndex(FName("Left_Ankle_Joint_01"));
	int32 RFootIndex = RefSkel.FindBoneIndex(FName("Right_Ankle_Joint_01"));

	SearchHitFrames(RFootIndex, pAnimInst->GetRequiredBones());
}

//-------------------------------------------------------------------------------------------------
void AVBM_HUDBase::SearchHitFrames(int32 BoneIndex, const FBoneContainer& BoneContainer)
{
	FCompactPoseBoneIndex PoseBone(BoneIndex);

	FBlendedCurve EmptyCurve;
	FCSPose<FCompactPose> CSPose;

	TArray<FVector> BonePosList;

	FCompactPose AnimPose;
	AnimPose.SetBoneContainer(&BoneContainer);

	for (int32 Frame = 0; Frame < Anim->NumFrames; ++Frame)
	{
		float Time = Anim->GetTimeAtFrame(Frame);
		Anim->GetAnimationPose(AnimPose, EmptyCurve, FAnimExtractContext(Time));

		CSPose.InitPose(AnimPose);

		FVector BonePos = CSPose.GetComponentSpaceTransform(PoseBone).GetTranslation();

		BonePosList.Add(BonePos);
	}

	for (int32 Frame = 1; Frame < Anim->NumFrames; ++Frame)
	{
		float BoneVel = (BonePosList[Frame] - BonePosList[Frame - 1]).Size();

		BoneVels.Add(BoneVel);
	}

	TArray<int32> LocalMaxIndices;

	for (int32 IdxVel = 1; IdxVel < BoneVels.Num() - 1; ++IdxVel)
	{
		if (BoneVels[IdxVel] > LimitVel &&
			BoneVels[IdxVel] > BoneVels[IdxVel - 1] &&
			BoneVels[IdxVel] > BoneVels[IdxVel + 1])
		{
			LocalMaxIndices.Add(IdxVel);
		}
	}

	for (int32 IdxVel = 1; IdxVel < LocalMaxIndices.Num(); ++IdxVel)
	{
		int32 BeginIndex = LocalMaxIndices[IdxVel - 1];
		int32 EndIndex = LocalMaxIndices[IdxVel];

		TArray<int32> LocalMinIndices;
		for (int32 IdxVel = BeginIndex + 1; IdxVel < EndIndex; ++IdxVel)
		{
			if (BoneVels[IdxVel] < LimitVel &&
				BoneVels[IdxVel] < BoneVels[IdxVel - 1] &&
				BoneVels[IdxVel] < BoneVels[IdxVel + 1])
			{
				LocalMinIndices.Add(IdxVel);
			}
		}

		if (LocalMinIndices.Num() == 1)
		{
			HitFrames.Add(LocalMinIndices[0] + 1);
		}
	}
}

//-------------------------------------------------------------------------------------------------
void AVBM_HUDBase::SearchMatchFrames()
{
	
}


#pragma optimize("", on)