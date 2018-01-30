// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VBM_HUDBase.generated.h"

/**
 * 
 */
UCLASS()
class VIRTUALBALLMANAGER_API AVBM_HUDBase : public AHUD
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = Settings)
	UAnimSequence* Anim;

	UPROPERTY(EditAnywhere, Category = Settings)
	float ScaleY = 20.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	float LimitVel = 10.f;

	UPROPERTY(EditAnywhere, Category = Settings)
	FName BoneName = "Right_Ankle_Joint_01";

public:

	UFUNCTION(BlueprintCallable)
	void AnalyzeMotion(const UAnimInstance* AnimInst);

	/** The Main Draw loop for the hud.  Gets called before any messaging.  Should be subclassed */
	virtual void DrawHUD() override;	

	void SearchHitFrames(int32 BoneIndex, const FBoneContainer& BoneContainer);

	void SearchMatchFrames();

protected:

	TArray<float> BoneVels;
	TArray<int32> HitFrames;
	TArray<int32> MatchFrames;

public:


};
