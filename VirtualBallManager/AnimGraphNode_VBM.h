// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimNode_VBM.h"
#include "AnimGraphNode_VBM.generated.h"

/**
 * 
 */
UCLASS()
class VIRTUALBALLMANAGER_API UAnimGraphNode_VBM : public UAnimGraphNode_Base
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_VBM Node;

	//~ Begin UEdGraphNode Interface.
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface.

	//~ Begin UAnimGraphNode_Base Interface
	virtual FString GetNodeCategory() const override;
	//~ End UAnimGraphNode_Base Interface
};
