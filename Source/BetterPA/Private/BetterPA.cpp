#include "BetterPA.h"
#include "BetterPAGenerator.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "AssetToolsModule.h"
#include "Factories/PhysicsAssetFactory.h"
#include "SBetterPABonePicker.h"
#include "SBetterPAConstraintGraph.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FBetterPAModule"

void FBetterPAModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// Register context menu extension
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedAssets>& MenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	
	MenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FBetterPAModule::OnExtendContentBrowserAssetSelectionMenu));
	MenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FBetterPAModule::OnExtendContentBrowserPhysicsAssetSelectionMenu));
}

void FBetterPAModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

TSharedRef<FExtender> FBetterPAModule::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	if (SelectedAssets.Num() == 1 && SelectedAssets[0].GetClass() == USkeletalMesh::StaticClass())
	{
		Extender->AddMenuExtension(
			"GetAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateRaw(this, &FBetterPAModule::AddMenuEntry, SelectedAssets[0])
		);
	}

	return Extender;
}

TSharedRef<FExtender> FBetterPAModule::OnExtendContentBrowserPhysicsAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	if (SelectedAssets.Num() == 1 && SelectedAssets[0].GetClass() == UPhysicsAsset::StaticClass())
	{
		Extender->AddMenuExtension(
			"GetAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateRaw(this, &FBetterPAModule::AddPhysicsAssetMenuEntry, SelectedAssets[0])
		);
	}

	return Extender;
}

void FBetterPAModule::AddMenuEntry(FMenuBuilder& MenuBuilder, FAssetData SelectedAsset)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("GenerateBetterPA", "Generate Better Physics Asset"),
		LOCTEXT("GenerateBetterPATooltip", "Generates a physics asset with better capsule placement."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FBetterPAModule::OnGenerateBetterPA, SelectedAsset))
	);
}

void FBetterPAModule::AddPhysicsAssetMenuEntry(FMenuBuilder& MenuBuilder, FAssetData SelectedAsset)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenConstraintGraph", "Open Constraint Graph"),
		LOCTEXT("OpenConstraintGraphTooltip", "Opens a graph editor to manage constraints."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FBetterPAModule::OnOpenConstraintGraph, SelectedAsset))
	);
}

void FBetterPAModule::OnGenerateBetterPA(FAssetData SelectedAsset)
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(SelectedAsset.GetAsset());
	if (!SkeletalMesh)
	{
		return;
	}

	TSharedPtr<SWindow> PickerWindow;
	TSharedPtr<SBetterPABonePicker> BonePicker;

	PickerWindow = SNew(SWindow)
		.Title(LOCTEXT("SelectBones", "Select Bones for Physics Asset"))
		.ClientSize(FVector2D(400, 600))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	BonePicker = SNew(SBetterPABonePicker)
		.SkeletalMesh(SkeletalMesh);

	PickerWindow->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			BonePicker.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(10)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Generate", "Generate"))
				.OnClicked_Lambda([this, SelectedAsset, SkeletalMesh, BonePicker, PickerWindow]()
				{
					TSet<FName> SelectedBones = BonePicker->GetSelectedBones();
					GeneratePhysicsAsset(SelectedAsset, SkeletalMesh, SelectedBones);
					PickerWindow->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(10, 0, 0, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.OnClicked_Lambda([PickerWindow]()
				{
					PickerWindow->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
		]
	);

	FSlateApplication::Get().AddWindow(PickerWindow.ToSharedRef());
}

void FBetterPAModule::GeneratePhysicsAsset(FAssetData SelectedAsset, USkeletalMesh* SkeletalMesh, const TSet<FName>& SelectedBones)
{
	FString PackageName = SelectedAsset.PackageName.ToString() + "_PhysicsAsset";
	FString AssetName = SelectedAsset.AssetName.ToString() + "_PhysicsAsset";

	// Create Physics Asset
	IAssetTools& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UPhysicsAssetFactory* Factory = NewObject<UPhysicsAssetFactory>();
	Factory->TargetSkeletalMesh = SkeletalMesh;
	
	UObject* NewAsset = AssetTools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), UPhysicsAsset::StaticClass(), Factory);
	UPhysicsAsset* PhysicsAsset = Cast<UPhysicsAsset>(NewAsset);

	if (PhysicsAsset)
	{
		if (!PhysicsAsset->PreviewSkeletalMesh.Get())
		{
			PhysicsAsset->PreviewSkeletalMesh = SkeletalMesh;
		}
		
		FBetterPAGenerator::GeneratePhysicsAsset(SkeletalMesh, PhysicsAsset, SelectedBones);
	}
}

void FBetterPAModule::OnOpenConstraintGraph(FAssetData SelectedAsset)
{
	UPhysicsAsset* PhysicsAsset = Cast<UPhysicsAsset>(SelectedAsset.GetAsset());
	if (!PhysicsAsset)
	{
		return;
	}

	TSharedPtr<SWindow> GraphWindow = SNew(SWindow)
		.Title(LOCTEXT("ConstraintGraph", "Constraint Graph Editor"))
		.ClientSize(FVector2D(800, 600));

	GraphWindow->SetContent(
		SNew(SBetterPAConstraintGraph)
		.PhysicsAsset(PhysicsAsset)
	);

	FSlateApplication::Get().AddWindow(GraphWindow.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBetterPAModule, BetterPA)