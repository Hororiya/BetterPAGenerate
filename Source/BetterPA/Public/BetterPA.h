// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "ContentBrowserDelegates.h"

class USkeletalMesh;
class UPhysicsAsset;

class FBetterPAModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	void AddMenuEntry(FMenuBuilder& MenuBuilder, FAssetData SelectedAsset);
	void OnGenerateBetterPA(FAssetData SelectedAsset);
	void GeneratePhysicsAsset(FAssetData SelectedAsset, USkeletalMesh* SkeletalMesh, const TSet<FName>& SelectedBones);
	
	// New Menu Entry for Physics Asset
	TSharedRef<FExtender> OnExtendContentBrowserPhysicsAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	void AddPhysicsAssetMenuEntry(FMenuBuilder& MenuBuilder, FAssetData SelectedAsset);
	void OnOpenConstraintGraph(FAssetData SelectedAsset);
};
