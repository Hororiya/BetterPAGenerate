#include "BetterPAConstraintGraphSchema.h"
#include "BetterPAConstraintGraphNode.h"

void UBetterPAConstraintGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// Add actions here if needed (e.g. right click to add node)
}

const FPinConnectionResponse UBetterPAConstraintGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	// Prevent connecting to self
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect to self"));
	}

	// Prevent connecting input to input or output to output
	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Directions must be opposite"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("Create Constraint"));
}

bool UBetterPAConstraintGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	bool bModified = UEdGraphSchema::TryCreateConnection(A, B);
	
	if (bModified)
	{
		// Logic to actually create the constraint in the Physics Asset will go here or be triggered by an event
	}

	return bModified;
}
