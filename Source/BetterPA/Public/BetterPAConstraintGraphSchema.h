#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "BetterPAConstraintGraphSchema.generated.h"

UCLASS()
class BETTERPA_API UBetterPAConstraintGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	// UEdGraphSchema interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	// End of UEdGraphSchema interface
};
