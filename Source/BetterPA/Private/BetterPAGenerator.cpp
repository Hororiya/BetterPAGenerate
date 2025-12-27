#include "BetterPAGenerator.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "ReferenceSkeleton.h"


void FBetterPAGenerator::GeneratePhysicsAsset(USkeletalMesh* SkeletalMesh, UPhysicsAsset* PhysicsAsset, const TSet<FName>& SelectedBones)
{
	if (!SkeletalMesh || !PhysicsAsset)
	{
		return;
	}

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	const TArray<FMeshBoneInfo>& BoneInfo = RefSkeleton.GetRefBoneInfo();
	const TArray<FTransform>& BonePose = RefSkeleton.GetRefBonePose();

	// Calculate all component space transforms once
	TArray<FTransform> ComponentSpaceTransforms;
	FAnimationRuntime::FillUpComponentSpaceTransforms(RefSkeleton, BonePose, ComponentSpaceTransforms);

	PhysicsAsset->SkeletalBodySetups.Empty();
	PhysicsAsset->ConstraintSetup.Empty();

	// BFS Queue: Store Bone Indices
	TQueue<int32> BoneQueue;
	
	// Start from root (usually index 0)
	if (BoneInfo.Num() > 0)
	{
		BoneQueue.Enqueue(0);
	}

	// Map to store created BodySetups by BoneName for constraint generation
	TMap<FName, USkeletalBodySetup*> CreatedBodySetups;
	// Map to store Bone Index to created BodySetup
	TMap<int32, USkeletalBodySetup*> BoneIndexToBodySetup;

	while (!BoneQueue.IsEmpty())
	{
		int32 CurrentBoneIndex;
		BoneQueue.Dequeue(CurrentBoneIndex);

		FName BoneName = BoneInfo[CurrentBoneIndex].Name;

		// Find children
		TArray<int32> ChildrenIndices;
		for (int32 i = 0; i < BoneInfo.Num(); ++i)
		{
			if (BoneInfo[i].ParentIndex == CurrentBoneIndex)
			{
				ChildrenIndices.Add(i);
				BoneQueue.Enqueue(i);
			}
		}

		// Skip if not selected
		if (!SelectedBones.Contains(BoneName))
		{
			continue;
		}

		// Create Body Setup
		USkeletalBodySetup* NewBodySetup = NewObject<USkeletalBodySetup>(PhysicsAsset, NAME_None, RF_Transactional);
		NewBodySetup->BoneName = BoneName;
		NewBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
		
		FKSphylElem SphylElem;
		
		// Use pre-calculated component space transform
		FTransform CurrentBoneTransform = ComponentSpaceTransforms[CurrentBoneIndex];
		
		// Find the nearest selected child (or child of child)
		int32 TargetChildIndex = INDEX_NONE;
		
		// BFS to find the nearest selected descendant
		TQueue<int32> SearchQueue;
		for (int32 ChildIdx : ChildrenIndices)
		{
			SearchQueue.Enqueue(ChildIdx);
		}
		
		while(!SearchQueue.IsEmpty())
		{
			int32 CandidateIndex;
			SearchQueue.Dequeue(CandidateIndex);
			
			if (SelectedBones.Contains(BoneInfo[CandidateIndex].Name))
			{
				TargetChildIndex = CandidateIndex;
				break; // Found one
			}
			
			// If not selected, add its children to search
			for (int32 i = 0; i < BoneInfo.Num(); ++i)
			{
				if (BoneInfo[i].ParentIndex == CandidateIndex)
				{
					SearchQueue.Enqueue(i);
				}
			}
		}

		FQuat CapsuleRotation = FQuat::Identity;

		if (TargetChildIndex != INDEX_NONE)
		{
			FTransform ChildBoneTransform = ComponentSpaceTransforms[TargetChildIndex];
			
			FVector StartPos = CurrentBoneTransform.GetLocation();
			FVector EndPos = ChildBoneTransform.GetLocation();
			
			FVector MidPoint = (StartPos + EndPos) * 0.5f;
			float Length = FVector::Dist(StartPos, EndPos);
			
			// Transform MidPoint to Bone Space
			FVector LocalMidPoint = CurrentBoneTransform.InverseTransformPosition(MidPoint);
			
			// Orientation: Capsule should align with the bone to child vector
			FVector Direction = (EndPos - StartPos).GetSafeNormal();
			
			// Calculate rotation to align Z axis (Capsule axis) with Direction
			FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction); 
			
			// Convert to local rotation relative to bone
			FQuat LocalRotation = CurrentBoneTransform.GetRotation().Inverse() * Rotation;
			CapsuleRotation = Rotation; // Store world rotation for constraint

			SphylElem.Center = LocalMidPoint;
			SphylElem.Rotation = LocalRotation.Rotator();
			
			// Radius scales with length: 25cm length -> 3cm radius
			SphylElem.Radius = (Length / 25.0f) * 3.0f;
			SphylElem.Length = Length; 
		}
		else
		{
			// No selected children: Use parent's length if available, otherwise default
			float Length = 5.0f;
			
			// Try to find parent length
			// We need to find the distance from parent to this bone
			int32 ParentIndex = BoneInfo[CurrentBoneIndex].ParentIndex;
			// Trace back to find the nearest selected parent
			while (ParentIndex != INDEX_NONE)
			{
				if (SelectedBones.Contains(BoneInfo[ParentIndex].Name))
				{
					FTransform ParentTransform = ComponentSpaceTransforms[ParentIndex];
					Length = FVector::Dist(ParentTransform.GetLocation(), CurrentBoneTransform.GetLocation());
					break;
				}
				ParentIndex = BoneInfo[ParentIndex].ParentIndex;
			}

			SphylElem.Center = FVector::ZeroVector;
			
			// Align Z axis to Y axis (RightVector)
			FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, FVector::RightVector);
			CapsuleRotation = CurrentBoneTransform.GetRotation() * Rotation; // Store world rotation

			SphylElem.Rotation = Rotation.Rotator();
			SphylElem.Radius = (Length / 25.0f) * 3.0f;
			SphylElem.Length = Length;
		}

		NewBodySetup->AggGeom.SphylElems.Add(SphylElem);
		PhysicsAsset->SkeletalBodySetups.Add(NewBodySetup);
		CreatedBodySetups.Add(BoneName, NewBodySetup);
		BoneIndexToBodySetup.Add(CurrentBoneIndex, NewBodySetup);

		// Generate Constraint with Parent
		int32 ParentIndex = BoneInfo[CurrentBoneIndex].ParentIndex;
		FName ParentBoneName = NAME_None;
		int32 FoundParentIndex = INDEX_NONE;
		
		// Find nearest selected parent
		while (ParentIndex != INDEX_NONE)
		{
			if (SelectedBones.Contains(BoneInfo[ParentIndex].Name))
			{
				ParentBoneName = BoneInfo[ParentIndex].Name;
				FoundParentIndex = ParentIndex;
				break;
			}
			ParentIndex = BoneInfo[ParentIndex].ParentIndex;
		}

		if (CreatedBodySetups.Contains(ParentBoneName))
		{
			UPhysicsConstraintTemplate* NewConstraint = NewObject<UPhysicsConstraintTemplate>(PhysicsAsset, NAME_None, RF_Transactional);
			
			NewConstraint->DefaultInstance.ConstraintBone1 = BoneName; // Child
			NewConstraint->DefaultInstance.ConstraintBone2 = ParentBoneName; // Parent
			
			// Position at child joint
			// Pos1 is relative to Child Bone
			NewConstraint->DefaultInstance.Pos1 = FVector::ZeroVector;
			
			// Orientation: Same as child capsule orientation.
			// CapsuleRotation is in World Space (Component Space).
			// We need it relative to Child Bone.
			FQuat RelRot1 = CurrentBoneTransform.GetRotation().Inverse() * CapsuleRotation;
			NewConstraint->DefaultInstance.PriAxis1 = RelRot1.GetAxisX();
			NewConstraint->DefaultInstance.SecAxis1 = RelRot1.GetAxisY();

			// Set constraint transform relative to parent bone (Bone2)
			// Location: Child Bone Location relative to Parent Bone
			FTransform ParentTransform = ComponentSpaceTransforms[FoundParentIndex];
			FVector RelPos2 = ParentTransform.InverseTransformPosition(CurrentBoneTransform.GetLocation());
			NewConstraint->DefaultInstance.Pos2 = RelPos2;
			
			// Orientation: Same as child capsule orientation, but relative to Parent Bone.
			FQuat RelRot2 = ParentTransform.GetRotation().Inverse() * CapsuleRotation;
			NewConstraint->DefaultInstance.PriAxis2 = RelRot2.GetAxisX();
			NewConstraint->DefaultInstance.SecAxis2 = RelRot2.GetAxisY();

			// Limits
			// Angular: Limited 45 degrees
			NewConstraint->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 45.0f);
			NewConstraint->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 45.0f);
			NewConstraint->DefaultInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 45.0f);

			// Linear: Locked
			NewConstraint->DefaultInstance.SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
			NewConstraint->DefaultInstance.SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
			NewConstraint->DefaultInstance.SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
			
			// Disable collision between linked bodies
			NewConstraint->DefaultInstance.ProfileInstance.bDisableCollision = true;

			PhysicsAsset->ConstraintSetup.Add(NewConstraint);
		}
	}
	
	PhysicsAsset->UpdateBodySetupIndexMap();
	PhysicsAsset->UpdateBoundsBodiesArray();
	PhysicsAsset->MarkPackageDirty();
}
