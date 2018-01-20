// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimGraphNode_VBM.h"
#include "VBM_AnimInstance.h"



/////////////////////////////////////////////////////
// UAnimGraphNode_VBM

#define LOCTEXT_NAMESPACE "A3Nodes"

FString UAnimGraphNode_VBM::GetNodeCategory() const
{
	return TEXT("VBM_Node");
}

FLinearColor UAnimGraphNode_VBM::GetNodeTitleColor() const
{
	return FLinearColor(0.75f, 0.75f, 0.75f);
}

FText UAnimGraphNode_VBM::GetTooltipText() const
{
	return LOCTEXT("MMTooltip", "VBM_Node");
}

FText UAnimGraphNode_VBM::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("MMTitle", "VBM_Node");
}

#undef LOCTEXT_NAMESPACE