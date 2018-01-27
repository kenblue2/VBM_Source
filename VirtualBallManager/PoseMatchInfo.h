#pragma once


//-------------------------------------------------------------------------------------------------
struct FHitSection
{
	int32 BeginFrame;
	int32 EndFrame;
};


//-------------------------------------------------------------------------------------------------
struct FPoseMatchInfo
{
	FVector RootVel;
	FVector LeftFootVel;
	FVector RightFootVel;
	FVector LeftHandVel;
	FVector RightHandVel;

	FVector RootPos;
	FVector LeftFootPos;
	FVector RightFootPos;
	FVector LeftHandPos;
	FVector RightHandPos;

	float AlignedDegree;

	//---------------------------------------------------------------------------------------------
	FVector CalcOriginalVel(const FVector& Vel) const
	{
		return FRotator(0, AlignedDegree, 0).RotateVector(Vel);
	}

	//---------------------------------------------------------------------------------------------
	float ClacDiff(const FPoseMatchInfo& Other) const
	{
		return
			(RootVel - Other.RootVel).Size() +
			(LeftFootVel - Other.LeftFootVel).Size() +
			(RightFootVel - Other.RightFootVel).Size() +
			(LeftHandVel - Other.LeftHandVel).Size() +
			(RightHandVel - Other.RightHandVel).Size() +
			(LeftFootPos - Other.LeftFootPos).Size() +
			(LeftFootPos - Other.LeftFootPos).Size() +
			(RightFootPos - Other.RightFootPos).Size() +
			(RightHandPos - Other.RightHandPos).Size();
	}
};


//-------------------------------------------------------------------------------------------------
struct FMotionClip
{
	int32 BeginFrame;
	int32 EndFrame;

	int32 HitBeginFrame;
	int32 HitEndFrame;
};


//-------------------------------------------------------------------------------------------------
struct FAnimPlayer
{
	class UAnimSequence* pAnim;

	bool bStop;

	float Time;
	float Weight;

	float BlendInTime;
	float BlendOutTime;

	struct FTransform Align;

	FMotionClip MotionClip;

public:

	FAnimPlayer()
		: bStop(false)
		, pAnim(NULL)
		, Weight(0.f)
		, Align(FTransform::Identity)
	{
	}

	//---------------------------------------------------------------------------------------------
	FAnimPlayer(UAnimSequence* _pAnim, float BeginTime = 0.f, float BlendTime = 0.3f, float _Weight = 0.f)
		: bStop(false)
		, pAnim(_pAnim)
		, Weight(_Weight)
		, Time(BeginTime)
		, BlendInTime(BlendTime)
		, Align(FTransform::Identity)
	{
	}

	//---------------------------------------------------------------------------------------------
	void Update(float DeltaTime)
	{
		Time += DeltaTime;

		if (bStop)
		{
			Weight -= (DeltaTime / BlendOutTime);
		}
		else
		{
			Weight += (DeltaTime / BlendInTime);
		}

		Weight = FMath::Clamp(Weight, 0.f, 1.f);
	}

	//---------------------------------------------------------------------------------------------
	void Stop(float BlendTime = 0.3f)
	{
		bStop = true;
		BlendOutTime = BlendTime;
	}
};