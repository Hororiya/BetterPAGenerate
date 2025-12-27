#include "BetterPAConstraintGraphNode.h"

FText UBetterPAConstraintGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromName(BoneName);
}

FLinearColor UBetterPAConstraintGraphNode::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.8f, 0.2f);
}

void UBetterPAConstraintGraphNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "Constraint", FName("In"));
	CreatePin(EGPD_Output, "Constraint", FName("Out"));
}
