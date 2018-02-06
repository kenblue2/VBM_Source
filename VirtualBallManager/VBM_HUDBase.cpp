// Fill out your copyright notice in the Description page of Project Settings.

#include "VBM_HUDBase.h"
#include "VBM_Pawn.h"
#include "AnimNode_VBM.h"
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
	float DeltaX = float(ViewSize.X) / float(Anim->NumFrames);

	int32 AnimIndex = 0;

	auto PawnIt = GWorld->GetPawnIterator();
	AVBM_Pawn* pVBMPawn = Cast<AVBM_Pawn>(PawnIt->Get());
	if (pVBMPawn && pVBMPawn->pAnimNode)
	{
		FAnimNode_VBM* pNode = pVBMPawn->pAnimNode;

		float Y1 = ViewSize.Y - Anim->LimitMinPos * Anim->GraphScale;
		float Y2 = ViewSize.Y - Anim->LimitMaxPos * Anim->GraphScale;

		if (ShowPosGraph && ShowVelGraph == false)
		{
			//DrawLine(0, Y1, ViewSize.X, Y1, FLinearColor::Gray);
			DrawLine(0, Y2, ViewSize.X, Y2, FLinearColor::Gray);
		}

		TArray<FPoseMatchInfo>* pMatchInfos = pNode->AnimPoseInfos.Find(Anim);
		if (pMatchInfos)
		{
			if (ShowPosGraph || ShowCandidate)
			{
				if (Anim->BoneName == FName("Left_Ankle_Joint_01"))
				{
					for (int32 PoseIndex = 1; PoseIndex < pMatchInfos->Num(); ++PoseIndex)
					{
						const FVector& PreBonePos = (*pMatchInfos)[PoseIndex - 1].LeftFootPos;
						const FVector& CurBonePos = (*pMatchInfos)[PoseIndex].LeftFootPos;

						float Y1 = ViewSize.Y - PreBonePos.Z * Anim->GraphScale - 40.f;
						float Y2 = ViewSize.Y - CurBonePos.Z * Anim->GraphScale - 40.f;

						DrawLine((PoseIndex - 1) * DeltaX, Y1, PoseIndex * DeltaX, Y2, FColor::Black, 1);
					}
				}
				else if (Anim->BoneName == FName("Right_Ankle_Joint_01"))
				{
					for (int32 PoseIndex = 1; PoseIndex < pMatchInfos->Num(); ++PoseIndex)
					{
						const FVector& PreBonePos = (*pMatchInfos)[PoseIndex - 1].RightFootPos;
						const FVector& CurBonePos = (*pMatchInfos)[PoseIndex].RightFootPos;

						float Y1 = ViewSize.Y - PreBonePos.Z * Anim->GraphScale - 40.f;
						float Y2 = ViewSize.Y - CurBonePos.Z * Anim->GraphScale - 40.f;

						DrawLine((PoseIndex - 1) * DeltaX, Y1, PoseIndex * DeltaX, Y2, FColor::Black, 1);
					}
				}
			}
			
			if (ShowVelGraph)
			{
				if (Anim->BoneName == FName("Left_Ankle_Joint_01"))
				{
					for (int32 PoseIndex = 1; PoseIndex < pMatchInfos->Num(); ++PoseIndex)
					{
						const FVector& PreBonePos = (*pMatchInfos)[PoseIndex - 1].LeftFootVel;
						const FVector& CurBonePos = (*pMatchInfos)[PoseIndex].LeftFootVel;

						float Y1 = ViewSize.Y - PreBonePos.Size() / 10.f * Anim->GraphScale - 40.f;
						float Y2 = ViewSize.Y - CurBonePos.Size() / 10.f * Anim->GraphScale - 40.f;

						DrawLine((PoseIndex - 1) * DeltaX, Y1, PoseIndex * DeltaX, Y2, FColor::Purple, 1);
					}
				}
				else if (Anim->BoneName == FName("Right_Ankle_Joint_01"))
				{
					for (int32 PoseIndex = 1; PoseIndex < pMatchInfos->Num(); ++PoseIndex)
					{
						const FVector& PreBonePos = (*pMatchInfos)[PoseIndex - 1].RightFootVel;
						const FVector& CurBonePos = (*pMatchInfos)[PoseIndex].RightFootVel;

						float Y1 = ViewSize.Y - PreBonePos.Size() / 10.f * Anim->GraphScale - 40.f;
						float Y2 = ViewSize.Y - CurBonePos.Size() / 10.f * Anim->GraphScale - 40.f;

						DrawLine((PoseIndex - 1) * DeltaX, Y1, PoseIndex * DeltaX, Y2, FColor::Purple, 1);
					}
				}
			}
		}

		TArray<FMotionClip>* pMotionClips = pNode->AnimMotionClips.Find(Anim);
		if (pMotionClips)
		{
			for (int32 IdxClip = 0; IdxClip < pMotionClips->Num(); ++IdxClip)
			{
				float BeginPos2 = (*pMotionClips)[IdxClip].HitBeginFrame;
				float EndPos2 = (*pMotionClips)[IdxClip].HitEndFrame;

				if (ShowVelGraph)
				{
					DrawLine(BeginPos2 * DeltaX, 0, BeginPos2 * DeltaX, ViewSize.Y, FColor::Magenta);
				}
				
				if (ShowPosGraph)
				{
					DrawLine(EndPos2 * DeltaX, 0, EndPos2 * DeltaX, ViewSize.Y, FColor::Red);
				}

				if (ShowVelGraph && ShowPosGraph)
				{
					DrawLine(BeginPos2 * DeltaX, ViewSize.Y - 20.f, EndPos2 * DeltaX, ViewSize.Y - 20.f, FColor::Red, 7);
				}
			}

			if (ShowVelGraph && ShowPosGraph)
			{
				DrawLine(0, ViewSize.Y - 20.f, ViewSize.X, ViewSize.Y - 20.f, FColor::Black, 3);
			}

			if (ShowCandidate)
			{
				float Y1 = ViewSize.Y - 20.f;

				for (int32 IdxClip = 0; IdxClip < pMotionClips->Num(); ++IdxClip)
				{
					float BeginPos2 = (*pMotionClips)[IdxClip].MoveBeginFrame;
					float EndPos2 = (*pMotionClips)[IdxClip].MoveEndFrame;

					float X1 = BeginPos2 * DeltaX;
					float X2 = EndPos2 * DeltaX;

					DrawLine(X1, 0, X1, ViewSize.Y, FLinearColor::Gray);
					DrawLine(X2, 0, X2, ViewSize.Y, FLinearColor::Gray);

					DrawLine(X1, Y1, X2, Y1, FColor::Blue, 7);
				}

				DrawLine(0, Y1, ViewSize.X, Y1, FColor::Black, 3);
			}

			if (ShowMotionClip)
			{
				for (int32 IdxClip = 0; IdxClip < pMotionClips->Num(); ++IdxClip)
				{
					float BeginPos = (*pMotionClips)[IdxClip].MoveBeginFrame;
					float EndPos = (*pMotionClips)[IdxClip].MoveEndFrame;

					float X1 = BeginPos * DeltaX;
					float X2 = EndPos * DeltaX;

					float Y1 = ViewSize.Y - 200.f;
					float Y2 = ViewSize.Y - 40.f;

					DrawRect(FLinearColor::Red, X1, Y1, X2 - X1, Y2 - Y1);
				}
			}
		}
	}
	
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