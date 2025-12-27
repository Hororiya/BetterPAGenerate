#include "SBetterPAConstraintGraph.h"
#include "EdGraph/EdGraph.h"
#include "SGraphPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "BetterPAConstraintGraphSchema.h"
#include "BetterPAConstraintGraphNode.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GraphEditor.h"
#include "AnimationRuntime.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"

void SBetterPAConstraintGraph::Construct(const FArguments& InArgs)
{
	PhysicsAsset = InArgs._PhysicsAsset;
	CurrentMode = EConstraintGenerationMode::Standard;
	bScaleByDistance = false;
	ScalingFactor = 1.0f;
	
	CreateGraph();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBorder)
					.Padding(4)
					[
						CreateBodyList()
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 4, 0, 0)
				[
					CreateSettingsPanel()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 4, 0, 0)
				[
					SNew(SButton)
					.Text(FText::FromString("Apply Constraints"))
					.OnClicked(this, &SBetterPAConstraintGraph::OnApplyChanges)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				GraphPanel.ToSharedRef()
			]
		]
	];
}

void SBetterPAConstraintGraph::CreateGraph()
{
	if (!GraphObj)
	{
		GraphObj = NewObject<UEdGraph>(GetTransientPackage(), NAME_None, RF_Transactional);
		GraphObj->Schema = UBetterPAConstraintGraphSchema::StaticClass();
		GraphObj->AddToRoot();
	}

	GraphPanel = SNew(SGraphPanel)
		.GraphObj(GraphObj);
}

TSharedRef<SWidget> SBetterPAConstraintGraph::CreateBodyList()
{
	TSharedPtr<SVerticalBox> ListBox = SNew(SVerticalBox);

	if (PhysicsAsset)
	{
		// Sort BodySetups by BoneName
		TArray<USkeletalBodySetup*> SortedBodySetups = PhysicsAsset->SkeletalBodySetups;
		SortedBodySetups.Sort([](const USkeletalBodySetup& A, const USkeletalBodySetup& B)
		{
			return A.BoneName.ToString() < B.BoneName.ToString();
		});

		for (int32 i = 0; i < SortedBodySetups.Num(); ++i)
		{
			USkeletalBodySetup* BodySetup = SortedBodySetups[i];
			if (BodySetup)
			{
				// We need the original index for the node creation, so we need to find it
				int32 OriginalIndex = PhysicsAsset->SkeletalBodySetups.Find(BodySetup);
				
				ListBox->AddSlot()
				.AutoHeight()
				.Padding(2)
				[
					SNew(SButton)
					.Text(FText::FromName(BodySetup->BoneName))
					.OnClicked(this, &SBetterPAConstraintGraph::OnAddBodyNode, BodySetup->BoneName, OriginalIndex)
				];
			}
		}
	}

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			ListBox.ToSharedRef()
		];
}

TSharedRef<SWidget> SBetterPAConstraintGraph::CreateSettingsPanel()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(FText::FromString("Constraint Settings"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SCheckBox)
			.Style(FCoreStyle::Get(), "RadioButton")
			.IsChecked(this, &SBetterPAConstraintGraph::GetModeCheckState, EConstraintGenerationMode::Standard)
			.OnCheckStateChanged(this, &SBetterPAConstraintGraph::OnModeChanged, EConstraintGenerationMode::Standard)
			[
				SNew(STextBlock).Text(FText::FromString("Standard (Locked/Limited)"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(SCheckBox)
			.Style(FCoreStyle::Get(), "RadioButton")
			.IsChecked(this, &SBetterPAConstraintGraph::GetModeCheckState, EConstraintGenerationMode::Mesh)
			.OnCheckStateChanged(this, &SBetterPAConstraintGraph::OnModeChanged, EConstraintGenerationMode::Mesh)
			[
				SNew(STextBlock).Text(FText::FromString("Mesh (Free/Limited)"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 2, 2, 2)
		[
			SNew(SVerticalBox)
			.IsEnabled(this, &SBetterPAConstraintGraph::IsMeshSettingsEnabled)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SBetterPAConstraintGraph::GetScaleByDistanceCheckState)
				.OnCheckStateChanged(this, &SBetterPAConstraintGraph::OnScaleByDistanceChanged)
				[
					SNew(STextBlock).Text(FText::FromString("Scale Linear Limit by Distance"))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 4, 0)
				[
					SNew(STextBlock).Text(FText::FromString("Scale Factor:"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpinBox<float>)
					.Value(this, &SBetterPAConstraintGraph::GetScalingFactor)
					.OnValueChanged(this, &SBetterPAConstraintGraph::OnScalingFactorChanged)
					.MinValue(0.1f)
					.MaxValue(10.0f)
				]
			]
		];
}

FReply SBetterPAConstraintGraph::OnAddBodyNode(FName BoneName, int32 BodyIndex)
{
	if (GraphObj)
	{
		UBetterPAConstraintGraphNode* NewNode = NewObject<UBetterPAConstraintGraphNode>(GraphObj);
		NewNode->BoneName = BoneName;
		NewNode->BodyIndex = BodyIndex;
		
		NewNode->CreateNewGuid();
		NewNode->NodePosX = 0;
		NewNode->NodePosY = 0;
		NewNode->AllocateDefaultPins();
		
		GraphObj->AddNode(NewNode);
		NewNode->GetGraph()->NotifyGraphChanged();
	}
	return FReply::Handled();
}

FReply SBetterPAConstraintGraph::OnApplyChanges()
{
	if (!PhysicsAsset || !GraphObj)
	{
		return FReply::Handled();
	}

	for (UEdGraphNode* Node : GraphObj->Nodes)
	{
		UBetterPAConstraintGraphNode* SourceNode = Cast<UBetterPAConstraintGraphNode>(Node);
		if (!SourceNode) continue;

		// Check outputs
		for (UEdGraphPin* Pin : SourceNode->Pins)
		{
			if (Pin->Direction == EGPD_Output)
			{
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					UBetterPAConstraintGraphNode* TargetNode = Cast<UBetterPAConstraintGraphNode>(LinkedPin->GetOwningNode());
					if (TargetNode)
					{
						FName Bone1Name = SourceNode->BoneName;
						FName Bone2Name = TargetNode->BoneName;

						// Check if constraint already exists
						bool bExists = false;
						for (UPhysicsConstraintTemplate* ExistingConstraint : PhysicsAsset->ConstraintSetup)
						{
							if ((ExistingConstraint->DefaultInstance.ConstraintBone1 == Bone1Name && ExistingConstraint->DefaultInstance.ConstraintBone2 == Bone2Name) ||
								(ExistingConstraint->DefaultInstance.ConstraintBone1 == Bone2Name && ExistingConstraint->DefaultInstance.ConstraintBone2 == Bone1Name))
							{
								bExists = true;
								break;
							}
						}

						if (bExists)
						{
							continue; // Skip existing
						}

						// Create Constraint between SourceNode and TargetNode
						UPhysicsConstraintTemplate* NewConstraint = NewObject<UPhysicsConstraintTemplate>(PhysicsAsset, NAME_None, RF_Transactional);
						
						NewConstraint->DefaultInstance.ConstraintBone1 = Bone1Name;
						NewConstraint->DefaultInstance.ConstraintBone2 = Bone2Name;
						
						NewConstraint->DefaultInstance.ProfileInstance.bDisableCollision = true;

						// Calculate Transforms
						FTransform T1 = FTransform::Identity;
						FTransform T2 = FTransform::Identity;
						bool bTransformsValid = false;
						
						// We need to find the center of the capsules, not just the bone locations.
						// The bone location is T1.GetLocation().
						// The capsule center is defined in the BodySetup AggGeom.
						FVector Body1Center = FVector::ZeroVector;
						FVector Body2Center = FVector::ZeroVector;

						if (USkeletalMesh* SkelMesh = PhysicsAsset->PreviewSkeletalMesh.Get())
						{
							const FReferenceSkeleton& RefSkeleton = SkelMesh->GetRefSkeleton();
							int32 BoneIndex1 = RefSkeleton.FindBoneIndex(Bone1Name);
							int32 BoneIndex2 = RefSkeleton.FindBoneIndex(Bone2Name);
							
							if (BoneIndex1 != INDEX_NONE && BoneIndex2 != INDEX_NONE)
							{
								TArray<FTransform> ComponentSpaceTransforms;
								FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, RefSkeleton.GetRefBonePose(), ComponentSpaceTransforms);
								
								T1 = ComponentSpaceTransforms[BoneIndex1];
								T2 = ComponentSpaceTransforms[BoneIndex2];
								bTransformsValid = true;
								
								// Find BodySetup for Bone1
								int32 BodyIndex1 = PhysicsAsset->FindBodyIndex(Bone1Name);
								if (BodyIndex1 != INDEX_NONE)
								{
									USkeletalBodySetup* BS1 = PhysicsAsset->SkeletalBodySetups[BodyIndex1];
									if (BS1 && BS1->AggGeom.SphylElems.Num() > 0)
									{
										// Use the first capsule's center (transformed to component space)
										// SphylElem.Center is in Bone Space (Local to T1)
										Body1Center = T1.TransformPosition(BS1->AggGeom.SphylElems[0].Center);
									}
									else
									{
										Body1Center = T1.GetLocation();
									}
								}
								
								// Find BodySetup for Bone2
								int32 BodyIndex2 = PhysicsAsset->FindBodyIndex(Bone2Name);
								if (BodyIndex2 != INDEX_NONE)
								{
									USkeletalBodySetup* BS2 = PhysicsAsset->SkeletalBodySetups[BodyIndex2];
									if (BS2 && BS2->AggGeom.SphylElems.Num() > 0)
									{
										Body2Center = T2.TransformPosition(BS2->AggGeom.SphylElems[0].Center);
									}
									else
									{
										Body2Center = T2.GetLocation();
									}
								}
							}
						}

						if (CurrentMode == EConstraintGenerationMode::Standard)
						{
							// Standard Mode: Locked Linear, Limited Angular (45 deg)
							NewConstraint->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 45.0f);
							NewConstraint->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 45.0f);
							NewConstraint->DefaultInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 45.0f);

							NewConstraint->DefaultInstance.SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
							NewConstraint->DefaultInstance.SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
							NewConstraint->DefaultInstance.SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);

							if (bTransformsValid)
							{
								// Constraint at Body2 (Target) location (Standard behavior: usually at child bone)
								
								// Pos1 (Relative to Body1/Parent)
								NewConstraint->DefaultInstance.Pos1 = T1.InverseTransformPosition(T2.GetLocation());
								
								// Orientation relative to Body1/Parent (aligned with Child)
								FQuat RelRot1 = T1.GetRotation().Inverse() * T2.GetRotation();
								NewConstraint->DefaultInstance.PriAxis1 = RelRot1.GetAxisX();
								NewConstraint->DefaultInstance.SecAxis1 = RelRot1.GetAxisY();

								// Pos2 (Relative to Body2/Child)
								NewConstraint->DefaultInstance.Pos2 = FVector::ZeroVector;
								
								// Orientation relative to Body2/Child (Identity)
								NewConstraint->DefaultInstance.PriAxis2 = FVector(1,0,0);
								NewConstraint->DefaultInstance.SecAxis2 = FVector(0,1,0);
							}
						}
						else if (CurrentMode == EConstraintGenerationMode::Mesh)
						{
							// Mesh Mode: Free Angular, Limited Linear
							NewConstraint->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0.0f);
							NewConstraint->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0.0f);
							NewConstraint->DefaultInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.0f);

							float LinearLimit = 0.0f;
							if (bTransformsValid)
							{
								// Distance between CENTERS of capsules
								float Distance = FVector::Dist(Body1Center, Body2Center);
								if (bScaleByDistance)
								{
									LinearLimit = Distance * ScalingFactor;
								}
								else
								{
									LinearLimit = 10.0f; 
								}
							}

							NewConstraint->DefaultInstance.SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, LinearLimit);
							NewConstraint->DefaultInstance.SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, LinearLimit);
							NewConstraint->DefaultInstance.SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, LinearLimit);

							if (bTransformsValid)
							{
								// Constraint at Midpoint of CENTERS
								FVector MidPoint = (Body1Center + Body2Center) * 0.5f;

								// Pos1 (Relative to Body1)
								NewConstraint->DefaultInstance.Pos1 = T1.InverseTransformPosition(MidPoint);
								// Orientation: Identity relative to world? Or aligned with connection?
								// Usually constraints are aligned with one of the bodies or the connection.
								// Let's align with Body1 for simplicity, or World.
								// If Free Angular, orientation matters less for limits, but matters for reference frames.
								NewConstraint->DefaultInstance.PriAxis1 = FVector(1,0,0);
								NewConstraint->DefaultInstance.SecAxis1 = FVector(0,1,0);

								// Pos2 (Relative to Body2)
								NewConstraint->DefaultInstance.Pos2 = T2.InverseTransformPosition(MidPoint);
								
								// Orientation relative to Body2 (to match Body1's frame at that point if we want them aligned)
								// If we use Body1's rotation as the constraint frame:
								FQuat RelRot = T2.GetRotation().Inverse() * T1.GetRotation();
								NewConstraint->DefaultInstance.PriAxis2 = RelRot.GetAxisX();
								NewConstraint->DefaultInstance.SecAxis2 = RelRot.GetAxisY();
							}
						}

						PhysicsAsset->ConstraintSetup.Add(NewConstraint);
					}
				}
			}
		}
	}

	PhysicsAsset->UpdateBodySetupIndexMap();
	PhysicsAsset->UpdateBoundsBodiesArray();
	PhysicsAsset->MarkPackageDirty();
	
	return FReply::Handled();
}

void SBetterPAConstraintGraph::OnModeChanged(ECheckBoxState NewState, EConstraintGenerationMode Mode)
{
	if (NewState == ECheckBoxState::Checked)
	{
		CurrentMode = Mode;
	}
}

ECheckBoxState SBetterPAConstraintGraph::GetModeCheckState(EConstraintGenerationMode Mode) const
{
	return CurrentMode == Mode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SBetterPAConstraintGraph::OnScaleByDistanceChanged(ECheckBoxState NewState)
{
	bScaleByDistance = (NewState == ECheckBoxState::Checked);
}

ECheckBoxState SBetterPAConstraintGraph::GetScaleByDistanceCheckState() const
{
	return bScaleByDistance ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SBetterPAConstraintGraph::OnScalingFactorChanged(float NewValue)
{
	ScalingFactor = NewValue;
}

float SBetterPAConstraintGraph::GetScalingFactor() const
{
	return ScalingFactor;
}

bool SBetterPAConstraintGraph::IsMeshSettingsEnabled() const
{
	return CurrentMode == EConstraintGenerationMode::Mesh;
}
