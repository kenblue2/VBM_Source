// Fill out your copyright notice in the Description page of Project Settings.

#include "VirtualBallManagerGameModeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "VBM_AnimInstance.h"
#include "AnimNode_VBM.h"
#include "VBM_Pawn.h"


#include <WinSock2.h>
#include <process.h>


#pragma optimize("", off)


typedef enum _NUI_SKELETON_POSITION_INDEX
{
	NUI_SKELETON_POSITION_HIP_CENTER = 0,
	NUI_SKELETON_POSITION_SPINE = (NUI_SKELETON_POSITION_HIP_CENTER + 1),
	NUI_SKELETON_POSITION_SHOULDER_CENTER = (NUI_SKELETON_POSITION_SPINE + 1),
	NUI_SKELETON_POSITION_HEAD = (NUI_SKELETON_POSITION_SHOULDER_CENTER + 1),
	NUI_SKELETON_POSITION_SHOULDER_LEFT = (NUI_SKELETON_POSITION_HEAD + 1),
	NUI_SKELETON_POSITION_ELBOW_LEFT = (NUI_SKELETON_POSITION_SHOULDER_LEFT + 1),
	NUI_SKELETON_POSITION_WRIST_LEFT = (NUI_SKELETON_POSITION_ELBOW_LEFT + 1),
	NUI_SKELETON_POSITION_HAND_LEFT = (NUI_SKELETON_POSITION_WRIST_LEFT + 1),
	NUI_SKELETON_POSITION_SHOULDER_RIGHT = (NUI_SKELETON_POSITION_HAND_LEFT + 1),
	NUI_SKELETON_POSITION_ELBOW_RIGHT = (NUI_SKELETON_POSITION_SHOULDER_RIGHT + 1),
	NUI_SKELETON_POSITION_WRIST_RIGHT = (NUI_SKELETON_POSITION_ELBOW_RIGHT + 1),
	NUI_SKELETON_POSITION_HAND_RIGHT = (NUI_SKELETON_POSITION_WRIST_RIGHT + 1),
	NUI_SKELETON_POSITION_HIP_LEFT = (NUI_SKELETON_POSITION_HAND_RIGHT + 1),
	NUI_SKELETON_POSITION_KNEE_LEFT = (NUI_SKELETON_POSITION_HIP_LEFT + 1),
	NUI_SKELETON_POSITION_ANKLE_LEFT = (NUI_SKELETON_POSITION_KNEE_LEFT + 1),
	NUI_SKELETON_POSITION_FOOT_LEFT = (NUI_SKELETON_POSITION_ANKLE_LEFT + 1),
	NUI_SKELETON_POSITION_HIP_RIGHT = (NUI_SKELETON_POSITION_FOOT_LEFT + 1),
	NUI_SKELETON_POSITION_KNEE_RIGHT = (NUI_SKELETON_POSITION_HIP_RIGHT + 1),
	NUI_SKELETON_POSITION_ANKLE_RIGHT = (NUI_SKELETON_POSITION_KNEE_RIGHT + 1),
	NUI_SKELETON_POSITION_FOOT_RIGHT = (NUI_SKELETON_POSITION_ANKLE_RIGHT + 1),
	NUI_SKELETON_POSITION_COUNT = (NUI_SKELETON_POSITION_FOOT_RIGHT + 1)
} 	NUI_SKELETON_POSITION_INDEX;


SOCKET serv_sock;


TArray<FVector> BonePosList;
TArray<FVector> BoneVelList;

FColor BoneColor = FColor::Blue;

bool bReceivePose = false;

//-------------------------------------------------------------------------------------------------
void __cdecl RecvThread(void * p)
{
	SOCKET sock = (SOCKET)p;
	char buf[1024];
	while (true)
	{
		while (bReceivePose);

		int recvsize = recv(sock, buf, sizeof(buf) - 1, 0);
		if (recvsize <= 0)
		{
			//printf("접속종료\n");
			break;
		}
		buf[recvsize] = '\0';
		//printf("\rserver >> %s\n>> ", buf);

		FVector CurPosList[NUI_SKELETON_POSITION_COUNT];
		memcpy(&CurPosList[0], buf, sizeof(CurPosList));

		BonePosList.SetNum(NUI_SKELETON_POSITION_COUNT);
		BoneVelList.SetNum(NUI_SKELETON_POSITION_COUNT);

		for (int32 BoneIdx = 0; BoneIdx < BoneVelList.Num(); ++BoneIdx)
		{
			BoneVelList[BoneIdx] = (CurPosList[BoneIdx] - BonePosList[BoneIdx]) * 30.f;
			BonePosList[BoneIdx] = CurPosList[BoneIdx];
		}

		bReceivePose = true;
	}
}

//-------------------------------------------------------------------------------------------------
void DrawBone(const TArray<FVector>& Pose, NUI_SKELETON_POSITION_INDEX joint0, NUI_SKELETON_POSITION_INDEX joint1)
{
	if (BonePosList.Num() > 0)
	{
		DrawDebugLine(GWorld, Pose[joint0], Pose[joint1], FColor::White, false, -1.f, 0, 2.f);
	}
}

//-------------------------------------------------------------------------------------------------
void DrawPose(const TArray<FVector>& Pose)
{
	if (Pose.Num() < NUI_SKELETON_POSITION_COUNT)
		return;

	// Render Torso
	DrawBone(Pose, NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
	DrawBone(Pose, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
	DrawBone(Pose, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);
	DrawBone(Pose, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
	DrawBone(Pose, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);
	DrawBone(Pose, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT);
	DrawBone(Pose, NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT);

	// Left Arm
	DrawBone(Pose, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT);
	DrawBone(Pose, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT);
	DrawBone(Pose, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);

	// Right Arm
	DrawBone(Pose, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT);
	DrawBone(Pose, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT);
	DrawBone(Pose, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);

	// Left Leg
	DrawBone(Pose, NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT);
	DrawBone(Pose, NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT);
	DrawBone(Pose, NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT);

	// Right Leg
	DrawBone(Pose, NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT);
	DrawBone(Pose, NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT);
	DrawBone(Pose, NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT);
}


//-------------------------------------------------------------------------------------------------
AVirtualBallManagerGameModeBase::AVirtualBallManagerGameModeBase()
	: pPrevPawn(NULL)
	, LimitSpeed(200.f)
{
	PrimaryActorTick.bCanEverTick = true;
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	WSADATA wsaData;
	int retval = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (retval != 0)
	{
		printf("WSAStartup() Error\n");
		return;
	}

	serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(4000);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	retval = connect(serv_sock, (SOCKADDR*)&serv_addr, sizeof(SOCKADDR));
	if (retval == SOCKET_ERROR)
	{
		printf("connect() Error\n");
		return;
	}

	_beginthread(RecvThread, NULL, (void*)serv_sock);
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	closesocket(serv_sock);
	WSACleanup();
}

//-------------------------------------------------------------------------------------------------
FVector AVirtualBallManagerGameModeBase::GetBallPos()
{
	FVector BallPos = FVector::ZeroVector;

	for (auto& BallCtrl : BallCtrls)
	{
		if (BallCtrl.BallTime > 0.f)
		{
			BallPos = BallCtrl.GetBallPos();
		}
	}

	return BallPos;
}

//-------------------------------------------------------------------------------------------------
FRotator AVirtualBallManagerGameModeBase::GetBallRot(float DeltaTime)
{
	FVector AxisAng = FVector::ZeroVector;

	for (auto& BallCtrl : BallCtrls)
	{
		if (BallCtrl.BallTime > 0.f)
		{
			AxisAng = BallCtrl.BallAxisAng;
		}
	}

	return FQuat(AxisAng.GetSafeNormal(), AxisAng.Size() * DeltaTime).Rotator();
}

void DrawMatchInfo(const FPoseMatchInfo& MatchInfo)
{
	DrawDebugLine(GWorld, MatchInfo.RootPos, MatchInfo.RootPos + MatchInfo.RootVel, FColor::White);
	DrawDebugLine(GWorld, MatchInfo.LeftFootPos, MatchInfo.LeftFootPos + MatchInfo.LeftFootVel, FColor::Magenta);
	DrawDebugLine(GWorld, MatchInfo.RightFootPos, MatchInfo.RightFootPos + MatchInfo.RightFootVel, FColor::Cyan);
	//DrawDebugLine(GWorld, MatchInfo.LeftHandPos, MatchInfo.LeftHandPos + MatchInfo.LeftHandVel, FColor::Red);
	//DrawDebugLine(GWorld, MatchInfo.RightHandPos, MatchInfo.RightHandPos + MatchInfo.RightHandVel, FColor::Blue);

	DrawDebugPoint(GWorld, MatchInfo.RootPos, 5, FColor::White);
	DrawDebugPoint(GWorld, MatchInfo.LeftFootPos, 5, FColor::Magenta);
	DrawDebugPoint(GWorld, MatchInfo.RightFootPos, 5, FColor::Cyan);
	//DrawDebugPoint(GWorld, MatchInfo.LeftHandPos, 5, FColor::Red);
	//DrawDebugPoint(GWorld, MatchInfo.RightHandPos, 5, FColor::Blue);
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawPose(BonePosList);

	/*for (const FVector& FootPos: FootTrajectory)
	{
		DrawDebugSphere(GWorld, FootPos, 3.f, 4, FColor::White);
	}

	for (int32 IdxPos = 1; IdxPos < FootTrajectory.Num(); ++IdxPos)
	{
		DrawDebugLine(GWorld, FootTrajectory[IdxPos - 1], FootTrajectory[IdxPos], FColor::White, false, -1.f, 0, 1.f);
	}*/

	/*
	for (int32 IdxPos = 1; IdxPos < PosePosList.Num(); ++IdxPos)
	{
		const FVector& Pos1 = PosePosList[IdxPos - 1][NUI_SKELETON_POSITION_ANKLE_RIGHT];
		const FVector& Pos2 = PosePosList[IdxPos][NUI_SKELETON_POSITION_ANKLE_RIGHT];

		DrawDebugLine(GWorld, Pos1, Pos2, FColor::Cyan, false, -1.f, 0, 1.f);
	}

	for (auto& PosePos : PosePosList)
	{
		DrawPose(PosePos);
	}

	for (auto& MatchInfo : UserMatchInfos)
	{
		DrawMatchInfo(MatchInfo);
	}*/

	if (bReceivePose)
	{
		PosePosList.Add(BonePosList);
		PoseVelList.Add(BoneVelList);

		if (PosePosList.Num() > MaxUserFrame)
		{
			PosePosList.RemoveAt(0);
			PoseVelList.RemoveAt(0);
		}

		SelectPoseMatchByUser3(NUI_SKELETON_POSITION_ANKLE_LEFT);
		SelectPoseMatchByUser3(NUI_SKELETON_POSITION_ANKLE_RIGHT);

		bReceivePose = false;
	}

	for (auto& BallCtrl : BallCtrls)
	{
		BallCtrl.Update(DeltaSeconds);
	}

	if (BallCtrls.Num() >= 2 && BallCtrls.Last().BallTime > 0.f)
	{
		BallCtrls.RemoveAt(0);
	}

	// show current ball trajectory
	for (auto& BallCtrl : BallCtrls)
	{
		if (BallCtrl.BallTrajectory.Num() == 0 ||
			BallCtrl.GetBallPos() == BallCtrl.BallTrajectory[0] ||
			BallCtrl.GetBallPos() == BallCtrl.BallTrajectory.Last())
			continue;

		for (const FVector& BallPos : BallCtrl.BallTrajectory)
		{
			DrawDebugSphere(GetWorld(), BallPos, 2.f, 8, FColor::Black);
		}
	}

	if (PassOrders.Num() < 3 || FootTrajectories.Num() < 2)
		return;

	if (PassOrders.Last() == PassOrders[PassOrders.Num() - 2])
	{
		PassOrders.Pop();
		return;
	}

	if (PassOrders[0]->pDestPawn == NULL && PassOrders[0]->bBeginNextMotion == false)
	{
		PassOrders[0]->pDestPawn = PassOrders[1];
		
		//PassOrders[0]->CreateNextPlayer();
		PassOrders[0]->CreateNextPlayer(FootTrajectories[0], bUseLeftFootList[0]);
	}

	if (PassOrders[1]->pDestPawn == NULL)
	{
		PassOrders[1]->pPrevPawn = PassOrders[0];
		PassOrders[1]->pDestPawn = PassOrders[2];

		//PassOrders[1]->CreateNextPlayer();
		PassOrders[1]->CreateNextPlayer(FootTrajectories[1], bUseLeftFootList[1]);

		if (PassOrders[0]->pDestPawn != NULL)
		{
			PassOrders[0]->TimeError += DeltaSeconds;
			PassOrders[0]->PlayHitMotion();
		}

		FBallController BallCtrl;
		{
			BallCtrl.BallAxisAng = PassOrders[0]->HitAxisAng;
			BallCtrl.BallTime = PassOrders[0]->pAnimNode->BallTime;
			BallCtrl.BallTrajectory = PassOrders[0]->pAnimNode->PassTrajectory2;
		}
		BallCtrls.Add(BallCtrl);
	}

	if (PassOrders[0]->bBeginNextMotion)
	{
		PassOrders[0]->pDestPawn = NULL;
		
		PassOrders.RemoveAt(0);

		if (bUseLeftFootList.Num() > 0)
		{
			bUseLeftFootList.RemoveAt(0);
		}
		
		if (FootTrajectories.Num() > 0)
		{
			FootTrajectories.RemoveAt(0);
		}
	}
}

//-------------------------------------------------------------------------------------------------
FTransform AVirtualBallManagerGameModeBase::GeneratePoseMatchInfo(
	FPoseMatchInfo& OutInfo, const TArray<FVector>& CSPosList, const TArray<FVector> CSVelList)
{
	FVector HipPos = CSPosList[NUI_SKELETON_POSITION_HIP_CENTER];
	FVector LFootPos = CSPosList[NUI_SKELETON_POSITION_ANKLE_LEFT];
	FVector RFootPos = CSPosList[NUI_SKELETON_POSITION_ANKLE_RIGHT];
	FVector LHandPos = CSPosList[NUI_SKELETON_POSITION_WRIST_LEFT];
	FVector RHandPos = CSPosList[NUI_SKELETON_POSITION_WRIST_RIGHT];

	FVector HipVel = CSVelList[NUI_SKELETON_POSITION_HIP_CENTER];
	FVector LFootVel = CSVelList[NUI_SKELETON_POSITION_ANKLE_LEFT];
	FVector RFootVel = CSVelList[NUI_SKELETON_POSITION_ANKLE_RIGHT];
	FVector LHandVel = CSVelList[NUI_SKELETON_POSITION_WRIST_LEFT];
	FVector RHandVel = CSVelList[NUI_SKELETON_POSITION_WRIST_RIGHT];

	FVector FrontDir;
	{
		FVector LThighPos = CSPosList[NUI_SKELETON_POSITION_HIP_LEFT];
		FVector RThighPos = CSPosList[NUI_SKELETON_POSITION_HIP_RIGHT];
		
		FVector Dir1 = LThighPos - HipPos;
		FVector Dir2 = RThighPos - HipPos;

		FrontDir = (Dir1 ^ Dir2).GetSafeNormal2D();
	}

	float AlignedDegree;
	FTransform AlignTransform;
	{
		FVector AlignPos = HipPos;
		//AlignPos.Z = 0.f;
		AlignPos.Z = HipPos.Z - CSPosList[NUI_SKELETON_POSITION_FOOT_LEFT].Z;

		FQuat AlignQuat = FQuat::FindBetween(FrontDir, FVector::ForwardVector);

		AlignedDegree = -AlignQuat.Rotator().Yaw;
		AlignTransform = FTransform(-AlignPos) * FTransform(AlignQuat);
	}

	OutInfo.AlignedDegree = AlignedDegree;

	OutInfo.RootPos = AlignTransform.TransformPosition(HipPos);
	OutInfo.LeftFootPos = AlignTransform.TransformPosition(LFootPos);
	OutInfo.RightFootPos = AlignTransform.TransformPosition(RFootPos);
	OutInfo.LeftHandPos = AlignTransform.TransformPosition(LHandPos);
	OutInfo.RightHandPos = AlignTransform.TransformPosition(RHandPos);

	OutInfo.RootVel = AlignTransform.TransformVector(HipVel);
	OutInfo.LeftFootVel = AlignTransform.TransformVector(LFootVel);
	OutInfo.RightFootVel = AlignTransform.TransformVector(RFootVel);
	OutInfo.LeftHandVel = AlignTransform.TransformVector(LHandVel);
	OutInfo.RightHandVel = AlignTransform.TransformVector(RHandVel);

	return AlignTransform;
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::SelectPoseMatchByUser(int32 BoneIndex)
{
	TArray<TPair<int32, int32>> HitSections;
	{
		TPair<int32, int32> Section;

		int32 NumPoses = PoseVelList.Num();
		for (int32 PoseIndex = 1; PoseIndex < NumPoses; ++PoseIndex)
		{
			float PreSpeed = PoseVelList[PoseIndex - 1][BoneIndex].Size();
			float CurSpeed = PoseVelList[PoseIndex][BoneIndex].Size();

			if (PreSpeed < LimitSpeed && CurSpeed > LimitSpeed)
			{
				Section.Key = PoseIndex;
			}
			else if (PreSpeed > LimitSpeed && CurSpeed < LimitSpeed)
			{
				Section.Value = PoseIndex;

				if (Section.Key < Section.Value && (Section.Value - Section.Key) > 3)
				{
					HitSections.Add(Section);
				}
			}
		}
	}

	if (HitSections.Num() < 2)
		return;

	FootTrajectory.Empty();
	{
		int32 MaxIndex = 0;
		int32 MinIndex = 0;

		float MaxSpeed = 0.f;
		float MinSpeed = FLT_MAX;

		for (int32 PoseIndex = HitSections[0].Key; PoseIndex < HitSections[1].Key; ++PoseIndex)
		{
			float BoneSpeed = PoseVelList[PoseIndex][BoneIndex].Size();

			if (BoneSpeed > MaxSpeed)
			{
				MaxIndex = PoseIndex;
				MaxSpeed = BoneSpeed;
			}

			if (BoneSpeed < MinSpeed)
			{
				MinIndex = PoseIndex;
				MinSpeed = BoneSpeed;
			}
		}

		//UserMatchInfos.Empty();

		for (int32 PoseIndex = MaxIndex; PoseIndex <= MinIndex; ++PoseIndex)
		{
			//FPoseMatchInfo MatchInfo;
			//GeneratePoseMatchInfo(MatchInfo, PosePosList[PoseIndex], PoseVelList[PoseIndex]);

			//FootTrajectory.Add(MatchInfo.RightFootPos);
			//FootTrajectory.Add(MatchInfo.RightFootVel);

			//UserMatchInfos.Add(MatchInfo);

			FVector FootPos = PosePosList[PoseIndex][NUI_SKELETON_POSITION_ANKLE_RIGHT];
			FootTrajectory.Add(FootPos);
		}
	}

	if (FootTrajectory.Num() > 0)
	{
		FootTrajectories.Add(FootTrajectory);
	}

	PosePosList.Empty();
	PoseVelList.Empty();
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::SelectPoseMatchByUser2(int32 BoneIndex)
{
	FAnimSection HitSec;

	int32 NumPose = PosePosList.Num();
	if (NumPose < 2)
		return;

	float PreBoneHeight = PosePosList[NumPose - 2][BoneIndex].Z;
	float CurBoneHeight = PosePosList[NumPose - 1][BoneIndex].Z;

	if (PreBoneHeight > LimitMinHeight && CurBoneHeight < LimitMinHeight)
	{
		HitSec.EndFrame = NumPose - 1;
	}
	else
	{
		return;
	}

	float MaxHeight = 0.f;
	int32 MaxPoseIndex = -1;

	for (int32 IdxPose = HitSec.EndFrame; IdxPose > 0; --IdxPose)
	{
		float BoneHeight = PosePosList[IdxPose][BoneIndex].Z;

		if (BoneHeight > LimitMaxHeight)
		{
			if (MaxHeight < BoneHeight)
			{
				MaxHeight = BoneHeight;
				MaxPoseIndex = IdxPose;
			}
		}

		if (MaxPoseIndex > 0 && BoneHeight < LimitMinHeight)
		{
			HitSec.BeginFrame = IdxPose;
			break;
		}
	}

	if (MaxPoseIndex < 0 || HitSec.BeginFrame < 0 || HitSec.EndFrame <= HitSec.BeginFrame)
		return;

	FootTrajectory.Empty();
	for (int32 PoseIndex = HitSec.BeginFrame; PoseIndex < HitSec.EndFrame; ++PoseIndex)
	{
		FVector FootPos = PosePosList[PoseIndex][BoneIndex];
		FootTrajectory.Add(FootPos);
	}

	if (FootTrajectory.Num() > 0)
	{
		FootTrajectories.Add(FootTrajectory);

		if (BoneIndex == NUI_SKELETON_POSITION_ANKLE_LEFT)
		{
			bUseLeftFootList.Add(true);
		}
		else
		{
			bUseLeftFootList.Add(false);
		}
	}

	PosePosList.Empty();
	PoseVelList.Empty();
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::SelectPoseMatchByUser3(int32 BoneIndex)
{
	FAnimSection HitSec;

	int32 NumPose = PosePosList.Num();
	if (NumPose < 2)
		return;

	float PreBoneHeight = PosePosList[NumPose - 2][BoneIndex].Z;
	float CurBoneHeight = PosePosList[NumPose - 1][BoneIndex].Z;

	if (PreBoneHeight > LimitMinHeight && CurBoneHeight < LimitMinHeight)
	{
		HitSec.EndFrame = NumPose - 1;
	}
	else
	{
		return;
	}

	float MaxHeight = 0.f;
	int32 MaxPoseIndex = -1;

	for (int32 IdxPose = HitSec.EndFrame; IdxPose > 1; --IdxPose)
	{
		float CurBoneHeight = PosePosList[IdxPose][BoneIndex].Z;
		//float PreBoneHeight = PosePosList[IdxPose - 1][BoneIndex].Z;

		if (CurBoneHeight > LimitMaxHeight)
		{
			if (MaxHeight < CurBoneHeight)
			{
				MaxHeight = CurBoneHeight;
				MaxPoseIndex = IdxPose;
			}
		}

		if (MaxPoseIndex > 0 && CurBoneHeight < LimitMinHeight)
		{
			HitSec.BeginFrame = IdxPose;
			break;
		}
	}

	if (MaxPoseIndex < 0 || HitSec.BeginFrame < 0 || HitSec.EndFrame <= HitSec.BeginFrame)
		return;

	for (int32 IdxPose = HitSec.BeginFrame; IdxPose > 0; --IdxPose)
	{
		float CurHeight = PosePosList[IdxPose][BoneIndex].Z;
		float PreHeight = PosePosList[IdxPose - 1][BoneIndex].Z;

		if (PreHeight > CurHeight)
		{
			HitSec.BeginFrame = IdxPose;
			break;
		}
	}

	for (int32 IdxPose = HitSec.EndFrame; IdxPose < NumPose; ++IdxPose)
	{
		float CurHeight = PosePosList[IdxPose][BoneIndex].Z;
		float PreHeight = PosePosList[IdxPose - 1][BoneIndex].Z;

		if (PreHeight < CurHeight)
		{
			HitSec.EndFrame = IdxPose;
			break;
		}
	}

	HitSec.EndFrame = PosePosList.Num();

	FootTrajectory.Empty();
	for (int32 PoseIndex = HitSec.BeginFrame; PoseIndex < HitSec.EndFrame; ++PoseIndex)
	{
		FVector FootPos = PosePosList[PoseIndex][BoneIndex];
		FootTrajectory.Add(FootPos);
	}

	if (FootTrajectory.Num() > 0)
	{
		FootTrajectories.Add(FootTrajectory);

		if (BoneIndex == NUI_SKELETON_POSITION_ANKLE_LEFT)
		{
			bUseLeftFootList.Add(true);
		}
		else
		{
			bUseLeftFootList.Add(false);
		}
	}

	PosePosList.Empty();
	PoseVelList.Empty();
}


#pragma optimize("", on)