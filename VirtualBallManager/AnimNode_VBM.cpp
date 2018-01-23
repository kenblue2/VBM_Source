#include "AnimNode_VBM.h"
#include "AnimationRuntime.h"
#include "VBM_AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "VBM_Pawn.h"
#include "DrawDebugHelpers.h"

#pragma optimize("", off)

const float GRAVITY = 490.f;


//-------------------------------------------------------------------------------------------------
// FAnimNode_VBM
FAnimNode_VBM::FAnimNode_VBM()
	: BallTime(-1.f)
	, bHitBall(false)
	, bIdleState(false)
{
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);

	BoneContainer = Context.AnimInstanceProxy->GetRequiredBones();
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);
	Result.Update(Context);

	// update animplayers
	if (bIdleState == false)
	{
		float DeltaTime = Context.GetDeltaTime();

		for (auto& Player : AnimPlayers)
		{
			Player.Update(DeltaTime);
		}

		if (AnimPlayers.Num() > 0)
		{
			const FAnimPlayer& CurPlayer = AnimPlayers.Last();

			float EndTime = CurPlayer.pAnim->GetTimeAtFrame(CurPlayer.MotionClip.EndFrame);

			if (CurPlayer.Time > EndTime)
			{
				bIdleState = true;
			}

			if (AnimPlayers[0].bStop && AnimPlayers[0].Weight <= 0.f)
			{
				AnimPlayers.RemoveAt(0);
			}
		}
	}

	if (PassTrajectory2.Num() > 0)
	{
		BallTime += Context.GetDeltaTime();
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::Evaluate_AnyThread(FPoseContext& Output)
{
	Result.Evaluate(Output);

	ResultPose.CopyBonesFrom(Output.Pose);

	if (AnimPlayers.Num() > 0)
	{
		TArray<FCompactPose> AnimPoses;
		TArray<FBlendedCurve> EmptyCurves;
		TArray<float> AnimWeights;

		FCompactPose AnimPose;
		AnimPose.CopyBonesFrom(Output.Pose);

		FBlendedCurve EmptyCurve;

		for (auto& Player : AnimPlayers)
		{
			Player.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(Player.Time));

			AnimPose[FCompactPoseBoneIndex(0)] *= Player.Align;

			AnimPoses.Add(AnimPose);
			AnimWeights.Add(Player.Weight);
		}

		FAnimationRuntime::BlendPosesTogether(AnimPoses, EmptyCurves, AnimWeights, ResultPose, EmptyCurve);

		if (ResultPose.IsValid())
		{
			Output.Pose = ResultPose;
		}
	}
}

//-------------------------------------------------------------------------------------------------
bool FAnimNode_VBM::CanBeTransition()
{
	if (AnimPlayers.Num() == 0)
		return false;

	FAnimPlayer CurPlayer = AnimPlayers.Last();

	if (CurPlayer.bStop || CurPlayer.pAnim == NULL)
		return false;

	float CurTime = CurPlayer.Time;
	float EndTime = CurPlayer.pAnim->GetTimeAtFrame(CurPlayer.MotionClip.EndFrame);

	if (CurTime < EndTime)
		return false;

	return true;
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::PreUpdate(const UAnimInstance* InAnimInstance)
{
	AVBM_Pawn* pVBMPawn = Cast<AVBM_Pawn>(InAnimInstance->TryGetPawnOwner());
	if (pVBMPawn == NULL)
		return;

	if (pVBMPawn->pAnimNode == NULL)
	{
		pVBMPawn->pAnimNode = this;
	}

	if (GWorld->IsPlayInEditor() == false)
		return;

	const FBoneContainer& RequiredBones = InAnimInstance->GetRequiredBones();
	if (RequiredBones.IsValid() == false)
		return;

	if (Anims.IsValidIndex(AnimIndex) == false)
		return;

	if (Anims.Num() != AnimPoseInfos.Num())
	{
		for (auto& pAnim : Anims)
		{
			if (pAnim != NULL)
			{
				GeneratePoseMatchInfos(pAnim, RequiredBones);

				FPoseMatchInfo& FirstMatchInfo = AnimPoseInfos.CreateIterator()->Value[0];

				AnalyzeMotion(FirstMatchInfo);
			}
		}
	}

	

	if (AnimPlayers.Num() <= 0)
	{
		TArray<UAnimSequence*> pAnims;
		AnimMotionClips.GetKeys(pAnims);

		if (pAnims.Num() > 0)
		{
			UAnimSequence* pAnim = pAnims[rand() % pAnims.Num()];
			TArray<FMotionClip>* pMotionClips = AnimMotionClips.Find(pAnim);

			if (pMotionClips != NULL && pMotionClips->Num() > 0)
			{
				FMotionClip CurClip = (*pMotionClips)[rand() % pMotionClips->Num()];
				//float BeginTime = pAnim->GetTimeAtFrame(CurClip.BeginFrame);
				float BeginTime = pAnim->GetTimeAtFrame(CurClip.EndFrame);

				FAnimPlayer AnimPlayer(pAnim, BeginTime, 0.3f, 1.f);
				AnimPlayer.MotionClip = CurClip;

				AnimPlayers.Add(AnimPlayer);
			}
		}
	}
	else if (PassTrajectory2.Num() > 0)
	{
		if (BallTime > 0.f)
		{
			pVBMPawn->bHitBall = true;
		}

		float BallEndTime = (float)PassTrajectory2.Num() * 0.033f;
		float RemainedTime = BallEndTime - BallTime;

		if (pVBMPawn->pDestPawn != NULL)
		{
			if (RemainedTime < pVBMPawn->pDestPawn->HitBallTime)
			{
				pVBMPawn->bBeginNextMotion = true;

				pVBMPawn->pDestPawn->TimeError = pVBMPawn->pDestPawn->HitBallTime - RemainedTime;
				//pVBMPawn->pDestPawn = NULL;
			}
		}

		

		if (BallTime > 0.f)
		{
			float fBallFrame = BallTime / 0.033f;
			int32 iBallFrame = (int32)fBallFrame;
			float Ratio = fBallFrame - iBallFrame;

			if (PassTrajectory2.IsValidIndex(iBallFrame) &&
				PassTrajectory2.IsValidIndex(iBallFrame + 1))
			{
				FVector BallPos = FMath::Lerp(PassTrajectory2[iBallFrame], PassTrajectory2[iBallFrame + 1], Ratio);

				DrawDebugSphere(GWorld, BallPos, 15.f, 16, FColor::Orange);
			}
			else if (iBallFrame > 0)
			{
				DrawDebugSphere(GWorld, PassTrajectory2.Last(), 15.f, 16, FColor::Orange);
			}
		}

		if (pVBMPawn->pDestPawn != NULL && ShowDebugInfo)
		{
			if (RemainedTime > 0.f)
			{
				for (int32 IdxPos = 1; IdxPos < PassTrajectory.Num(); ++IdxPos)
				{
					DrawDebugLine(GWorld, PassTrajectory[IdxPos - 1], PassTrajectory[IdxPos], FColor::Silver, false, -1.f, 0, 1.f);
				}

				for (int32 IdxPos = 1; IdxPos < PassTrajectory2.Num(); ++IdxPos)
				{
					DrawDebugLine(GWorld, PassTrajectory2[IdxPos - 1], PassTrajectory2[IdxPos], FColor::Red, false, -1.f, 0, 1.f);
				}
			}

			for (int32 IdxHit = 0; IdxHit < NextHitVels.Num(); ++IdxHit)
			{
				const FVector& HitPos = NextHitPoss[IdxHit];
				const FVector& HitVel = NextHitVels[IdxHit];

				DrawDebugPoint(GWorld, HitPos, 5.f, FColor::Cyan);
				DrawDebugLine(GWorld, HitPos, HitPos + HitVel, FColor::Magenta);
			}

			for (auto& BallTrajectory : BallTrajectories)
			{
				for (int32 IdxPos = 1; IdxPos < BallTrajectory.Num(); ++IdxPos)
				{
					DrawDebugLine(GWorld, BallTrajectory[IdxPos - 1], BallTrajectory[IdxPos], FColor::Black);
				}
			}

			DrawDebugLine(GWorld, pVBMPawn->PlayerPos, pVBMPawn->pDestPawn->PlayerPos, FColor::Yellow, false, -1.f, 0, 1.f);
			DrawDebugSphere(GWorld, pVBMPawn->pDestPawn->PlayerPos, 10.f, 16, FColor::Green);

			DrawDebugSphere(GWorld, pVBMPawn->HitBallPos, 3.f, 8, FColor::Red);
		}
	}

/*
	TArray<FVector> HitPosList;

	for (auto PawnIt = GWorld->GetPawnIterator(); PawnIt; ++PawnIt)
	{
		APawn* pPawn = PawnIt->Get();
		if (pPawn == NULL)
			continue;

		TArray<UActorComponent*> Components = pPawn->GetComponentsByClass(USkeletalMeshComponent::StaticClass());
		if (Components.Num() == 0)
			continue;

		USkeletalMeshComponent* pSkelComp = Cast<USkeletalMeshComponent>(Components[0]);
		if (pSkelComp == NULL)
			continue;

		FVector HitPos = pSkelComp->GetBoneLocation(FName("Right_Ankle_Joint_01"));
		DrawDebugSphere(GWorld, HitPos, 5.f, 8, FColor::Red);

		HitPosList.Add(HitPos);
	}
*/

	//FVector AnklePos = InAnimInstance->GetSkelMeshComponent()->GetBoneLocation(FName("Right_Ankle_Joint_01"));
	//FVector KneePos = InAnimInstance->GetSkelMeshComponent()->GetBoneLocation(FName("Right_Knee_Joint_01"));
	//FVector ToePos = InAnimInstance->GetSkelMeshComponent()->GetBoneLocation(FName("Right_Toe_Joint_01"));

	//FVector HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
	//FVector HitPos = InAnimInstance->GetSkelMeshComponent()->GetSocketLocation(FName("Right_Hit_Socket"));

	//DrawDebugLine(GWorld, HitPos, HitPos + HitDir * 100.f, FColor::Red);
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::CreateNextPlayer(AVBM_Pawn* pPawn, float DiffTime)
{
	FVector PlayerPos = pPawn->PlayerPos;
	FVector DestPos = pPawn->pDestPawn->PlayerPos;

	FVector PassDir = (DestPos - PlayerPos).GetSafeNormal2D();

	float MinCost = FLT_MAX;
	FAnimPlayer MinPlayer;

	// select motion clip
	for (auto& AnimMotionClip : AnimMotionClips)
	{
		UAnimSequence* pNextAnim = AnimMotionClip.Key;
		if (pNextAnim == NULL)
			continue;

		for (auto& MotionClip : AnimMotionClip.Value)
		{
			float BeginTime = pNextAnim->GetTimeAtFrame(MotionClip.BeginFrame);

			FAnimPlayer NextPlayer(pNextAnim, BeginTime);
			{
				NextPlayer.MotionClip = MotionClip;
				NextPlayer.Align = CalcAlignTransoform(NextPlayer, BoneContainer);
			}

			for (int32 Frame = MotionClip.HitBeginFrame; Frame < MotionClip.HitEndFrame; ++Frame)
			{
				float HitTime = pNextAnim->GetTimeAtFrame(Frame);

				FVector AnklePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Ankle_Joint_01"), BoneContainer);
				FVector KneePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Knee_Joint_01"), BoneContainer);
				FVector ToePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Toe_Joint_01"), BoneContainer);

				FVector HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
				HitDir = NextPlayer.Align.TransformVector(HitDir);

				TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pNextAnim);
				if (pMatchInfos != NULL)
				{
					float Speed = (*pMatchInfos)[Frame - 1].RightFootVel.Size();

					FVector HitVel = HitDir * Speed;

					TArray<FVector> Trajectory;
					GenerateBallTrajectory(Trajectory, pPawn->PlayerPos, HitVel);

					float Dist = (Trajectory.Last() - pPawn->pDestPawn->PlayerPos).Size();
					float Angle = FMath::Abs(FMath::Acos(PassDir | HitVel));

					float Cost = Angle * 10.f + Dist;

					if (MinCost > Cost)
					{
						MinCost = Cost;
						MinPlayer = NextPlayer;
					}
				}
			}
		}
	}

	NextAnimPlayer = MinPlayer;

	CreateNextHitInfo(NextAnimPlayer, pPawn, BoneContainer);

	//PassTrajectory2.Empty();
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::PlayHitMotion(AVBM_Pawn* pPawn, float DiffTime)
{
	bIdleState = false;

	DiffTime += 0.033f;

	NextAnimPlayer.Time += DiffTime;
	BallTime = BeginBallTime + DiffTime;

	AnimPlayers.Last().Stop();
	AnimPlayers.Add(NextAnimPlayer);

	GenerateBallTrajectory(PassTrajectory2, pPawn->HitBallPos, pPawn->HitBallVel);
	AdjustBallTrajectory(PassTrajectory2, pPawn->HitBallVel, pPawn->pDestPawn->HitBallPos);
}

//-------------------------------------------------------------------------------------------------
bool FAnimNode_VBM::CreateNextPlayer(FAnimPlayer& OutPlayer, const FBoneContainer& RequiredBones, const AVBM_Pawn* pPawn)
{
	if (pPawn == NULL || pPawn->pDestPawn == NULL)
		return false;

	FVector PlayerPos = pPawn->PlayerPos;
	FVector DestPos = pPawn->pDestPawn->PlayerPos;

	FVector PassDir = (DestPos - PlayerPos).GetSafeNormal2D();

	float MinCost = FLT_MAX;
	FAnimPlayer MinPlayer;

	// select motion clip
	for (auto& AnimMotionClip : AnimMotionClips)
	{
		UAnimSequence* pNextAnim = AnimMotionClip.Key;
		if (pNextAnim == NULL)
			continue;

		for (auto& MotionClip : AnimMotionClip.Value)
		{
			float BeginTime = pNextAnim->GetTimeAtFrame(MotionClip.BeginFrame);

			FAnimPlayer NextPlayer(pNextAnim, BeginTime);
			{
				NextPlayer.MotionClip = MotionClip;
				NextPlayer.Align = CalcAlignTransoform(NextPlayer, RequiredBones);
			}

			//FVector ClipHitDir = CalcHitDir(pNextAnim, MotionClip, RequiredBones);
			//ClipHitDir = NextPlayer.Align.TransformVector(ClipHitDir);

			//TArray<FVector> Trajectory;
			//{
			//	FVector MaxHitVel = GetMaxHitVel(pNextAnim, MotionClip, RequiredBones);
			//	MaxHitVel = NextPlayer.Align.TransformVector(MaxHitVel);

			//	GenerateBallTrajectory(Trajectory, pPawn->PlayerPos, MaxHitVel);
			//}

			for (int32 Frame = MotionClip.HitBeginFrame; Frame < MotionClip.HitEndFrame; ++Frame)
			{
				float HitTime = pNextAnim->GetTimeAtFrame(Frame);

				FVector AnklePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Ankle_Joint_01"), RequiredBones);
				FVector KneePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Knee_Joint_01"), RequiredBones);
				FVector ToePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Toe_Joint_01"), RequiredBones);

				FVector HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
				HitDir = NextPlayer.Align.TransformVector(HitDir);

				TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pNextAnim);
				if (pMatchInfos != NULL)
				{
					float Speed = (*pMatchInfos)[Frame - 1].RightFootVel.Size();

					FVector HitVel = HitDir * Speed;

					TArray<FVector> Trajectory;
					GenerateBallTrajectory(Trajectory, pPawn->PlayerPos, HitVel);

					float Dist = (Trajectory.Last() - pPawn->pDestPawn->PlayerPos).Size();
					float Angle = FMath::Abs(FMath::Acos(PassDir | HitVel));

					float Cost = Angle * 10.f + Dist;

					if (MinCost > Cost)
					{
						MinCost = Cost;
						MinPlayer = NextPlayer;
					}
				}
			}
		}
	}

	OutPlayer = MinPlayer;
	return true;
}

//-------------------------------------------------------------------------------------------------
FTransform FAnimNode_VBM::CalcAlignTransoform(const FAnimPlayer& NextPlayer, const FBoneContainer& RequiredBones)
{
	FBlendedCurve EmptyCurve;

	FCompactPose AnimPose;
	AnimPose.SetBoneContainer(&RequiredBones);

	const FAnimPlayer& CurPlayer = AnimPlayers.Last();

	float EndTime = CurPlayer.pAnim->GetTimeAtFrame(CurPlayer.MotionClip.EndFrame);
	CurPlayer.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(EndTime));

	FVector CurFrontDir = CalcFrontDir(AnimPose);
	CurFrontDir = CurPlayer.Align.TransformVector(CurFrontDir);

	AnimPose[FCompactPoseBoneIndex(0)] *= CurPlayer.Align;
	FVector CurHipPos = AnimPose[FCompactPoseBoneIndex(0)].GetTranslation();

	float BeginTime = NextPlayer.pAnim->GetTimeAtFrame(NextPlayer.MotionClip.BeginFrame);
	NextPlayer.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(BeginTime));

	FVector NextFrontDir = CalcFrontDir(AnimPose);
	FVector NextHipPos = AnimPose[FCompactPoseBoneIndex(0)].GetTranslation();

	FQuat AlignQuat = FQuat::FindBetween(NextFrontDir, CurFrontDir);
	FVector AlignPos = CurHipPos - NextHipPos;
	AlignPos.Z = 0.f;

	return FTransform(-NextHipPos) * FTransform(AlignQuat) * FTransform(NextHipPos) * FTransform(AlignPos);
}

//-------------------------------------------------------------------------------------------------
bool FAnimNode_VBM::CreateNextPlayer(FAnimPlayer& OutPlayer, const FBoneContainer& RequiredBones)
{
	TArray<UAnimSequence*> pAnims;
	AnimMotionClips.GetKeys(pAnims);

	if (pAnims.Num() <= 0)
		return false;

	UAnimSequence* pNextAnim = pAnims[rand() % pAnims.Num()];
	TArray<FMotionClip>* pMotionClips = AnimMotionClips.Find(pNextAnim);

	if (pMotionClips == NULL && pMotionClips->Num() == 0)
		return false;

	FMotionClip NextClip = (*pMotionClips)[rand() % pMotionClips->Num()];
	
	float BeginTime = pNextAnim->GetTimeAtFrame(NextClip.BeginFrame);

/*
	FTransform AlignTransform;
	{
		FCompactPose AnimPose;
		FBlendedCurve EmptyCurve;
		AnimPose.SetBoneContainer(&RequiredBones);

		const FAnimPlayer& CurPlayer = AnimPlayers.Last();

		float EndTime = CurPlayer.pAnim->GetTimeAtFrame(CurPlayer.MotionClip.EndFrame);
		CurPlayer.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(EndTime));

		FVector CurFrontDir = CalcFrontDir(pNextAnim->GetSkeleton(), AnimPose);
		CurFrontDir = CurPlayer.Align.TransformVector(CurFrontDir);

		AnimPose[FCompactPoseBoneIndex(0)] *= CurPlayer.Align;
		FVector CurHipPos = AnimPose[FCompactPoseBoneIndex(0)].GetTranslation();

		pNextAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(BeginTime));

		FVector NextFrontDir = CalcFrontDir(pNextAnim->GetSkeleton(), AnimPose);
		FVector NextHipPos = AnimPose[FCompactPoseBoneIndex(0)].GetTranslation();

		FQuat AlignQuat = FQuat::FindBetween(NextFrontDir, CurFrontDir);
		FVector AlignPos = CurHipPos - NextHipPos;
		AlignPos.Z = 0.f;

		AlignTransform =
			FTransform(-NextHipPos) * FTransform(AlignQuat) * FTransform(NextHipPos) *
			FTransform(AlignPos);
	}
*/
	
	FAnimPlayer NextPlayer(pNextAnim, BeginTime);
	{
		NextPlayer.MotionClip = NextClip;
		NextPlayer.Align = CalcAlignTransoform(NextPlayer, RequiredBones);
	}

	OutPlayer = NextPlayer;
	return true;
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::CreateNextHitInfo(
	const FAnimPlayer& NextPlayer, AVBM_Pawn* pPawn, const FBoneContainer& RequiredBones)
{
	CalcHitDir(NextPlayer.pAnim, NextPlayer.MotionClip.HitBeginFrame, NextPlayer.MotionClip.HitEndFrame, RequiredBones);

	for (FVector& HitVel : NextHitVels)
	{
		HitVel = NextPlayer.Align.TransformVector(HitVel);
	}

	for (FVector& HitPos : NextHitPoss)
	{
		HitPos = NextPlayer.Align.TransformPosition(HitPos) + pPawn->GetActorLocation();
	}

	BallTrajectories.Empty();

	int32 MinTrajectoryIndex = -1;
	float MinDist = FLT_MAX;

	for (int32 IdxHit = 0; IdxHit < NextHitPoss.Num(); ++IdxHit)
	{
		TArray<FVector> Trajectory;
		GenerateBallTrajectory(Trajectory, NextHitPoss[IdxHit], NextHitVels[IdxHit]);

		float Dist = (Trajectory.Last() - pPawn->pDestPawn->PlayerPos).Size();

		if (Dist < MinDist)
		{
			MinDist = Dist;
			pPawn->HitBallPos = NextHitPoss[IdxHit];
			pPawn->HitBallVel = NextHitVels[IdxHit];
			PassTrajectory = Trajectory;

			float HitTime = NextPlayer.pAnim->GetTimeAtFrame(NextPlayer.MotionClip.HitBeginFrame + IdxHit);
			float BeginTime = NextPlayer.pAnim->GetTimeAtFrame(NextPlayer.MotionClip.BeginFrame);

			//BallTime = BeginTime - HitTime;
			BeginBallTime = BeginTime - HitTime;
			pPawn->HitBallTime = HitTime - BeginTime;

			MinTrajectoryIndex = IdxHit;
		}

		BallTrajectories.Add(Trajectory);
	}

	if (BallTrajectories.IsValidIndex(MinTrajectoryIndex))
	{
		BallTrajectories.RemoveAt(MinTrajectoryIndex);
	}
}

//-------------------------------------------------------------------------------------------------
FVector FAnimNode_VBM::CalcHitDir(UAnimSequence* pAnim, const FMotionClip& Clip, const FBoneContainer& RequiredBones)
{
	FVector AvgHitDir = FVector::ZeroVector;

	for (int32 Frame = Clip.HitBeginFrame; Frame < Clip.HitEndFrame; ++Frame)
	{
		float HitTime = pAnim->GetTimeAtFrame(Frame);

		FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Ankle_Joint_01"), RequiredBones);
		FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Knee_Joint_01"), RequiredBones);
		FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Toe_Joint_01"), RequiredBones);

		FVector HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();

		TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pAnim);
		if (pMatchInfos != NULL)
		{
			float Speed = (*pMatchInfos)[Frame - 1].RightFootVel.Size();

			AvgHitDir += HitDir * Speed;
		}
	}

	return AvgHitDir.GetSafeNormal2D();
}

//-------------------------------------------------------------------------------------------------
FVector FAnimNode_VBM::GetMaxHitVel(UAnimSequence* pAnim, const FMotionClip& Clip, const FBoneContainer& RequiredBones)
{
	FVector MaxHitVel = FVector::ZeroVector;

	for (int32 Frame = Clip.HitBeginFrame; Frame < Clip.HitEndFrame; ++Frame)
	{
		float HitTime = pAnim->GetTimeAtFrame(Frame);

		FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Ankle_Joint_01"), RequiredBones);
		FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Knee_Joint_01"), RequiredBones);
		FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Toe_Joint_01"), RequiredBones);

		FVector HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();

		TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pAnim);
		if (pMatchInfos != NULL)
		{
			float Speed = (*pMatchInfos)[Frame - 1].RightFootVel.Size();

			if (MaxHitVel.Size() < Speed)
			{
				MaxHitVel = HitDir * Speed;
			}
		}
	}

	return MaxHitVel;
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::CalcHitDir(UAnimSequence* pAnim, int32 BeginFrame, int32 EndFrame, const FBoneContainer& RequiredBones)
{
	NextHitVels.Empty();
	NextHitPoss.Empty();

	for (int32 Frame = BeginFrame; Frame < EndFrame; ++Frame)
	{
		float HitTime = pAnim->GetTimeAtFrame(Frame);

		FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Ankle_Joint_01"), RequiredBones);
		FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Knee_Joint_01"), RequiredBones);
		FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Toe_Joint_01"), RequiredBones);

		FVector HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();

		TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pAnim);
		if (pMatchInfos != NULL)
		{
			float Speed = (*pMatchInfos)[Frame - 1].RightFootVel.Size();

			NextHitVels.Add(HitDir * Speed);
			NextHitPoss.Add(ToePos);
		}
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::GenerateBallTrajectory(TArray<FVector>& OutTrajectory, const FVector& BeginPos, const FVector& BeginVel)
{
	float Pz0 = BeginPos.Z;
	float Vz0 = BeginVel.Z;

	float t1 = (Vz0 + FMath::Sqrt(Vz0 * Vz0 + 2.f * GRAVITY * Pz0)) / GRAVITY;

	float Vz1 = GRAVITY * t1 - Vz0;

	float t2 = (Vz1 + FMath::Sqrt(Vz1 * Vz1 - 2.f * GRAVITY * Pz0)) / GRAVITY;

	FVector BallPos;
	TArray<FVector> BallPoss;

	for (float Time = 0.f; Time < t1 + t2; Time += 0.033f)
	{
		BallPos.X = BeginPos.X + BeginVel.X * Time;
		BallPos.Y = BeginPos.Y + BeginVel.Y * Time;

		if (Time < t1)
		{
			BallPos.Z = BeginPos.Z + (Vz0 - 0.5f * GRAVITY * Time) * Time;
		}
		else
		{
			float dt = Time - t1;

			BallPos.Z = (Vz1 - 0.5f * GRAVITY * dt) * dt;
		}

		BallPoss.Add(BallPos);
	}

	OutTrajectory = BallPoss;
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::AdjustBallTrajectory(TArray<FVector>& Trajectory, const FVector& BeginVel, const FVector& EndPos)
{
	FVector NewBeginVel = BeginVel;

	while (true)
	{
		FVector DestPos = Trajectory.Last();
		FVector DiffPos = DestPos - EndPos;

		int32 BeginIndex = 0;

		for (int32 IdxPos = Trajectory.Num() - 1; IdxPos > 0; --IdxPos)
		{
			if (Trajectory[IdxPos - 1].Z < Trajectory[IdxPos].Z)
			{
				BeginIndex = IdxPos;
				break;
			}
		}

		int32 MinIndex = BeginIndex;
		FVector MinDir;
		float MinDist = FLT_MAX;

		for (int32 IdxPos = BeginIndex; IdxPos < Trajectory.Num(); ++IdxPos)
		{
			FVector Diff = EndPos - Trajectory[IdxPos];

			if (MinDist > Diff.Size())
			{
				MinDir = Diff;
				MinDist = Diff.Size();
				MinIndex = IdxPos;
			}
		}

		if (MinDist < 0.02f)
		{
			Trajectory.SetNum(MinIndex + 1);
			break;
		}
	
		NewBeginVel += MinDir * 0.01f;

		GenerateBallTrajectory(Trajectory, Trajectory[0], NewBeginVel, EndPos);
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::GenerateBallTrajectory(
	TArray<FVector>& OutTrajectory, const FVector& BeginPos, const FVector& BeginVel, const FVector& EndPos)
{
	FVector AlignedHorVel = (EndPos - BeginPos).GetSafeNormal2D() * BeginVel.Size2D();

	float Pz0 = BeginPos.Z;
	float Vz0 = BeginVel.Z;
	//float Pz1 = Pz0;
	float Pz1 = 0.f;

	float t1 = (Vz0 + FMath::Sqrt(Vz0 * Vz0 + 2.f * GRAVITY * Pz0)) / GRAVITY;

	float Vz1 = GRAVITY * t1 - Vz0;

	float t2 = (Vz1 + FMath::Sqrt(Vz1 * Vz1 - 2.f * GRAVITY * Pz1)) / GRAVITY;

	FVector BallPos;
	TArray<FVector> BallPoss;

	for (float Time = 0.f; Time < t1 + t2; Time += 0.033f)
	{
		BallPos.X = BeginPos.X + AlignedHorVel.X * Time;
		BallPos.Y = BeginPos.Y + AlignedHorVel.Y * Time;

		if (Time < t1)
		{
			BallPos.Z = BeginPos.Z + (Vz0 - 0.5f * GRAVITY * Time) * Time;
		}
		else
		{
			float dt = Time - t1;

			BallPos.Z = (Vz1 - 0.5f * GRAVITY * dt) * dt;
		}

		BallPoss.Add(BallPos);
	}

	OutTrajectory = BallPoss;
}

//-------------------------------------------------------------------------------------------------
FVector FAnimNode_VBM::CalcFrontDir(const FCompactPose& AnimPose, FVector* pOutHipPos)
{
	FCSPose<FCompactPose> CSPose;
	CSPose.InitPose(AnimPose);

	const FReferenceSkeleton& RefSkel = AnimPose.GetBoneContainer().GetReferenceSkeleton();

	int32 HipIndex = RefSkel.FindBoneIndex(FName("Hip"));
	int32 LThighIndex = RefSkel.FindBoneIndex(FName("Left_Thigh_Joint_01"));
	int32 RThighIndex = RefSkel.FindBoneIndex(FName("Right_Thigh_Joint_01"));

	FCompactPoseBoneIndex HipPoseIndex = AnimPose.GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(HipIndex);
	FCompactPoseBoneIndex LThighPoseIndex = AnimPose.GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(LThighIndex);
	FCompactPoseBoneIndex RThighPoseIndex = AnimPose.GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(RThighIndex);

	FVector HipPos = CSPose.GetComponentSpaceTransform(HipPoseIndex).GetTranslation();
	FVector LThighPos = CSPose.GetComponentSpaceTransform(LThighPoseIndex).GetTranslation();
	FVector RThighPos = CSPose.GetComponentSpaceTransform(RThighPoseIndex).GetTranslation();

	FVector Dir1 = LThighPos - HipPos;
	FVector Dir2 = RThighPos - HipPos;

	FVector FrontDir = (Dir1 ^ Dir2).GetSafeNormal2D();

	//DrawDebugLine(GWorld, HipPos, HipPos + FrontDir * 200, FColor::Red);

	if (pOutHipPos != NULL)
	{
		*pOutHipPos = HipPos;
	}

	return FrontDir;
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::AlignPose(USkeleton* pSkel, FCompactPose& AnimPose, FQuat* pOutQuat)
{
	FCompactPoseBoneIndex RootIndex(0);

	FVector FrontDir = CalcFrontDir(AnimPose);
	FVector RootPos = AnimPose[RootIndex].GetTranslation();

	FQuat AlignQuat = FQuat::FindBetween(FrontDir, FVector::ForwardVector);
	FQuat RootQuat = AnimPose[RootIndex].GetRotation();

	AnimPose[RootIndex].SetRotation(AlignQuat * RootQuat);
	AnimPose[RootIndex].SetTranslation(FVector(0, 0, RootPos.Z));

	if (pOutQuat != NULL)
	{
		*pOutQuat = AlignQuat;
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::DrawPose(const FCompactPose& Pose, const FColor& Color)
{
	FCSPose<FCompactPose> CSPose;
	CSPose.InitPose(Pose);

	int32 NumBones = Pose.GetNumBones();
	for (int32 IdxBone = 1; IdxBone < NumBones; ++IdxBone)
	{
		FCompactPoseBoneIndex ParentBoneIndex = Pose.GetParentBoneIndex(FCompactPoseBoneIndex(IdxBone));

		FVector Pos1 = CSPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(IdxBone)).GetTranslation();
		FVector Pos2 = CSPose.GetComponentSpaceTransform(ParentBoneIndex).GetTranslation();

		DrawDebugLine(GWorld, Pos1, Pos2, Color);
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::DrawAnimPoses(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont)
{
	FCompactPose AnimPose;
	AnimPose.SetBoneContainer(&BoneCont);

	FBlendedCurve EmptyCurve;

	EndFrame = FMath::Min(EndFrame, pAnimSeq->NumFrames);

	for (int32 IdxFrame = BeginFrame; IdxFrame < EndFrame; IdxFrame += FrameStep)
	{
		float AnimTime = pAnimSeq->GetTimeAtFrame(IdxFrame);

		pAnimSeq->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(AnimTime));

		AlignPose(pAnimSeq->GetSkeleton(), AnimPose);
		DrawPose(AnimPose, BoneColor);
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::DrawAnimPoses(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont, const FTransform& Align)
{
	FCompactPose AnimPose;
	AnimPose.SetBoneContainer(&BoneCont);

	FBlendedCurve EmptyCurve;

	for (int32 IdxFrame = 0; IdxFrame < pAnimSeq->NumFrames; IdxFrame += FrameStep)
	{
		float AnimTime = pAnimSeq->GetTimeAtFrame(IdxFrame);

		pAnimSeq->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(AnimTime));

		AnimPose[FCompactPoseBoneIndex(0)] *= Align;

		DrawPose(AnimPose, BoneColor);
	}
}

//-------------------------------------------------------------------------------------------------
FVector FAnimNode_VBM::CalcBoneCSLocation(
	const UAnimSequence* pAnimSeq, float AnimTime, const FName& BoneName, const FBoneContainer& BoneCont)
{
	FBlendedCurve EmptyCurve;

	FCompactPose AnimPose;
	AnimPose.SetBoneContainer(&BoneCont);

	pAnimSeq->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(AnimTime));

	FCSPose<FCompactPose> CSPose;
	CSPose.InitPose(AnimPose);

	const FReferenceSkeleton& RefSkel = pAnimSeq->GetSkeleton()->GetReferenceSkeleton();

	int32 BoneIndex = RefSkel.FindBoneIndex(BoneName);

	return CSPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(BoneIndex)).GetTranslation();
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::GeneratePoseMatchInfos(UAnimSequence* pAnimSeq, const FBoneContainer& BoneCont)
{
	FCompactPose PrevAnimPose;
	PrevAnimPose.SetBoneContainer(&BoneCont);

	FCompactPose CurAnimPose;
	CurAnimPose.SetBoneContainer(&BoneCont);

	FBlendedCurve EmptyCurve;

	TArray<FPoseMatchInfo> MatchInfos;

	int32 NumFrames = pAnimSeq->NumFrames;
	for (int32 IdxFrame = 1; IdxFrame < NumFrames; ++IdxFrame)
	{
		float PrevAnimTime = pAnimSeq->GetTimeAtFrame(IdxFrame - 1);
		float CurAnimTime = pAnimSeq->GetTimeAtFrame(IdxFrame);

		pAnimSeq->GetBonePose(PrevAnimPose, EmptyCurve, FAnimExtractContext(PrevAnimTime));
		pAnimSeq->GetBonePose(CurAnimPose, EmptyCurve, FAnimExtractContext(CurAnimTime));

		FPoseMatchInfo MatchInfo;
		GeneratePoseMatchInfo(PrevAnimPose, CurAnimPose, MatchInfo);

		MatchInfos.Add(MatchInfo);
	}

	if (MatchInfos.Num() > 0)
	{
		AnimPoseInfos.Add(pAnimSeq, MatchInfos);
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::GeneratePoseMatchInfo(FCompactPose& PrevPose, FCompactPose& CurPose, FPoseMatchInfo& OutInfo)
{
	const FBoneContainer& BoneContainer = CurPose.GetBoneContainer();

	const FReferenceSkeleton& RefSkel = BoneContainer.GetReferenceSkeleton();

	int32 RootIndex = RefSkel.FindBoneIndex(FName("Root"));
	int32 LFootIndex = RefSkel.FindBoneIndex(FName("Left_Ankle_Joint_01"));
	int32 RFootIndex = RefSkel.FindBoneIndex(FName("Right_Ankle_Joint_01"));
	int32 LHandIndex = RefSkel.FindBoneIndex(FName("Left_Wrist_Joint_01"));
	int32 RHandIndex = RefSkel.FindBoneIndex(FName("Right_Wrist_Joint_01"));

	FCompactPoseBoneIndex RootPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(RootIndex);
	FCompactPoseBoneIndex LFootPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(LFootIndex);
	FCompactPoseBoneIndex RFootPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(RFootIndex);
	FCompactPoseBoneIndex LHandPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(LHandIndex);
	FCompactPoseBoneIndex RHandPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(RHandIndex);

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

		FCompactPoseBoneIndex HipPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(HipIndex);
		FCompactPoseBoneIndex LThighPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(LThighIndex);
		FCompactPoseBoneIndex RThighPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(RThighIndex);

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

		PoseMatchInfo.RootVel = AlignTransform.TransformVector(CurRootPos - PrevRootPos) * 30.f;
		PoseMatchInfo.LeftFootVel = AlignTransform.TransformVector(CurLFootPos - PrevLFootPos) * 30.f;
		PoseMatchInfo.RightFootVel = AlignTransform.TransformVector(CurRFootPos - PrevRFootPos) * 30.f;
		PoseMatchInfo.LeftHandVel = AlignTransform.TransformVector(CurLHandPos - PrevLHandPos) * 30.f;
		PoseMatchInfo.RightHandVel = AlignTransform.TransformVector(CurRHandPos - PrevRHandPos) * 30.f;
	}

	OutInfo = PoseMatchInfo;
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::DrawPoseMatchInfo(UAnimSequence* pAnimSeq)
{
	TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pAnimSeq);
	if (pMatchInfos == NULL)
		return;

	DrawPoseMatchInfo(*pMatchInfos);
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::DrawPoseMatchInfo(const TArray<FPoseMatchInfo>& MatchInfos)
{
	//for (auto& MatchInfo : MatchInfos)
	int32 NumInfos = MatchInfos.Num();
	EndFrame = FMath::Min(EndFrame, NumInfos);

	for (int32 IdxInfo = BeginFrame; IdxInfo < EndFrame; IdxInfo += FrameStep)
	{
		const FPoseMatchInfo& MatchInfo = MatchInfos[IdxInfo];

		DrawDebugLine(GWorld, FVector::ZeroVector, MatchInfo.RootVel, FColor::Black);
		DrawDebugLine(GWorld, MatchInfo.LeftFootPos, MatchInfo.LeftFootPos + MatchInfo.LeftFootVel, FColor::Magenta);
		DrawDebugLine(GWorld, MatchInfo.RightFootPos, MatchInfo.RightFootPos + MatchInfo.RightFootVel, FColor::Red);

		DrawDebugPoint(GWorld, MatchInfo.LeftFootPos, 5, FColor::Magenta);
		DrawDebugPoint(GWorld, MatchInfo.RightFootPos, 5, FColor::Red);
		DrawDebugPoint(GWorld, MatchInfo.LeftHandPos, 5, FColor::Cyan);
		DrawDebugPoint(GWorld, MatchInfo.RightHandPos, 5, FColor::Blue);
	}
}

// --------------------------------------------------------------------------------------------------
int32 FAnimNode_VBM::SearchBestMatchFrame(const UAnimSequence* pNextAnim)
{
	UAnimSequence* pCurAnim = AnimPlayers.Last().pAnim;
	if (pCurAnim == NULL)
		return 0;

	TArray<FPoseMatchInfo>* pCurMatchInfos = AnimPoseInfos.Find(pCurAnim);
	if (pCurMatchInfos == NULL)
		return 0;

	TArray<FPoseMatchInfo>* pNextMatchInfos = AnimPoseInfos.Find(pNextAnim);
	if (pNextMatchInfos == NULL)
		return 0;

	int32 CurFrame = pCurAnim->GetFrameAtTime(AnimPlayers.Last().Time);

	const FPoseMatchInfo& CurMatchInfo = (*pCurMatchInfos)[FMath::Max(0, CurFrame - 1)];

	int32 MinFrame = 0;
	float MinCost =  FLT_MAX;

	int32 NumInfos = pNextMatchInfos->Num() - 90;
	for (int32 IdxInfo = 0; IdxInfo < NumInfos; ++IdxInfo)
	{
		const FPoseMatchInfo& NextMatchInfo = (*pNextMatchInfos)[IdxInfo];

		float MatchCost = CalcMatchCost(CurMatchInfo, NextMatchInfo);
		if (MatchCost < MinCost)
		{
			MinCost = MatchCost;
			MinFrame = IdxInfo + 1;
		}
	}

	return MinFrame;
}

// --------------------------------------------------------------------------------------------------
float FAnimNode_VBM::CalcMatchCost(const FPoseMatchInfo& CurMatchInfo, const FPoseMatchInfo& NextMatchInfo)
{
	return
		(CurMatchInfo.RootVel - NextMatchInfo.RootVel).Size() +
		(CurMatchInfo.LeftFootVel - NextMatchInfo.LeftFootVel).Size() +
		(CurMatchInfo.RightFootVel - NextMatchInfo.RightFootVel).Size() +
		(CurMatchInfo.LeftHandVel - NextMatchInfo.LeftHandVel).Size() +
		(CurMatchInfo.RightHandVel - NextMatchInfo.RightHandVel).Size() +
		(CurMatchInfo.RootPos - NextMatchInfo.RootPos).Size() +
		(CurMatchInfo.LeftFootPos - NextMatchInfo.LeftFootPos).Size() +
		(CurMatchInfo.RightFootPos - NextMatchInfo.RightFootPos).Size() +
		(CurMatchInfo.LeftHandPos - NextMatchInfo.LeftHandPos).Size() +
		(CurMatchInfo.RightHandPos - NextMatchInfo.RightHandPos).Size();
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::AnalyzeMotion(const FPoseMatchInfo& MatchInfo)
{
	for (const auto& AnimPoseInfo : AnimPoseInfos)
	{
		TArray<int32> LocalMaxIndices;

		const TArray<FPoseMatchInfo>& PoseMatchInfos = AnimPoseInfo.Value;

		int NumInfos = PoseMatchInfos.Num() - 1;
		for (int32 IdxInfo = 1; IdxInfo < NumInfos; ++IdxInfo)
		{
			float RFootVel = PoseMatchInfos[IdxInfo].RightFootVel.Size();

			if (RFootVel > LimitVel &&
				RFootVel > PoseMatchInfos[IdxInfo - 1].RightFootVel.Size() &&
				RFootVel > PoseMatchInfos[IdxInfo + 1].RightFootVel.Size())
			{
				LocalMaxIndices.Add(IdxInfo);
			}
		}

		TArray<int32> HitFrames;
		TArray<FHitSection> HitSections;

		for (int32 IdxMax = 1; IdxMax < LocalMaxIndices.Num(); ++IdxMax)
		{
			int32 BeginIndex = LocalMaxIndices[IdxMax - 1];
			int32 EndIndex = LocalMaxIndices[IdxMax];

			TArray<int32> LocalMinIndices;
			for (int32 IdxInfo = BeginIndex + 1; IdxInfo < EndIndex; ++IdxInfo)
			{
				float RFootVel = PoseMatchInfos[IdxInfo].RightFootVel.Size();

				if (RFootVel < LimitVel &&
					RFootVel < PoseMatchInfos[IdxInfo - 1].RightFootVel.Size() &&
					RFootVel < PoseMatchInfos[IdxInfo + 1].RightFootVel.Size())
				{
					LocalMinIndices.Add(IdxInfo);
				}
			}

			if (LocalMinIndices.Num() == 1)
			{
				HitFrames.Add(LocalMinIndices[0] + 1);

				FHitSection HitSec;
				{
					HitSec.BeginFrame = BeginIndex + 1;
					HitSec.EndFrame = HitFrames.Last();
				}
				HitSections.Add(HitSec);
			}
		}

		if (HitFrames.Num() > 0)
		{
			AnimHitFrames.Add(AnimPoseInfo.Key, HitFrames);
			AnimHitSections.Add(AnimPoseInfo.Key, HitSections);
		}

		TArray<int32> MatchFrames;

		int NumHit = HitFrames.Num();
		for (int32 IdxHit = 0; IdxHit <= NumHit; ++IdxHit)
		{
			int32 BeginFrame = 0;
			if (IdxHit > 0)
			{
				BeginFrame = HitFrames[IdxHit - 1] + 1;
			}

			int32 EndFrame = PoseMatchInfos.Num();
			if (IdxHit < NumHit)
			{
				EndFrame = HitFrames[IdxHit];
			}

			float MinCost = FLT_MAX;
			int32 MinFrame = -1;

			for (int32 Frame = BeginFrame; Frame < EndFrame; ++Frame)
			{
				float Cost = CalcMatchCost(PoseMatchInfos[Frame], MatchInfo);

				if (Cost < MinCost)
				{
					MinCost = Cost;
					MinFrame = Frame;
				}
			}

			if (MinFrame >= 0)
			{
				MatchFrames.Add(MinFrame);
			}
		}

		if (MatchFrames.Num() > 0)
		{
			AnimMatchFrames.Add(AnimPoseInfo.Key, MatchFrames);
		}

		TArray<FMotionClip> MotionClips;

		int32 NumMatch = MatchFrames.Num();
		for (int32 IdxMatch = 1; IdxMatch < NumMatch; ++IdxMatch)
		{
			FMotionClip MotionClip;
			{
				MotionClip.BeginFrame = MatchFrames[IdxMatch - 1];
				MotionClip.EndFrame = MatchFrames[IdxMatch];
			}

			int32 NumHit = HitSections.Num();
			for (int32 IdxHit = 0; IdxHit < NumHit; ++IdxHit)
			{
				int32 HitBegin = HitSections[IdxHit].BeginFrame;
				int32 HitEnd = HitSections[IdxHit].EndFrame;

				if (MotionClip.BeginFrame < HitBegin && HitEnd < MotionClip.EndFrame)
				{
					MotionClip.HitBeginFrame = HitBegin;
					MotionClip.HitEndFrame = HitEnd;
				}
			}

			MotionClips.Add(MotionClip);
		}

		if (MotionClips.Num() > 0)
		{
			AnimMotionClips.Add(AnimPoseInfo.Key, MotionClips);
		}
	}
}


/*
//--------------------------------------------------------------------------------------------------
void FAnimNode_VBM::SavePoseMatchInfos(const FString& FilePath)
{
	FBufferArchive ToBinary;

	for (auto& Info : AnimPoseInfos)
	{
		UAnimSequence* pAnimSeq = Info.Key;
		TArray<FPoseMatchInfo>& MatchInfos = Info.Value;

		int64 MemSize = sizeof(UAnimSequence);

		ToBinary.Serialize(pAnimSeq, MemSize);

		int32 NumInfos = MatchInfos.Num();
		for (FPoseMatchInfo& MatchInfo : MatchInfos)
		{
			ToBinary << MatchInfo.RootVel;
			ToBinary << MatchInfo.LeftFootVel;
			ToBinary << MatchInfo.RightFootVel;
			ToBinary << MatchInfo.LeftFootPos;
			ToBinary << MatchInfo.RightFootPos;
			ToBinary << MatchInfo.LeftHandPos;
			ToBinary << MatchInfo.RightHandPos;
		}
	}

	if (FFileHelper::SaveArrayToFile(ToBinary, *FilePath))
	{
		// Free Binary Array 	
		ToBinary.FlushCache();
		ToBinary.Empty();
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::LoadPoseMatchInfos(const FString& FilePath)
{
	TArray<uint8> TheBinaryArray;
	if (FFileHelper::LoadFileToArray(TheBinaryArray, *FilePath) == false)
		return;

	if (TheBinaryArray.Num() <= 0)
		return;

	Anims.Empty();
	AnimPoseInfos.Empty();

	FMemoryReader FromBinary = FMemoryReader(TheBinaryArray, true); //true, free data after done
	FromBinary.Seek(0);

	while (FromBinary.AtEnd() == false)
	{
		UAnimSequence* pAnimSeq = (UAnimSequence*)(FMemory::Malloc(sizeof(UAnimSequence)));
		FromBinary.Serialize(pAnimSeq, sizeof(UAnimSequence));

		TArray<FPoseMatchInfo> MatchInfos;

		int32 NumInfos = pAnimSeq->NumFrames - 1;
		for (int32 IdxInfo = 0; IdxInfo < NumInfos; ++IdxInfo)
		{
			FPoseMatchInfo MatchInfo;
			{
				FromBinary << MatchInfo.RootVel;
				FromBinary << MatchInfo.LeftFootVel;
				FromBinary << MatchInfo.RightFootVel;
				FromBinary << MatchInfo.LeftFootPos;
				FromBinary << MatchInfo.RightFootPos;
				FromBinary << MatchInfo.LeftHandPos;
				FromBinary << MatchInfo.RightHandPos;
			}

			MatchInfos.Add(MatchInfo);
		}

		if (MatchInfos.Num() > 0)
		{
			AnimPoseInfos.Add(pAnimSeq, MatchInfos);

			Anims.Add(pAnimSeq);
		}
	}

	FromBinary.FlushCache();

	// Empty & Close Buffer 
	TheBinaryArray.Empty();
	FromBinary.Close();
}
*/

#pragma optimize("", on)