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

		int recvsize = recv(sock, buf, sizeof(buf), 0);
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
			BoneVelList[BoneIdx] = CurPosList[BoneIdx] - BonePosList[BoneIdx];
			BonePosList[BoneIdx] = CurPosList[BoneIdx];
		}

		bReceivePose = true;
	}
}

//-------------------------------------------------------------------------------------------------
void DrawBone(NUI_SKELETON_POSITION_INDEX joint0, NUI_SKELETON_POSITION_INDEX joint1)
{
	if (BonePosList.Num() > 0)
	{
		DrawDebugLine(GWorld, BonePosList[joint0], BonePosList[joint1], BoneColor, false, -1.f, 0, 3.f);
	}
}

//-------------------------------------------------------------------------------------------------
void DrawPose()
{
	// Render Torso
	DrawBone(NUI_SKELETON_POSITION_HEAD, NUI_SKELETON_POSITION_SHOULDER_CENTER);
	DrawBone(NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT);
	DrawBone(NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT);
	DrawBone(NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE);
	DrawBone(NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_HIP_CENTER);
	DrawBone(NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT);
	DrawBone(NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT);

	// Left Arm
	DrawBone(NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT);
	DrawBone(NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT);
	DrawBone(NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);

	// Right Arm
	DrawBone(NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT);
	DrawBone(NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT);
	DrawBone(NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);

	// Left Leg
	DrawBone(NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT);
	DrawBone(NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT);
	DrawBone(NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT);

	// Right Leg
	DrawBone(NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT);
	DrawBone(NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT);
	DrawBone(NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT);
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

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::Tick(float DeltaSeconds)
{
	DrawPose();

	if (bReceivePose)
	{
		PosePosList.Add(BonePosList);
		PoseVelList.Add(BoneVelList);

		if (PosePosList.Num() > 100)
		{
			PosePosList.RemoveAt(0);
			PoseVelList.RemoveAt(0);
		}

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

	SelectPoseMatchByUser(NUI_SKELETON_POSITION_ANKLE_RIGHT);

	// show current ball trajectory
	//for (auto& BallCtrl : BallCtrls)
	//{
	//	if (BallCtrl.BallTrajectory.Num() == 0 ||
	//		BallCtrl.GetBallPos() == BallCtrl.BallTrajectory[0] ||
	//		BallCtrl.GetBallPos() == BallCtrl.BallTrajectory.Last())
	//		continue;

	//	for (const FVector& BallPos : BallCtrl.BallTrajectory)
	//	{
	//		DrawDebugSphere(GetWorld(), BallPos, 15.f, 8, FColor::Black);
	//	}
	//}

	if (PassOrders.Num() < 3)
		return;

	if (PassOrders[0]->pDestPawn == NULL && PassPoses.Num() > 0)
	{
		PassOrders[0]->pDestPawn = PassOrders[1];
		PassOrders[0]->CreateNextPlayer(PassPoses[0]);
	}

	if (PassOrders[1]->pDestPawn == NULL && PassPoses.Num() > 1)
	{
		PassOrders[1]->pPrevPawn = PassOrders[0];
		PassOrders[1]->pDestPawn = PassOrders[2];
		PassOrders[1]->CreateNextPlayer(PassPoses[1]);

		PassOrders[0]->TimeError += DeltaSeconds;
		PassOrders[0]->PlayHitMotion();

		FBallController BallCtrl;
		{
			BallCtrl.BallAxisAng = PassOrders[0]->HitAxisAng;
			BallCtrl.BallTime = PassOrders[0]->pAnimNode->BallTime;
			BallCtrl.BallTrajectory = PassOrders[0]->pAnimNode->PassTrajectory2;
		}
		BallCtrls.Add(BallCtrl);
	}

	if (PassOrders[0]->bBeginNextMotion && PassPoses.Num() > 1)
	{
		PassOrders[0]->pDestPawn = NULL;
		PassOrders.RemoveAt(0);
		PassPoses.RemoveAt(0);
	}
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::GeneratePoseMatchInfo(
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
		AlignPos.Z = 0.f;

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
}

//-------------------------------------------------------------------------------------------------
void AVirtualBallManagerGameModeBase::SelectPoseMatchByUser(int32 BoneIndex)
{
	TArray<TPair<int32, int32>> HitSections;
	{
		TPair<int32, int32> Section;

		int32 NumPoses = FMath::Min(100, PoseVelList.Num());
		for (int32 PoseIndex = 1; PoseIndex < NumPoses; ++PoseIndex)
		{
			float PreSpeed = PoseVelList[PoseIndex - 1][BoneIndex].Size() * 30.f;
			float CurSpeed = PoseVelList[PoseIndex][BoneIndex].Size() * 30.f;

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

	int32 MaxIndex = 0;
	float MaxBoneSpeed = 0.f;

	for (int32 PoseIndex = HitSections[0].Key; PoseIndex < HitSections[0].Value; ++PoseIndex)
	{
		float BoneSpeed = PoseVelList[PoseIndex][BoneIndex].Size() * 30.f;

		if (BoneSpeed > MaxBoneSpeed)
		{
			MaxIndex = PoseIndex;
			MaxBoneSpeed = BoneSpeed;
		}
	}

	FPoseMatchInfo UserPose;
	GeneratePoseMatchInfo(UserPose, PosePosList[MaxIndex], PoseVelList[MaxIndex]);

	PassPoses.Add(UserPose);

	PosePosList.Empty();
	PoseVelList.Empty();
}


#pragma optimize("", on)