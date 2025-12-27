#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;
class UPhysicsAsset;

class BETTERPA_API FBetterPAGenerator
{
public:
	static void GeneratePhysicsAsset(USkeletalMesh* SkeletalMesh, UPhysicsAsset* PhysicsAsset, const TSet<FName>& SelectedBones);
};
