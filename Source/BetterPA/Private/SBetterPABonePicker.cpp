#include "SBetterPABonePicker.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ReferenceSkeleton.h"

void SBetterPABonePicker::Construct(const FArguments& InArgs)
{
	SkeletalMesh = InArgs._SkeletalMesh;

	if (SkeletalMesh)
	{
		const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
		const TArray<FMeshBoneInfo>& BoneInfo = RefSkeleton.GetRefBoneInfo();

		TArray<TSharedPtr<FBetterPABoneItem>> AllItems;
		AllItems.SetNum(BoneInfo.Num());

		// Create all items first
		for (int32 i = 0; i < BoneInfo.Num(); ++i)
		{
			AllItems[i] = MakeShared<FBetterPABoneItem>(BoneInfo[i].Name, i);
		}

		// Build hierarchy
		for (int32 i = 0; i < BoneInfo.Num(); ++i)
		{
			int32 ParentIndex = BoneInfo[i].ParentIndex;
			if (ParentIndex != INDEX_NONE)
			{
				AllItems[ParentIndex]->Children.Add(AllItems[i]);
				AllItems[i]->Parent = AllItems[ParentIndex];
			}
			else
			{
				RootItems.Add(AllItems[i]);
			}
		}
	}

	ChildSlot
	[
		SAssignNew(TreeView, STreeView<TSharedPtr<FBetterPABoneItem>>)
		.TreeItemsSource(&RootItems)
		.OnGenerateRow(this, &SBetterPABonePicker::OnGenerateRow)
		.OnGetChildren(this, &SBetterPABonePicker::OnGetChildren)
		.SelectionMode(ESelectionMode::None)
	];
	
	if (TreeView.IsValid())
	{
		for (const auto& RootItem : RootItems)
		{
			TreeView->SetItemExpansion(RootItem, true);
		}
	}
}

TSharedRef<ITableRow> SBetterPABonePicker::OnGenerateRow(TSharedPtr<FBetterPABoneItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FBetterPABoneItem>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.IsChecked(this, &SBetterPABonePicker::GetCheckState, Item)
			.OnCheckStateChanged(this, &SBetterPABonePicker::OnCheckStateChanged, Item)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromName(Item->BoneName))
		]
	];
}

void SBetterPABonePicker::OnGetChildren(TSharedPtr<FBetterPABoneItem> Item, TArray<TSharedPtr<FBetterPABoneItem>>& OutChildren)
{
	OutChildren = Item->Children;
}

void SBetterPABonePicker::OnCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FBetterPABoneItem> Item)
{
	bool bIsSelected = (NewState == ECheckBoxState::Checked);
	RecursivelySetSelection(Item, bIsSelected);
}

ECheckBoxState SBetterPABonePicker::GetCheckState(TSharedPtr<FBetterPABoneItem> Item) const
{
	return Item->bIsSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SBetterPABonePicker::RecursivelySetSelection(TSharedPtr<FBetterPABoneItem> Item, bool bSelected)
{
	Item->bIsSelected = bSelected;
	for (auto& Child : Item->Children)
	{
		RecursivelySetSelection(Child, bSelected);
	}
}

TSet<FName> SBetterPABonePicker::GetSelectedBones() const
{
	TSet<FName> SelectedBones;
	for (const auto& RootItem : RootItems)
	{
		CollectSelectedBones(RootItem, SelectedBones);
	}
	return SelectedBones;
}

void SBetterPABonePicker::CollectSelectedBones(TSharedPtr<FBetterPABoneItem> Item, TSet<FName>& OutSelectedBones) const
{
	if (Item->bIsSelected)
	{
		OutSelectedBones.Add(Item->BoneName);
	}
	for (const auto& Child : Item->Children)
	{
		CollectSelectedBones(Child, OutSelectedBones);
	}
}
