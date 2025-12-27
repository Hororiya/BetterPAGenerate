#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Engine/SkeletalMesh.h"

struct FBetterPABoneItem
{
	FName BoneName;
	int32 BoneIndex;
	TArray<TSharedPtr<FBetterPABoneItem>> Children;
	TWeakPtr<FBetterPABoneItem> Parent;
	bool bIsSelected;

	FBetterPABoneItem(FName InBoneName, int32 InBoneIndex)
		: BoneName(InBoneName)
		, BoneIndex(InBoneIndex)
		, bIsSelected(true)
	{}
};

class BETTERPA_API SBetterPABonePicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBetterPABonePicker) {}
		SLATE_ARGUMENT(USkeletalMesh*, SkeletalMesh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Returns the set of selected bone names
	TSet<FName> GetSelectedBones() const;

private:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FBetterPABoneItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildren(TSharedPtr<FBetterPABoneItem> Item, TArray<TSharedPtr<FBetterPABoneItem>>& OutChildren);
	void OnCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FBetterPABoneItem> Item);
	ECheckBoxState GetCheckState(TSharedPtr<FBetterPABoneItem> Item) const;
	
	void RecursivelySetSelection(TSharedPtr<FBetterPABoneItem> Item, bool bSelected);
	void CollectSelectedBones(TSharedPtr<FBetterPABoneItem> Item, TSet<FName>& OutSelectedBones) const;

	USkeletalMesh* SkeletalMesh;
	TArray<TSharedPtr<FBetterPABoneItem>> RootItems;
	TSharedPtr<STreeView<TSharedPtr<FBetterPABoneItem>>> TreeView;
};
