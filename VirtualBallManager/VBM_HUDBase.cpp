// Fill out your copyright notice in the Description page of Project Settings.

#include "VBM_HUDBase.h"
#include "Animation/AnimInstance.h"
#include "VirtualBallManagerGameModeBase.h"


#pragma optimize("", off)


//-------------------------------------------------------------------------------------------------
void AVBM_HUDBase::DrawHUD()
{
	Super::DrawHUD();
	
	FVector2D ViewSize;
	GWorld->GetGameViewport()->GetViewportSize(ViewSize);

	float MidY = ViewSize.Y * 0.5f;
	/*
	if (BoneVels.Num() > 0)
	{
		for (int32 IdxVel = 1; IdxVel < BoneVels.Num(); ++IdxVel)
		{
			float X1 = (IdxVel - 1) * 2.f;
			float X2 = (IdxVel    ) * 2.f;

			float Y1 = MidY - BoneVels[IdxVel - 1] * ScaleY;
			float Y2 = MidY - BoneVels[IdxVel    ] * ScaleY;

			DrawLine(X1, Y1, X2, Y2, FLinearColor::Black, 1.f);
		}

		float LimitY = MidY - LimitVel * ScaleY;

		DrawLine(0, LimitY, ViewSize.X, LimitY, FLinearColor::Red);
		DrawLine(0, MidY, ViewSize.X, MidY, FLinearColor::White);

		for (int32 HitFrame : HitFrames)
		{
			float X = (HitFrame - 1) * 2.f;
			float Y = MidY - BoneVels[HitFrame - 1] * ScaleY;

			DrawRect(FLinearColor::Red, X - 3, Y - 3, 7, 7);
		}
	}
	*/
	
	AVirtualBallManagerGameModeBase* pGameMode = GWorld->GetAuthGameMode<AVirtualBallManagerGameModeBase>();
	if (pGameMode == NULL || pGameMode->PoseVelList.Num() == 0)
		return;

	/*int32 NumPoses = pGameMode->PoseVelList.Num();
	for (int32 PoseIndex = 1; PoseIndex < NumPoses; ++PoseIndex)
	{
		const TArray<FVector>& PreVels = pGameMode->PoseVelList[PoseIndex - 1];
		const TArray<FVector>& CurVels = pGameMode->PoseVelList[PoseIndex];

		float Y1 = MidY - PreVels[18].Size();
		float Y2 = MidY - CurVels[18].Size();

		float LimitY = MidY - pGameMode->LimitSpeed;

		DrawLine((PoseIndex - 1) * 10.f, Y1, PoseIndex * 10.f, Y2, FColor::White);
		DrawLine(0, LimitY, ViewSize.X, LimitY, FLinearColor::Red);
		DrawLine(0, MidY, ViewSize.X, MidY, FLinearColor::Gray);
	}

	float LimitMinY = ViewSize.Y - pGameMode->LimitMinHeight * ScaleY;
	float LimitMaxY = ViewSize.Y - pGameMode->LimitMaxHeight * ScaleY;

	DrawLine(0, LimitMinY, ViewSize.X, LimitMinY, FLinearColor::Blue);
	DrawLine(0, LimitMaxY, ViewSize.X, LimitMaxY, FLinearColor::Red);*/

	int32 NumPoses = pGameMode->PosePosList.Num();
	for (int32 PoseIndex = 1; PoseIndex < NumPoses; ++PoseIndex)
	{
		const FVector& PreBonePos = pGameMode->PosePosList[PoseIndex - 1][18];
		const FVector& CurBonePos = pGameMode->PosePosList[PoseIndex][18];

		float Y1 = ViewSize.Y - PreBonePos.Z * ScaleY;
		float Y2 = ViewSize.Y - CurBonePos.Z * ScaleY;

		DrawLine((PoseIndex - 1) * 10.f, Y1, PoseIndex * 10.f, Y2, FColor::White);
	}

	for (int32 PoseIndex = 1; PoseIndex < NumPoses; ++PoseIndex)
	{
		const FVector& PreBonePos = pGameMode->PosePosList[PoseIndex - 1][14];
		const FVector& CurBonePos = pGameMode->PosePosList[PoseIndex][14];

		float Y1 = ViewSize.Y - PreBonePos.Z * ScaleY;
		float Y2 = ViewSize.Y - CurBonePos.Z * ScaleY;

		DrawLine((PoseIndex - 1) * 10.f, Y1, PoseIndex * 10.f, Y2, FColor::Yellow);
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
	//int32 RFootIndex = RefSkel.FindBoneIndex(FName("Right_Ankle_Joint_01"));
	int32 BoneIndex = RefSkel.FindBoneIndex(BoneName);
	if (BoneIndex < 0)
		return;

	SearchHitFrames(BoneIndex, pAnimInst->GetRequiredBones());
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