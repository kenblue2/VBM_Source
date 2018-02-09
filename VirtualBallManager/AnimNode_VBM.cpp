#include "AnimNode_VBM.h"
#include "AnimationRuntime.h"
#include "VBM_AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "VBM_Pawn.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMeshSocket.h"


#pragma optimize("", off)

const float GRAVITY = 980.f;


//-------------------------------------------------------------------------------------------------
// FAnimNode_VBM
FAnimNode_VBM::FAnimNode_VBM()
	: BallTime(-1.f)
	, bMoveBall(false)
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

			if (CurPlayer.pAnim)
			{
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
			if (Player.pAnim == NULL)
				continue;

			Player.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(Player.Time), true);

			AnimPose[FCompactPoseBoneIndex(0)] *= Player.Align;

			AnimPoses.Add(AnimPose);
			AnimWeights.Add(Player.Weight);
		}

		if (AnimPoses.Num() == 0)
			return;

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

void DrawMatchInfo2(const FPoseMatchInfo& MatchInfo)
{
	DrawDebugLine(GWorld, MatchInfo.RootPos, MatchInfo.RootPos + MatchInfo.RootVel, FColor::Yellow);
	DrawDebugLine(GWorld, MatchInfo.LeftFootPos, MatchInfo.LeftFootPos + MatchInfo.LeftFootVel, FColor::Red);
	DrawDebugLine(GWorld, MatchInfo.RightFootPos, MatchInfo.RightFootPos + MatchInfo.RightFootVel, FColor::Blue);
	//DrawDebugLine(GWorld, MatchInfo.LeftHandPos, MatchInfo.LeftHandPos + MatchInfo.LeftHandVel, FColor::Red);
	//DrawDebugLine(GWorld, MatchInfo.RightHandPos, MatchInfo.RightHandPos + MatchInfo.RightHandVel, FColor::Blue);

	DrawDebugPoint(GWorld, MatchInfo.RootPos, 5, FColor::Yellow);
	DrawDebugPoint(GWorld, MatchInfo.LeftFootPos, 5, FColor::Red);
	DrawDebugPoint(GWorld, MatchInfo.RightFootPos, 5, FColor::Blue);
	//DrawDebugPoint(GWorld, MatchInfo.LeftHandPos, 5, FColor::Red);
	//DrawDebugPoint(GWorld, MatchInfo.RightHandPos, 5, FColor::Blue);
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
			}
		}

		FPoseMatchInfo& IdleMatchInfo = AnimPoseInfos.CreateIterator()->Value[0];

		//if (pAnim->BoneName == FName("Right_Ankle_Joint_01"))
		//{
		//	AnalyzeMotionByRightFoot(IdleMatchInfo, RequiredBones);
		//}
		//else
		//{
		//	AnalyzeMotionByLeftFoot(IdleMatchInfo, RequiredBones);
		//}

		AnalyzeMotion(IdleMatchInfo, RequiredBones);
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
		static bool generate = false;

		if (generate == false)
		{
			generate = true;

			FCompactPose CommonPose;
			CommonPose.SetBoneContainer(&RequiredBones);

			FBlendedCurve EmptyCurve;
			Anims[0]->GetBonePose(CommonPose, EmptyCurve, FAnimExtractContext(0.f), true);

			for (auto& pAnim : Anims)
			{
				TArray<int32>* pMatchFrames = AnimMatchFrames.Find(pAnim);

				TArray<TArray<FQuat>> DeltaRotList;

				for (int IdxMatch = 0; IdxMatch < pMatchFrames->Num(); ++IdxMatch)
				{
					int32 MatchFrame = (*pMatchFrames)[IdxMatch];

					TArray<FQuat> DeltaRots;

					int32 IdxBone = 0;
					{
						FRawAnimSequenceTrack& AnimTrack = pAnim->GetRawAnimationTrack(IdxBone);
						{
							FQuat DeltaQuat = CommonPose.GetBones()[IdxBone].GetRotation() * AnimTrack.RotKeys[MatchFrame].Inverse();
							{
								FRotator DeltaRot = DeltaQuat.Rotator();
								DeltaRot.Yaw = 0.f;

								DeltaQuat = DeltaRot.Quaternion();
							}
							DeltaRots.Add(DeltaQuat);

							AnimTrack.RotKeys[MatchFrame] = DeltaQuat * AnimTrack.RotKeys[MatchFrame];
						}
					}

					for (int32 IdxBone = 1; IdxBone < RequiredBones.GetNumBones(); ++IdxBone)
					{
						FRawAnimSequenceTrack& AnimTrack = pAnim->GetRawAnimationTrack(IdxBone);
						if (AnimTrack.RotKeys.Num() == pAnim->NumFrames)
						{
							FQuat DeltaQuat = CommonPose.GetBones()[IdxBone].GetRotation() * AnimTrack.RotKeys[MatchFrame].Inverse();

							DeltaRots.Add(DeltaQuat);

							AnimTrack.RotKeys[MatchFrame] = DeltaQuat * AnimTrack.RotKeys[MatchFrame];
						}
					}

					if (DeltaRots.Num() > 0)
					{
						DeltaRotList.Add(DeltaRots);
					}
				}

				for (int IdxMatch = 1; IdxMatch < pMatchFrames->Num(); ++IdxMatch)
				{
					int32 PreMatchFrame = (*pMatchFrames)[IdxMatch - 1];
					int32 CurMatchFrame = (*pMatchFrames)[IdxMatch];

					int32 IdxRot = 0;
					/*{
						FRawAnimSequenceTrack& AnimTrack = pAnim->GetRawAnimationTrack(IdxRot);

						for (int32 Frame = PreMatchFrame + 1; Frame < CurMatchFrame; ++Frame)
						{
							float Alpha = float(Frame - PreMatchFrame) / float(CurMatchFrame - PreMatchFrame);

							FQuat BlendQuat = FQuat::Slerp(DeltaRotList[IdxMatch - 1][IdxRot], DeltaRotList[IdxMatch][IdxRot], Alpha);
							
							FRotator BlendRot = BlendQuat.Rotator();
							BlendRot.Yaw = 0.f;

							AnimTrack.RotKeys[Frame] = BlendRot.Quaternion() * AnimTrack.RotKeys[Frame];
						}

						++IdxRot;
					}*/

					for (int32 IdxBone = 0; IdxBone < RequiredBones.GetNumBones(); ++IdxBone)
					{
						FRawAnimSequenceTrack& AnimTrack = pAnim->GetRawAnimationTrack(IdxBone);
						if (AnimTrack.RotKeys.Num() < pAnim->NumFrames)
							continue;

						for (int32 Frame = PreMatchFrame + 1; Frame < CurMatchFrame; ++Frame)
						{
							float Alpha = float(Frame - PreMatchFrame) / float(CurMatchFrame - PreMatchFrame);

							FQuat BlendQuat = FQuat::Slerp(DeltaRotList[IdxMatch - 1][IdxRot], DeltaRotList[IdxMatch][IdxRot], Alpha);

							AnimTrack.RotKeys[Frame] = BlendQuat * AnimTrack.RotKeys[Frame];
						}

						++IdxRot;
					}
				}
			}
		}

		



		float RemainedTime = BallEndTime - BallTime;

		if (pVBMPawn->pDestPawn != NULL)
		{
			if (RemainedTime < pVBMPawn->pDestPawn->HitBallTime)
			{
				pVBMPawn->bBeginNextMotion = true;

				pVBMPawn->pDestPawn->TimeError = pVBMPawn->pDestPawn->HitBallTime - RemainedTime;
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
				BallPos = FMath::Lerp(PassTrajectory2[iBallFrame], PassTrajectory2[iBallFrame + 1], Ratio);
			}
			else if (iBallFrame > 0)
			{
				BallPos = PassTrajectory2.Last();
			}

			//DrawDebugSphere(GWorld, BallPos, 15.f, 16, FColor::Orange);
		}

		if (ShowDebugInfo && 0.f < BallTime && BallTime < BallEndTime)
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
				DrawDebugLine(GWorld, HitPos, HitPos + HitVel, FColor::Cyan);
			}

			for (auto& BallTrajectory : BallTrajectories)
			{
				for (int32 IdxPos = 1; IdxPos < BallTrajectory.Num(); ++IdxPos)
				{
					DrawDebugLine(GWorld, BallTrajectory[IdxPos - 1], BallTrajectory[IdxPos], FColor::Black);
				}
			}

			if (pVBMPawn->pDestPawn)
			{
				DrawDebugLine(GWorld, pVBMPawn->PlayerPos, pVBMPawn->pDestPawn->PlayerPos, FColor::Yellow, false, -1.f, 0, 1.f);
				DrawDebugSphere(GWorld, pVBMPawn->pDestPawn->PlayerPos, 10.f, 16, FColor::Green);

				DrawDebugSphere(GWorld, pVBMPawn->HitBallPos, 3.f, 8, FColor::Red);
			}
		}
	}

	if (BestTrajectory.Num() > 0 && UserTrajectory.Num() > 0)
	{
		for (const FVector& FootPos : BestTrajectory)
		{
			DrawDebugPoint(GWorld, FootPos, 5.f, FColor::Orange);
		}

		for (int32 IdxPos = 1; IdxPos < BestTrajectory.Num(); ++IdxPos)
		{
			FVector Pos1 = BestTrajectory[IdxPos - 1];
			FVector Pos2 = BestTrajectory[IdxPos];

			DrawDebugLine(GWorld, Pos1, Pos2, FColor::Orange, false, -1.f, 0, 1.f);
		}

		for (const FVector& FootPos : UserTrajectory)
		{
			DrawDebugPoint(GWorld, FootPos, 5.f, FColor::White);
		}

		for (int32 IdxPos = 1; IdxPos < UserTrajectory.Num(); ++IdxPos)
		{
			FVector Pos1 = UserTrajectory[IdxPos - 1];
			FVector Pos2 = UserTrajectory[IdxPos];

			DrawDebugLine(GWorld, Pos1, Pos2, FColor::White, false, -1.f, 0, 1.f);
		}
	}

	

	//if (pVBMPawn->pDestPawn == NULL)
	//{
	//	for (auto& MatchInfo : BestPoseMatchInfos)
	//	{
	//		DrawMatchInfo2(MatchInfo);
	//	}
	//}

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

	//if (pVBMPawn && pVBMPawn->pPrevPawn)
	//{
	//	FVector PlayerPos = pVBMPawn->PlayerPos;
	//	DrawDebugLine(GWorld, PlayerPos, PlayerPos + NextHitFrontDir * 300.f, FColor::Red, false, -1, 0, 1);
	//	DrawDebugLine(GWorld, PlayerPos, pVBMPawn->pPrevPawn->PlayerPos, FColor::Red, false, -1, 0, 1);
	//}

	/*FVector PlayerPos = pVBMPawn->PlayerPos;

	DrawDebugLine(GWorld, PlayerPos, PlayerPos + UserMoveDir1, FColor::Red, false, -1, 0, 2);
	DrawDebugLine(GWorld, PlayerPos + UserMoveDir1, PlayerPos + UserMoveDir1 + UserMoveDir2, FColor::Red, false, -1, 0, 2);

	DrawDebugLine(GWorld, PlayerPos, PlayerPos + FootMoveDir1, FColor::Blue, false, -1, 0, 2);
	DrawDebugLine(GWorld, PlayerPos + FootMoveDir1, PlayerPos + FootMoveDir1 + FootMoveDir2, FColor::Blue, false, -1, 0, 2);*/

	//if (Anims.IsValidIndex(AnimIndex) && Anims[AnimIndex])
	//{
	//	DrawPoseMatchInfo(Anims[AnimIndex]);
	//	DrawAnimPoses(Anims[AnimIndex], BoneContainer);
	//}
}

//-------------------------------------------------------------------------------------------------
float CalcTrajectoryDiff(TArray<FVector>& List1, TArray<FVector>& List2)
{
	FVector Dir1 = (List1[1] - List1[0]).GetSafeNormal2D();
	FVector Dir2 = (List2[1] - List2[0]).GetSafeNormal2D();

	FQuat AlignQuat = FQuat::FindBetweenNormals(Dir1, Dir2);

	FVector InitPos1 = List1[0];
	FVector InitPos2 = List2[0];

	for (FVector& Pos : List1)
	{
		Pos = (Pos - InitPos1);
	}

	for (FVector& Pos : List2)
	{
		Pos = AlignQuat.RotateVector(Pos - InitPos2);
	}


	float DiffTraj = 0.f;

	float Scale = (float)(List2.Num() - 1) / (float)(List1.Num() - 1);

	for (int32 IdxPos = 0; IdxPos < List1.Num() - 1; ++IdxPos)
	{
		float fIndex = Scale * IdxPos;
		int32 iIndex = (int32)fIndex;
		float Alpha = fIndex - iIndex;

		FVector Pos2 = FMath::Lerp(List2[iIndex], List2[iIndex + 1], Alpha);

		float DiffPos = (List1[IdxPos] - Pos2).Size();

		DiffTraj += DiffPos;
	}

	float DiffEndPos = (List1.Last() - List2.Last()).Size();

	DiffTraj += DiffEndPos;

	return DiffTraj / List1.Num();
}

//-------------------------------------------------------------------------------------------------
float CalcTrajectoryDiff2(TArray<FVector>& List1, TArray<FVector>& List2)
{
	FVector Dir1 = (List1[1] - List1[0]).GetSafeNormal2D();
	FVector Dir2 = (List2[1] - List2[0]).GetSafeNormal2D();

	FQuat AlignQuat = FQuat::FindBetweenNormals(Dir1, Dir2);

	FVector InitPos1 = List1[0];
	FVector InitPos2 = List2[0];

	for (FVector& Pos : List1)
	{
		Pos = (Pos - InitPos1);
	}

	for (FVector& Pos : List2)
	{
		Pos = AlignQuat.RotateVector(Pos - InitPos2);
	}

	FVector Vel1 = List1.Last() / (0.033f * (List1.Num() - 1));
	FVector Vel2 = List2.Last() / (0.033f * (List2.Num() - 1));

	float Length1 = 0.f;
	for (int32 IdxPos = 1; IdxPos < List1.Num(); ++IdxPos)
	{
		Length1 += (List1[IdxPos - 1] - List1[IdxPos]).Size();
	}

	float Length2 = 0.f;
	for (int32 IdxPos = 1; IdxPos < List2.Num(); ++IdxPos)
	{
		Length2 += (List2[IdxPos - 1] - List2[IdxPos]).Size();
	}

	//return FMath::Abs(Length1 - Length2) + (List1.Last() - List2.Last()).Size();
	//return (Vel1 - Vel2).Size() + (List1.Last() - List2.Last()).Size();
	return (Vel1 - Vel2).Size() + FMath::Abs(Length1 - Length2);
}

//-------------------------------------------------------------------------------------------------
float CalcTrajectoryDiff3(TArray<FVector>& List1, TArray<FVector>& List2)
{
	FVector Dir1 = (List1[1] - List1[0]).GetSafeNormal2D();
	FVector Dir2 = (List2[1] - List2[0]).GetSafeNormal2D();

	FQuat AlignQuat = FQuat::FindBetweenNormals(Dir1, Dir2);

	FVector InitPos1 = List1[0];
	FVector InitPos2 = List2[0];

	for (FVector& Pos : List1)
	{
		Pos = (Pos - InitPos1);
	}

	for (FVector& Pos : List2)
	{
		Pos = AlignQuat.RotateVector(Pos - InitPos2);
	}


	float DiffTraj = 0.f;

	int32 NumSamples = 10;

	float Scale1 = float(List1.Num() - 1) / float(NumSamples);
	float Scale2 = float(List2.Num() - 1) / float(NumSamples);

	for (int32 IdxPos = 0; IdxPos < NumSamples; ++IdxPos)
	{
		FVector Pos1;
		{
			float fIndex = Scale1 * IdxPos;
			int32 iIndex = (int32)fIndex;
			float Alpha = fIndex - iIndex;

			Pos1 = FMath::Lerp(List1[iIndex], List1[iIndex + 1], Alpha);
		}

		FVector Pos2;
		{
			float fIndex = Scale2 * IdxPos;
			int32 iIndex = (int32)fIndex;
			float Alpha = fIndex - iIndex;

			Pos2 = FMath::Lerp(List2[iIndex], List2[iIndex + 1], Alpha);
		}

		float DiffPos = (Pos1 - Pos2).Size();

		DiffTraj += DiffPos;
	}

	float DiffEndPos = (List1.Last() - List2.Last()).Size();

	DiffTraj += DiffEndPos;

	return DiffTraj / (float)NumSamples;
}

//-------------------------------------------------------------------------------------------------
float CalcTrajectoryDiff4(TArray<FVector>& UserMove, TArray<FVector>& CharMove)
{
	check(UserMove.Num() <= CharMove.Num());

	FVector Dir1 = (UserMove[1] - UserMove[0]).GetSafeNormal2D();
	FVector Dir2 = (CharMove[1] - CharMove[0]).GetSafeNormal2D();

	FQuat AlignQuat = FQuat::FindBetweenNormals(Dir1, Dir2);

	FVector InitPos1 = UserMove[0];
	FVector InitPos2 = CharMove[0];

	for (FVector& Pos : UserMove)
	{
		Pos = (Pos - InitPos1);
	}

	for (FVector& Pos : CharMove)
	{
		Pos = AlignQuat.RotateVector(Pos - InitPos2);
	}

	float DiffTraj = 0.f;
	for (int32 IdxPos = 0; IdxPos < UserMove.Num(); ++IdxPos)
	{
		float DiffPos = (UserMove[IdxPos] - CharMove[IdxPos]).Size();

		DiffTraj += DiffPos;
	}

	return DiffTraj;
}

//-------------------------------------------------------------------------------------------------
float CalcTrajectoryDiff5(const FMotionClip& UserClip, const FMotionClip& DataClip)
{
	if (UserClip.bAttack != DataClip.bAttack)
		return FLT_MAX;

	return
		FMath::Abs(UserClip.Angle1 - DataClip.Angle1) +
		FMath::Abs(UserClip.Angle2 - DataClip.Angle2) +
		FMath::Abs(UserClip.MoveDist - DataClip.MoveDist) +
		FMath::Abs(UserClip.MaxSpeed - DataClip.MaxSpeed);
}

//-------------------------------------------------------------------------------------------------
float CalcTrajectoryDiff6(const FMotionClip& UserClip, const FMotionClip& DataClip, FQuat& OutRot)
{
	FVector HorDir1 = UserClip.MoveDir1.GetSafeNormal2D();
	FVector HorDir2 = DataClip.MoveDir1.GetSafeNormal2D();

	FVector HorDir3 = UserClip.MoveDir2.GetSafeNormal2D();
	FVector HorDir4 = DataClip.MoveDir2.GetSafeNormal2D();

	FQuat AlignQuat = FQuat::FindBetweenNormals(HorDir1, HorDir2);

	FVector AlignedDir1 = AlignQuat.RotateVector(DataClip.MoveDir1);
	FVector AlignedDir2 = AlignQuat.RotateVector(DataClip.MoveDir2);

	float Angle1 = FMath::Acos(UserClip.MoveDir1.GetSafeNormal() | AlignedDir1.GetSafeNormal());
	float Angle2 = FMath::Acos(UserClip.MoveDir2.GetSafeNormal() | AlignedDir2.GetSafeNormal());

	OutRot = AlignQuat;

	if ((HorDir1 | HorDir2) < 0.f || (HorDir3 | HorDir4) < 0.f)
		return FLT_MAX;

	return Angle1 + Angle2;
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::CreateNextPlayer(AVBM_Pawn* pPawn, const TArray<FVector>& FootTrajectory, bool bUseLeftFoot)
{
	FVector PlayerPos = pPawn->PlayerPos;
	FVector DestPos = pPawn->pDestPawn->PlayerPos;

	FVector PassDir = (DestPos - PlayerPos).GetSafeNormal2D();

	float MinCost = FLT_MAX;
	FAnimPlayer MinPlayer;

	UserTrajectory = FootTrajectory;

	// select motion clip
	for (auto& AnimMotionClip : AnimMotionClips)
	{
		UAnimSequence* pNextAnim = AnimMotionClip.Key;
		if (pNextAnim == NULL)
			continue;

		TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pNextAnim);
		if (pMatchInfos == NULL)
			continue;

		FBlendedCurve EmptyCurve;

		FCompactPose AnimPose;
		AnimPose.SetBoneContainer(&BoneContainer);

		FMotionClip UserClip;

		if (UserTrajectory.Num() > 0)
		{
			if (bUseLeftFoot && pNextAnim->BoneName != FName("Left_Ankle_Joint_01"))
				continue;

			if (bUseLeftFoot == false && pNextAnim->BoneName != FName("Right_Ankle_Joint_01"))
				continue;

			FVector MidPos;
			float MaxHeight = 0.f;
			float MoveDist = 0.f;
			float MaxSpeed = 0.f;

			for (int32 IdxPos = 1; IdxPos < UserTrajectory.Num(); ++IdxPos)
			{
				const FVector& CurPos = UserTrajectory[IdxPos];
				const FVector& PrePos = UserTrajectory[IdxPos - 1];

				float FootSpeed = (CurPos - PrePos).Size();

				MoveDist += FootSpeed;

				if (MaxHeight < CurPos.Z)
				{
					MaxHeight = CurPos.Z;
					MidPos = CurPos;
				}

				if (MaxSpeed < FootSpeed)
				{
					MaxSpeed = FootSpeed * 30.f;
				}
			}

			FVector DirHor = (MidPos - UserTrajectory[0]).GetSafeNormal2D();
			FVector Dir1 = (MidPos - UserTrajectory[0]).GetSafeNormal();
			FVector Dir2 = (UserTrajectory.Last() - MidPos).GetSafeNormal();

			UserClip.Angle1 = FMath::Acos(Dir1 | DirHor);
			UserClip.Angle2 = FMath::Acos(Dir1 | Dir2);
			UserClip.MoveDist = MoveDist;
			UserClip.MaxSpeed = MaxSpeed;

			UserClip.MoveDir1 = MidPos - UserTrajectory[0];
			UserClip.MoveDir2 = UserTrajectory.Last() - MidPos;

			if ((Dir1.GetSafeNormal2D() | Dir2.GetSafeNormal2D()) > 0.f)
			{
				UserClip.bAttack = true;
			}
			else
			{
				UserClip.bAttack = false;
			}

			UserMoveDir1 = UserClip.MoveDir1;
			UserMoveDir2 = UserClip.MoveDir2;
		}

		for (auto& MotionClip : AnimMotionClip.Value)
		{
			float BeginTime = pNextAnim->GetTimeAtFrame(MotionClip.BeginFrame);

			FAnimPlayer NextPlayer(pNextAnim, BeginTime, 0.f, 1.f);
			{
				NextPlayer.MotionClip = MotionClip;
				NextPlayer.Align = CalcAlignTransoform(NextPlayer, BoneContainer);
			}

			float DiffTraj = 0.f;

			TArray<FVector> ClipTrajectory;

			FQuat AlignQuat = FQuat::Identity;

			if (UserTrajectory.Num() > 0)
			{
				//for (int32 IdxInfo = MotionClip.HitBeginFrame; IdxInfo <= MotionClip.HitEndFrame; ++IdxInfo)
				//{
				//	const FVector& FootPos = (*pMatchInfos)[IdxInfo].RightFootPos;
				//	ClipTrajectory.Add(FootPos);
				//}

				//int32 BeginFrame = MotionClip.HitBeginFrame;
				//int32 EndFrame = BeginFrame + UserTrajectory.Num();
				//for (int32 Frame = BeginFrame; Frame < EndFrame; ++Frame)
				for (int32 Frame = MotionClip.HitBeginFrame; Frame <= MotionClip.HitEndFrame; ++Frame)
				{
					float Time = pNextAnim->GetTimeAtFrame(Frame);

					FVector FootPos;
					if (pNextAnim->BoneName == FName("Right_Ankle_Joint_01"))
					{
						FootPos = CalcBoneCSLocation(pNextAnim, Time, FName("Right_Ankle_Joint_01"), BoneContainer);
					}
					else
					{
						FootPos = CalcBoneCSLocation(pNextAnim, Time, FName("Left_Ankle_Joint_01"), BoneContainer);
					}

					ClipTrajectory.Add(FootPos);
				}
				
				//DiffTraj = CalcTrajectoryDiff5(UserClip, MotionClip);
				DiffTraj = CalcTrajectoryDiff6(UserClip, MotionClip, AlignQuat);

				UserMoveDir1 = (UserClip.MoveDir1);
				UserMoveDir2 = (UserClip.MoveDir2);
			}

			for (int32 Frame = MotionClip.HitBeginFrame; Frame < MotionClip.HitEndFrame; ++Frame)
			{
				float MoveAngle = 0.f;
				FVector NextFrontDir;

				if (pPawn->pPrevPawn)
				{
					float HitTime = NextPlayer.pAnim->GetTimeAtFrame(Frame);
					NextPlayer.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(HitTime), true);
					NextFrontDir = NextPlayer.Align.TransformVector(CalcFrontDir(AnimPose));

					FVector BallMoveDir = pPawn->pPrevPawn->PlayerPos - pPawn->PlayerPos;

					MoveAngle = FMath::Acos(BallMoveDir.GetSafeNormal2D() | NextFrontDir.GetSafeNormal2D());
				}

				float HitTime = pNextAnim->GetTimeAtFrame(Frame);

				const FPoseMatchInfo& CurMatchPose = (*pMatchInfos)[Frame - 1];

				FVector HitDir;
				float FootSpeed;
				if (pNextAnim->BoneName == FName("Right_Ankle_Joint_01"))
				{
					FVector AnklePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Ankle_Joint_01"), BoneContainer);
					FVector KneePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Knee_Joint_01"), BoneContainer);
					FVector ToePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Toe_Joint_01"), BoneContainer);

					HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
					FootSpeed = CurMatchPose.RightFootVel.Size();
				}
				else
				{
					FVector AnklePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Left_Ankle_Joint_01"), BoneContainer);
					FVector KneePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Left_Knee_Joint_01"), BoneContainer);
					FVector ToePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Left_Toe_Joint_01"), BoneContainer);

					HitDir = ((KneePos - AnklePos) ^ (ToePos - AnklePos)).GetSafeNormal();
					FootSpeed = CurMatchPose.LeftFootVel.Size();
				}

				HitDir = NextPlayer.Align.TransformVector(HitDir);

				{
					FVector HitVel = HitDir * FootSpeed;

					TArray<FVector> BallTrajectory;
					GenerateBallTrajectory(BallTrajectory, pPawn->PlayerPos, HitVel);

					if (BallTrajectory.Num() == 0)
						continue;

					float Dist = (BallTrajectory.Last() - pPawn->pDestPawn->PlayerPos).Size();
					float HitAngle = FMath::Acos(PassDir | HitDir);
		
					float Cost =
						DiffTraj + Dist * DistWeight +
						HitAngle * HitAngleWeight +
						MoveAngle * MoveAngleWeight;

					if (MinCost > Cost)
					{
						MinCost = Cost;
						MinPlayer = NextPlayer;
						BestTrajectory = ClipTrajectory;
						//BestPoseMatchInfos = ClipMatchInfos;
						NextHitFrontDir = NextFrontDir;

						FootMoveDir1 = AlignQuat.RotateVector(MotionClip.MoveDir1);
						FootMoveDir2 = AlignQuat.RotateVector(MotionClip.MoveDir2);
					}
				}
			}
		}
	}

	NextAnimPlayer = MinPlayer;

	CreateNextHitInfo(NextAnimPlayer, pPawn, BoneContainer);
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::CreateNextPlayer(AVBM_Pawn* pPawn, bool bUseLeftFoot, int32 MotionType)
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

		TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pNextAnim);
		if (pMatchInfos == NULL)
			continue;

		if (bUseLeftFoot == true && pNextAnim->BoneName != FName("Left_Ankle_Joint_01"))
			continue;

		if (bUseLeftFoot == false && pNextAnim->BoneName != FName("Right_Ankle_Joint_01"))
			continue;

		if (MotionType == 1 && pNextAnim->GetName().Contains(FString("Attack")) == false)
			continue;

		if (MotionType == 2 && pNextAnim->GetName().Contains(FString("Pass")) == false)
			continue;

		if (MotionType == 3 && pNextAnim->GetName().Contains(FString("Toss")) == false)
			continue;

		if (MotionType != 1 && pNextAnim->GetName().Contains(FString("Attack")))
			continue;

		if (MotionType != 2 && pNextAnim->GetName().Contains(FString("Pass")))
			continue;

		if (MotionType != 3 && pNextAnim->GetName().Contains(FString("Toss")))
			continue;


		FBlendedCurve EmptyCurve;

		FCompactPose AnimPose;
		AnimPose.SetBoneContainer(&BoneContainer);

		for (auto& MotionClip : AnimMotionClip.Value)
		{
			float BeginTime = pNextAnim->GetTimeAtFrame(MotionClip.BeginFrame);

			FAnimPlayer NextPlayer(pNextAnim, BeginTime, 0.f, 1.f);
			{
				NextPlayer.MotionClip = MotionClip;
				NextPlayer.Align = CalcAlignTransoform(NextPlayer, BoneContainer);
			}

			TArray<FVector> ClipTrajectory;

			for (int32 Frame = MotionClip.HitBeginFrame; Frame < MotionClip.HitEndFrame; ++Frame)
			{
				float MoveAngle = 0.f;
				FVector NextFrontDir;

				if (pPawn->pPrevPawn)
				{
					float HitTime = NextPlayer.pAnim->GetTimeAtFrame(Frame);
					NextPlayer.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(HitTime), true);
					NextFrontDir = NextPlayer.Align.TransformVector(CalcFrontDir(AnimPose));

					FVector BallMoveDir = pPawn->pPrevPawn->PlayerPos - pPawn->PlayerPos;

					MoveAngle = FMath::Acos(BallMoveDir.GetSafeNormal2D() | NextFrontDir.GetSafeNormal2D());
				}

				float HitTime = pNextAnim->GetTimeAtFrame(Frame);

				const FPoseMatchInfo& CurMatchPose = (*pMatchInfos)[Frame - 1];

				FVector HitDir;
				float FootSpeed;
				if (pNextAnim->BoneName == FName("Right_Ankle_Joint_01"))
				{
					FVector AnklePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Ankle_Joint_01"), BoneContainer);
					FVector KneePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Knee_Joint_01"), BoneContainer);
					FVector ToePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Right_Toe_Joint_01"), BoneContainer);

					HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
					FootSpeed = CurMatchPose.RightFootVel.Size();
				}
				else
				{
					FVector AnklePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Left_Ankle_Joint_01"), BoneContainer);
					FVector KneePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Left_Knee_Joint_01"), BoneContainer);
					FVector ToePos = CalcBoneCSLocation(pNextAnim, HitTime, FName("Left_Toe_Joint_01"), BoneContainer);

					HitDir = ((KneePos - AnklePos) ^ (ToePos - AnklePos)).GetSafeNormal();
					FootSpeed = CurMatchPose.LeftFootVel.Size();
				}

				HitDir = NextPlayer.Align.TransformVector(HitDir);

				{
					FVector HitVel = HitDir * FootSpeed;

					TArray<FVector> BallTrajectory;
					GenerateBallTrajectory(BallTrajectory, pPawn->PlayerPos, HitVel);

					if (BallTrajectory.Num() == 0)
						continue;

					float Dist = (BallTrajectory.Last() - pPawn->pDestPawn->PlayerPos).Size();
					float HitAngle = FMath::Acos(PassDir | HitDir);

					float Cost =
						Dist * DistWeight +
						HitAngle * HitAngleWeight +
						MoveAngle * MoveAngleWeight;

					if (MinCost > Cost)
					{
						MinCost = Cost;
						MinPlayer = NextPlayer;
						BestTrajectory = ClipTrajectory;
						//BestPoseMatchInfos = ClipMatchInfos;
						NextHitFrontDir = NextFrontDir;
					}
				}
			}
		}
	}

	if (MinPlayer.pAnim == NULL)
	{
		return;
	}

	NextAnimPlayer = MinPlayer;

	CreateNextHitInfo(NextAnimPlayer, pPawn, BoneContainer);
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::PlayHitMotion(AVBM_Pawn* pPawn)
{
	if (NextAnimPlayer.pAnim == NULL)
		return;

	bIdleState = false;

	NextAnimPlayer.Time += pPawn->TimeError;
	BallTime = BallBeginTime + pPawn->TimeError;

	AnimPlayers.Last().Stop(0.f);
	AnimPlayers.Add(NextAnimPlayer);

	GenerateBallTrajectory(PassTrajectory2, pPawn->HitBallPos, pPawn->HitBallVel, pPawn->pDestPawn->HitBallPos);
	AdjustBallTrajectory(PassTrajectory2, pPawn->HitBallVel, pPawn->pDestPawn->HitBallPos);

	BallEndTime = float(PassTrajectory2.Num() - 1) * 0.033f;

	FVector BallMoveDir = (pPawn->pDestPawn->HitBallPos - pPawn->HitBallPos).GetSafeNormal();
	FVector BallRotAxis = (FVector::UpVector ^ BallMoveDir).GetSafeNormal();

	pPawn->HitAxisAng = BallRotAxis * pPawn->HitBallVel.Size() * RotRatio;
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
	CurPlayer.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(EndTime), true);

	FVector CurFrontDir = CalcFrontDir(AnimPose);
	CurFrontDir = CurPlayer.Align.TransformVector(CurFrontDir);

	AnimPose[FCompactPoseBoneIndex(0)] *= CurPlayer.Align;
	FVector CurHipPos = AnimPose[FCompactPoseBoneIndex(0)].GetTranslation();

	float BeginTime = NextPlayer.pAnim->GetTimeAtFrame(NextPlayer.MotionClip.BeginFrame);
	NextPlayer.pAnim->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(BeginTime), true);

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
	if (NextPlayer.pAnim == NULL)
		return;

	CalcHitDir(NextPlayer.pAnim, NextPlayer.MotionClip.HitBeginFrame, NextPlayer.MotionClip.HitEndFrame, RequiredBones);

	for (FVector& HitVel : NextHitVels)
	{
		HitVel = NextPlayer.Align.TransformVector(HitVel);
	}

	for (FVector& HitPos : NextHitPoss)
	{
		HitPos = NextPlayer.Align.TransformPosition(HitPos) + pPawn->GetActorLocation();
	}

	for (FVector& AxisAng : NextAxisAngs)
	{
		AxisAng = NextPlayer.Align.TransformVector(AxisAng);
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
			pPawn->HitAxisAng = NextAxisAngs[IdxHit];
			PassTrajectory = Trajectory;

			float HitTime = NextPlayer.pAnim->GetTimeAtFrame(NextPlayer.MotionClip.HitBeginFrame + IdxHit);
			float BeginTime = NextPlayer.pAnim->GetTimeAtFrame(NextPlayer.MotionClip.BeginFrame);

			//BallTime = BeginTime - HitTime;
			BallBeginTime = BeginTime - HitTime;
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

		FVector HitDir;
		if (pAnim->BoneName == FName("Right_Ankle_Joint_01"))
		{
			FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Ankle_Joint_01"), RequiredBones);
			FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Knee_Joint_01"), RequiredBones);
			FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Toe_Joint_01"), RequiredBones);

			HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
		}
		else
		{
			FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Ankle_Joint_01"), RequiredBones);
			FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Knee_Joint_01"), RequiredBones);
			FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Toe_Joint_01"), RequiredBones);

			HitDir = ((KneePos - AnklePos) ^ (ToePos - AnklePos)).GetSafeNormal();
		}

		TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pAnim);
		if (pMatchInfos != NULL)
		{
			if (pAnim->BoneName == FName("Right_Ankle_Joint_01"))
			{
				float Speed = (*pMatchInfos)[Frame - 1].RightFootVel.Size();

				AvgHitDir += HitDir * Speed;
			}
			else
			{
				float Speed = (*pMatchInfos)[Frame - 1].LeftFootVel.Size();

				AvgHitDir += HitDir * Speed;
			}
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

		FVector HitDir;
		if (pAnim->BoneName == FName("Right_Ankle_Joint_01"))
		{
			FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Ankle_Joint_01"), RequiredBones);
			FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Knee_Joint_01"), RequiredBones);
			FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Toe_Joint_01"), RequiredBones);

			HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
		}
		else
		{
			FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Ankle_Joint_01"), RequiredBones);
			FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Knee_Joint_01"), RequiredBones);
			FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Toe_Joint_01"), RequiredBones);

			HitDir = ((KneePos - AnklePos) ^ (ToePos - AnklePos)).GetSafeNormal();
		}

		TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pAnim);
		if (pMatchInfos != NULL)
		{
			float Speed;

			if (pAnim->BoneName == FName("Right_Ankle_Joint_01"))
			{
				Speed = (*pMatchInfos)[Frame - 1].RightFootVel.Size();
			}
			else
			{
				Speed = (*pMatchInfos)[Frame - 1].LeftFootVel.Size();
			}

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
	if (pAnim == NULL)
		return;

	NextHitVels.Empty();
	NextHitPoss.Empty();
	NextAxisAngs.Empty();

	for (int32 Frame = BeginFrame; Frame < EndFrame; ++Frame)
	{
		float HitTime = pAnim->GetTimeAtFrame(Frame);

		//FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Ankle_Joint_01"), RequiredBones);
		//FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Knee_Joint_01"), RequiredBones);
		//FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Toe_Joint_01"), RequiredBones);

		FVector HitPos;
		FVector HitDir;
		FVector RotAxis;


		if (pAnim->BoneName == FName("Right_Ankle_Joint_01"))
		{
			FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Ankle_Joint_01"), RequiredBones);
			FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Knee_Joint_01"), RequiredBones);
			FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Right_Toe_Joint_01"), RequiredBones);

			HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
			RotAxis = ((KneePos - AnklePos) ^ HitDir).GetSafeNormal();
			HitPos = ToePos;

			USkeletalMeshSocket* pHitSocket = pAnim->GetSkeleton()->FindSocket("Right_Hit_Socket");
			if (pHitSocket != NULL)
			{
				FTransform ToeTrans = CalcBoneCSTransform(pAnim, HitTime, FName("Right_Ankle_Joint_01"), BoneContainer);
				HitPos = (pHitSocket->GetSocketLocalTransform() * ToeTrans).GetTranslation();
			}
		}
		else
		{
			FVector AnklePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Ankle_Joint_01"), RequiredBones);
			FVector KneePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Knee_Joint_01"), RequiredBones);
			FVector ToePos = CalcBoneCSLocation(pAnim, HitTime, FName("Left_Toe_Joint_01"), RequiredBones);

			HitDir = ((KneePos - AnklePos) ^ (ToePos - AnklePos)).GetSafeNormal();
			RotAxis = (HitDir ^ (KneePos - AnklePos)).GetSafeNormal();
			HitPos = ToePos;

			USkeletalMeshSocket* pHitSocket = pAnim->GetSkeleton()->FindSocket("Left_Hit_Socket");
			if (pHitSocket != NULL)
			{
				FTransform ToeTrans = CalcBoneCSTransform(pAnim, HitTime, FName("Left_Ankle_Joint_01"), BoneContainer);
				HitPos = (pHitSocket->GetSocketLocalTransform() * ToeTrans).GetTranslation();
			}
		}

		if (HitPos.Z < BallRadius)
			continue;

		//FVector HitDir = ((ToePos - AnklePos) ^ (KneePos - AnklePos)).GetSafeNormal();
		//FVector RotAxis = ((KneePos - AnklePos) ^ HitDir).GetSafeNormal();

		TArray<FPoseMatchInfo>* pMatchInfos = AnimPoseInfos.Find(pAnim);
		if (pMatchInfos != NULL)
		{
			float Speed;

			if (pAnim->BoneName == FName("Right_Ankle_Joint_01"))
			{
				Speed = (*pMatchInfos)[Frame - 1].RightFootVel.Size();
			}
			else
			{
				Speed = (*pMatchInfos)[Frame - 1].LeftFootVel.Size();
			}

			NextHitVels.Add(HitDir * Speed);
			NextHitPoss.Add(HitPos);
			NextAxisAngs.Add(RotAxis * Speed * RotRatio);
		}
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::GenerateBallTrajectory(TArray<FVector>& OutTrajectory, const FVector& BeginPos, const FVector& BeginVel)
{
	float Pz0 = BeginPos.Z;
	float Vz0 = BeginVel.Z;

	float Eq0 = Vz0 * Vz0 + 2.f * GRAVITY * Pz0;
	if (Eq0 < 0.f)
		return;

	float t1 = (Vz0 + FMath::Sqrt(Eq0)) / GRAVITY;

	float Vz1 = GRAVITY * t1 - Vz0;

	float Eq1 = Vz1 * Vz1 - 2.f * GRAVITY * Pz0;
	if (Eq1 < 0.f)
		return;

	float t2 = (Vz1 + FMath::Sqrt(Eq1)) / GRAVITY;

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
	int32 NumIter = 0;
	int32 MaxIter = 10000;

	FVector NewBeginVel = BeginVel;

	while (Trajectory.Num() > 0 && NumIter < MaxIter)
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

		if (MinDist < Threshold)
		{
			Trajectory.SetNum(MinIndex + 1);
			break;
		}

		FVector DeltaDir = MinDir * 0.01f;

		NewBeginVel += DeltaDir;
		if (NewBeginVel.Z < 0.f)
		{
			NewBeginVel.Z = 0.f;
		}

		GenerateBallTrajectory(Trajectory, Trajectory[0], NewBeginVel, EndPos);

		++NumIter;
	}
}

//-------------------------------------------------------------------------------------------------
void FAnimNode_VBM::GenerateBallTrajectory(
	TArray<FVector>& OutTrajectory, const FVector& BeginPos, const FVector& BeginVel, const FVector& EndPos)
{
	FVector AlignedHorVel = (EndPos - BeginPos).GetSafeNormal2D() * BeginVel.Size2D();

	float Pz0 = BeginPos.Z;
	float Vz0 = BeginVel.Z;
	
	float Eq0 = Vz0 * Vz0 + 2.f * GRAVITY * (Pz0 - BallRadius);
	if (Eq0 < 0.f)
		return;

	//float Pz1 = Pz0;
	float Pz1 = BallRadius;

	float t1 = (Vz0 + FMath::Sqrt(Eq0)) / GRAVITY;
	float Vz1 = (GRAVITY * t1 - Vz0) * BallElasicity;

	float Eq2 = Vz1 * Vz1 - 2.f * GRAVITY * (Pz1 - BallRadius);
	if (Eq2 < 0.f)
		return;

	float t2 = (Vz1 + FMath::Sqrt(Eq2)) / GRAVITY;

	FVector BallPos;
	TArray<FVector> BallPoss;

	float Vx = AlignedHorVel.X;
	float Vy = AlignedHorVel.Y;

	for (float Time = 0.f; Time < t1 + t2; Time += 0.033f)
	{
		Vx = AlignedHorVel.X - Vx * AirReistance * Time;
		Vy = AlignedHorVel.Y - Vy * AirReistance * Time;

		BallPos.X = BeginPos.X + Vx * Time;
		BallPos.Y = BeginPos.Y + Vy * Time;

		if (Time < t1)
		{
			BallPos.Z = BeginPos.Z + (Vz0 - 0.5f * GRAVITY * Time) * Time;
		}
		else
		{
			float dt = Time - t1;

			BallPos.Z = BallRadius + (Vz1 - 0.5f * GRAVITY * dt) * dt;
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

		DrawDebugLine(GWorld, Pos1, Pos2, Color, false, -1, 0);
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

		pAnimSeq->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(AnimTime), true);

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

		pAnimSeq->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(AnimTime), true);

		AnimPose[FCompactPoseBoneIndex(0)] *= Align;

		DrawPose(AnimPose, BoneColor);
	}
}

//-------------------------------------------------------------------------------------------------
FTransform FAnimNode_VBM::CalcBoneCSTransform(const UAnimSequence* pAnimSeq, float AnimTime, const FName& BoneName, const FBoneContainer& BoneCont)
{
	FBlendedCurve EmptyCurve;

	FCompactPose AnimPose;
	AnimPose.SetBoneContainer(&BoneCont);

	pAnimSeq->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(AnimTime), true);

	FCSPose<FCompactPose> CSPose;
	CSPose.InitPose(AnimPose);

	const FReferenceSkeleton& RefSkel = pAnimSeq->GetSkeleton()->GetReferenceSkeleton();

	int32 BoneIndex = RefSkel.FindBoneIndex(BoneName);

	return CSPose.GetComponentSpaceTransform(FCompactPoseBoneIndex(BoneIndex));
}

//-------------------------------------------------------------------------------------------------
FVector FAnimNode_VBM::CalcBoneCSLocation(const UAnimSequence* pAnimSeq, float AnimTime, const FName& BoneName, const FBoneContainer& BoneCont)
{
	FBlendedCurve EmptyCurve;

	FCompactPose AnimPose;
	AnimPose.SetBoneContainer(&BoneCont);

	pAnimSeq->GetBonePose(AnimPose, EmptyCurve, FAnimExtractContext(AnimTime), true);

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

		pAnimSeq->GetBonePose(PrevAnimPose, EmptyCurve, FAnimExtractContext(PrevAnimTime), true);
		pAnimSeq->GetBonePose(CurAnimPose, EmptyCurve, FAnimExtractContext(CurAnimTime), true);

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

	float AlignedDegree = 0.f;
	FTransform AlignTransform = FTransform::Identity;
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
		PoseMatchInfo.AlignTransform = AlignTransform;

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

	for (int32 IdxInfo = BeginFrame - 1; IdxInfo < EndFrame; IdxInfo += FrameStep)
	{
		if (IdxInfo < 0)
			continue;

		const FPoseMatchInfo& MatchInfo = MatchInfos[IdxInfo];

		DrawDebugLine(GWorld, MatchInfo.RootPos, MatchInfo.RootPos + MatchInfo.RootVel, FColor::Black);
		DrawDebugLine(GWorld, MatchInfo.LeftFootPos, MatchInfo.LeftFootPos + MatchInfo.LeftFootVel, FColor::Magenta);
		DrawDebugLine(GWorld, MatchInfo.RightFootPos, MatchInfo.RightFootPos + MatchInfo.RightFootVel, FColor::Red);
		DrawDebugLine(GWorld, MatchInfo.LeftHandPos, MatchInfo.LeftHandPos + MatchInfo.LeftHandVel, FColor::Purple);
		DrawDebugLine(GWorld, MatchInfo.RightHandPos, MatchInfo.RightHandPos + MatchInfo.RightHandVel, FColor::Blue);

		DrawDebugPoint(GWorld, MatchInfo.RootPos, 5, FColor::Black);
		DrawDebugPoint(GWorld, MatchInfo.LeftFootPos, 5, FColor::Magenta);
		DrawDebugPoint(GWorld, MatchInfo.RightFootPos, 5, FColor::Red);
		DrawDebugPoint(GWorld, MatchInfo.LeftHandPos, 5, FColor::Purple);
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
void FAnimNode_VBM::AnalyzeMotion(const FPoseMatchInfo& IdleMatchInfo, const FBoneContainer& RequiredBones)
{
	bool bUseLeftFoot;

	for (const auto& AnimPoseInfo : AnimPoseInfos)
	{
		UAnimSequence* pAnim = AnimPoseInfo.Key;

		if (pAnim->BoneName == FName("Right_Ankle_Joint_01"))
		{
			bUseLeftFoot = false;
		}
		else
		{
			bUseLeftFoot = true;
		}

		TArray<int32> LocalMaxIndices;

		const TArray<FPoseMatchInfo>& PoseMatchInfos = AnimPoseInfo.Value;

		int NumInfos = PoseMatchInfos.Num() - 1;

		if (bUseLeftFoot)
		{
			for (int32 IdxInfo = 1; IdxInfo < NumInfos; ++IdxInfo)
			{
				float FootHeight = PoseMatchInfos[IdxInfo].LeftFootPos.Z;

				if (FootHeight > LimitMaxHeight &&
					FootHeight > PoseMatchInfos[IdxInfo - 1].LeftFootPos.Z &&
					FootHeight > PoseMatchInfos[IdxInfo + 1].LeftFootPos.Z)
				{
					LocalMaxIndices.Add(IdxInfo);
				}
			}
		}
		else
		{
			for (int32 IdxInfo = 1; IdxInfo < NumInfos; ++IdxInfo)
			{
				float FootHeight = PoseMatchInfos[IdxInfo].RightFootPos.Z;

				if (FootHeight > LimitMaxHeight &&
					FootHeight > PoseMatchInfos[IdxInfo - 1].RightFootPos.Z &&
					FootHeight > PoseMatchInfos[IdxInfo + 1].RightFootPos.Z)
				{
					LocalMaxIndices.Add(IdxInfo);
				}
			}
		}
		

		TArray<FAnimSection> HitSections;

		for (int32 MaxIndex : LocalMaxIndices)
		{
			int32 PoseIndex = MaxIndex;

			FAnimSection HitSec;

			while (true)
			{
				--PoseIndex;

				if (bUseLeftFoot)
				{
					if (PoseMatchInfos[PoseIndex].LeftFootPos.Z < pAnim->LimitMinPos)
					{
						HitSec.BeginFrame = PoseIndex + 1;
						break;
					}
				}
				else
				{
					if (PoseMatchInfos[PoseIndex].RightFootPos.Z < pAnim->LimitMinPos)
					{
						HitSec.BeginFrame = PoseIndex + 1;
						break;
					}
				}
			}

			PoseIndex = MaxIndex;

			while (true)
			{
				++PoseIndex;

				if (bUseLeftFoot)
				{
					if (PoseMatchInfos[PoseIndex].LeftFootPos.Z < pAnim->LimitMaxPos)
					{
						HitSec.EndFrame = PoseIndex + 1;
						break;
					}
				}
				else
				{
					if (PoseMatchInfos[PoseIndex].RightFootPos.Z < pAnim->LimitMaxPos)
					{
						HitSec.EndFrame = PoseIndex + 1;
						break;
					}
				}
			}

			int32 MaxFrame = -1;
			float MaxSpeed = 0.f;

			for (int32 Frame = HitSec.BeginFrame - 10; Frame < MaxIndex; ++Frame)
			{
				float Speed;

				if (bUseLeftFoot)
				{
					Speed = PoseMatchInfos[Frame].LeftFootVel.Size();
				}
				else
				{
					Speed = PoseMatchInfos[Frame].RightFootVel.Size();
				}

				if (MaxSpeed < Speed)
				{
					MaxSpeed = Speed;
					MaxFrame = Frame;
				}
			}

			check(MaxFrame >= 0);

			//HitSec.EndFrame = (HitSec.EndFrame + MaxIndex) * 0.5f;
			//if (AnimPoseInfo.Key->GetName().Contains(FString("Attack")))
			//{
			//	HitSec.BeginFrame = MaxIndex - 3;
			//	HitSec.EndFrame = MaxIndex + 3;
			//}
			//else
			//{
			//	HitSec.BeginFrame = (HitSec.BeginFrame + MaxIndex) / 2;
			//	HitSec.EndFrame = MaxIndex;
			//}

			//HitSec.BeginFrame = HitSec.BeginFrame - 1;
			HitSec.BeginFrame = MaxFrame;
			HitSec.EndFrame = MaxIndex;

			if (HitSec.BeginFrame < HitSec.EndFrame)
			{
				HitSections.Add(HitSec);
			}
		}

		if (HitSections.Num() > 0)
		{
			AnimHitSections.Add(AnimPoseInfo.Key, HitSections);
		}

		TArray<FAnimSection> MoveSections;

		for (int32 MaxIndex : LocalMaxIndices)
		{
			int32 PoseIndex = MaxIndex;

			FAnimSection AnimSec;
			AnimSec.MaxFrame = MaxIndex + 1;

			while (true)
			{
				--PoseIndex;

				if (bUseLeftFoot)
				{
					if (PoseMatchInfos[PoseIndex].LeftFootPos.Z < PoseMatchInfos[PoseIndex - 1].LeftFootPos.Z &&
						PoseMatchInfos[PoseIndex].LeftFootPos.Z < PoseMatchInfos[PoseIndex + 1].LeftFootPos.Z)
					{
						AnimSec.BeginFrame = PoseIndex + 1;
						break;
					}
				}
				else
				{
					if (PoseMatchInfos[PoseIndex].RightFootPos.Z < PoseMatchInfos[PoseIndex - 1].RightFootPos.Z &&
						PoseMatchInfos[PoseIndex].RightFootPos.Z < PoseMatchInfos[PoseIndex + 1].RightFootPos.Z)
					{
						AnimSec.BeginFrame = PoseIndex + 1;
						break;
					}
				}
			}

			PoseIndex = MaxIndex;

			while (true)
			{
				++PoseIndex;

				if (bUseLeftFoot)
				{
					if (PoseMatchInfos[PoseIndex].LeftFootPos.Z < PoseMatchInfos[PoseIndex - 1].LeftFootPos.Z &&
						PoseMatchInfos[PoseIndex].LeftFootPos.Z < PoseMatchInfos[PoseIndex + 1].LeftFootPos.Z)
					{
						AnimSec.EndFrame = PoseIndex + 1;
						break;
					}
				}
				else
				{
					if (PoseMatchInfos[PoseIndex].RightFootPos.Z < PoseMatchInfos[PoseIndex - 1].RightFootPos.Z &&
						PoseMatchInfos[PoseIndex].RightFootPos.Z < PoseMatchInfos[PoseIndex + 1].RightFootPos.Z)
					{
						AnimSec.EndFrame = PoseIndex + 1;
						break;
					}
				}
			}

			if (AnimSec.BeginFrame < AnimSec.EndFrame)
			{
				MoveSections.Add(AnimSec);
			}
		}

		if (MoveSections.Num() > 0)
		{
			AnimHitSections.Add(AnimPoseInfo.Key, HitSections);
		}

		TArray<int32> MatchFrames;

		int NumHit = HitSections.Num();
		for (int32 IdxHit = 0; IdxHit <= NumHit; ++IdxHit)
		{
			int32 BeginFrame = 0;
			if (IdxHit > 0)
			{
				BeginFrame = HitSections[IdxHit - 1].EndFrame;
			}

			int32 EndFrame = PoseMatchInfos.Num();
			if (IdxHit < NumHit)
			{
				EndFrame = HitSections[IdxHit].BeginFrame;
			}

			float MinCost = FLT_MAX;
			int32 MinFrame = -1;

			for (int32 Frame = BeginFrame; Frame < EndFrame; ++Frame)
			{
				float Cost = CalcMatchCost(PoseMatchInfos[Frame], IdleMatchInfo);

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

			int32 NumMove = MoveSections.Num();
			for (int32 IdxMove = 0; IdxMove < NumMove; ++IdxMove)
			{
				int32 MoveBegin = MoveSections[IdxMove].BeginFrame;
				int32 MoveEnd = MoveSections[IdxMove].EndFrame;

				if (MotionClip.BeginFrame < MoveBegin && MoveEnd < MotionClip.EndFrame)
				{
					MotionClip.MoveBeginFrame = MoveBegin;
					MotionClip.MoveEndFrame = MoveEnd;
					MotionClip.MoveMidFrame = MoveSections[IdxMove].MaxFrame;
					break;
				}
			}

			MotionClip.MaxSpeed = 0.f;
			MotionClip.MoveDist = 0.f;

			if (bUseLeftFoot)
			{
				for (int32 Frame = MotionClip.MoveBeginFrame; Frame < MotionClip.MoveEndFrame; ++Frame)
				{
					float FootSpeed = PoseMatchInfos[Frame - 1].LeftFootVel.Size();
					if (MotionClip.MaxSpeed < FootSpeed)
					{
						MotionClip.MaxSpeed = FootSpeed;
					}

					MotionClip.MoveDist += PoseMatchInfos[Frame - 1].LeftFootVel.Size() / 30.f;
				}
			}
			else
			{
				for (int32 Frame = MotionClip.MoveBeginFrame; Frame < MotionClip.MoveEndFrame; ++Frame)
				{
					float FootSpeed = PoseMatchInfos[Frame - 1].RightFootVel.Size();
					if (MotionClip.MaxSpeed < FootSpeed)
					{
						MotionClip.MaxSpeed = FootSpeed;
					}

					MotionClip.MoveDist += PoseMatchInfos[Frame - 1].RightFootVel.Size() / 30.f;
				}
			}

			FVector BeginPos;
			FVector MidPos;
			FVector EndPos;
			if (bUseLeftFoot)
			{
				BeginPos = CalcBoneCSLocation(AnimPoseInfo.Key, MotionClip.MoveBeginFrame * 0.033f, FName("Left_Ankle_Joint_01"), RequiredBones);
				MidPos = CalcBoneCSLocation(AnimPoseInfo.Key, MotionClip.MoveMidFrame * 0.033f, FName("Left_Ankle_Joint_01"), RequiredBones);
				EndPos = CalcBoneCSLocation(AnimPoseInfo.Key, MotionClip.MoveEndFrame * 0.033f, FName("Left_Ankle_Joint_01"), RequiredBones);
			}
			else
			{
				BeginPos = CalcBoneCSLocation(AnimPoseInfo.Key, MotionClip.MoveBeginFrame * 0.033f, FName("Right_Ankle_Joint_01"), RequiredBones);
				MidPos = CalcBoneCSLocation(AnimPoseInfo.Key, MotionClip.MoveMidFrame * 0.033f, FName("Right_Ankle_Joint_01"), RequiredBones);
				EndPos = CalcBoneCSLocation(AnimPoseInfo.Key, MotionClip.MoveEndFrame * 0.033f, FName("Right_Ankle_Joint_01"), RequiredBones);
			}

			FVector HorDir = (MidPos - BeginPos).GetSafeNormal2D();
			FVector MoveDir1 = (MidPos - BeginPos).GetSafeNormal();
			FVector MoveDir2 = (EndPos - MidPos).GetSafeNormal();

			//if ((MoveDir1.GetSafeNormal2D() | MoveDir2.GetSafeNormal2D()) > 0.f)
			if (AnimPoseInfo.Key->GetName().Contains(FString("Attack")))
			{
				MotionClip.bAttack = true;
			}
			else
			{
				MotionClip.bAttack = false;
			}

			MotionClip.Angle1 = FMath::Acos(MoveDir1 | HorDir);
			MotionClip.Angle2 = FMath::Acos(MoveDir1 | MoveDir2);

			MotionClip.MoveDir1 = MidPos - BeginPos;
			MotionClip.MoveDir2 = EndPos - MidPos;

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