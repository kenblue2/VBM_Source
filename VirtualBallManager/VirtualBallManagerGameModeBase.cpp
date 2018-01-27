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


SOCKET serv_sock;

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


FVector g_BonePosList[NUI_SKELETON_POSITION_COUNT];

FColor BoneColor = FColor::Green;


//-------------------------------------------------------------------------------------------------
void __cdecl RecvThread(void * p)
{
	SOCKET sock = (SOCKET)p;
	char buf[256];
	while (true)
	{
		int recvsize = recv(sock, buf, sizeof(buf), 0);
		if (recvsize <= 0)
		{
			//printf("접속종료\n");
			break;
		}
		buf[recvsize] = '\0';
		//printf("\rserver >> %s\n>> ", buf);

		memcpy(g_BonePosList, buf, sizeof(g_BonePosList));
	}
}

//-------------------------------------------------------------------------------------------------
void DrawBone(NUI_SKELETON_POSITION_INDEX joint0, NUI_SKELETON_POSITION_INDEX joint1)
{
	if (g_BonePosList != NULL)
	{
		DrawDebugLine(GWorld, g_BonePosList[joint0] * 100.f, g_BonePosList[joint1] * 100.f, BoneColor, false, -1.f, 0, 1.f);
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

	for (auto& BallCtrl : BallCtrls)
	{
		BallCtrl.Update(DeltaSeconds);
	}

	if (BallCtrls.Num() >= 2 && BallCtrls.Last().BallTime > 0.f)
	{
		BallCtrls.RemoveAt(0);
	}

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

	if (PassOrders[0]->pDestPawn == NULL)
	{
		PassOrders[0]->pDestPawn = PassOrders[1];
		PassOrders[0]->CreateNextPlayer();
	}

	if (PassOrders[1]->pDestPawn == NULL)
	{
		PassOrders[1]->pPrevPawn = PassOrders[0];
		PassOrders[1]->pDestPawn = PassOrders[2];
		PassOrders[1]->CreateNextPlayer();

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

	if (PassOrders[0]->bBeginNextMotion)
	{
		PassOrders[0]->pDestPawn = NULL;
		PassOrders.RemoveAt(0);
	}
}


#pragma optimize("", on)