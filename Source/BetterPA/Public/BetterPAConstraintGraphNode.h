#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "BetterPAConstraintGraphNode.generated.h"

class USkeletalBodySetup;

UCLASS()
class BETTERPA_API UBetterPAConstraintGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName BoneName;

	UPROPERTY()
	int32 BodyIndex;

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void AllocateDefaultPins() override;
	// End of UEdGraphNode interface
};
