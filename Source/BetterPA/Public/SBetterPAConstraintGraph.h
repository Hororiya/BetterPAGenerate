#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "PhysicsEngine/PhysicsAsset.h"

class UPhysicsAsset;
class SGraphPanel;
class UEdGraph;

enum class EConstraintGenerationMode
{
	Standard,
	Mesh
};

class BETTERPA_API SBetterPAConstraintGraph : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBetterPAConstraintGraph) {}
		SLATE_ARGUMENT(UPhysicsAsset*, PhysicsAsset)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	UPhysicsAsset* PhysicsAsset;
	TSharedPtr<SGraphPanel> GraphPanel;
	UEdGraph* GraphObj;
	
	// Settings
	EConstraintGenerationMode CurrentMode;
	bool bScaleByDistance;
	float ScalingFactor;

	void CreateGraph();
	TSharedRef<SWidget> CreateBodyList();
	TSharedRef<SWidget> CreateSettingsPanel();
	
	FReply OnAddBodyNode(FName BoneName, int32 BodyIndex);
	FReply OnApplyChanges();

	// UI Callbacks
	void OnModeChanged(ECheckBoxState NewState, EConstraintGenerationMode Mode);
	ECheckBoxState GetModeCheckState(EConstraintGenerationMode Mode) const;
	
	void OnScaleByDistanceChanged(ECheckBoxState NewState);
	ECheckBoxState GetScaleByDistanceCheckState() const;
	
	void OnScalingFactorChanged(float NewValue);
	float GetScalingFactor() const;
	
	bool IsMeshSettingsEnabled() const;
};
