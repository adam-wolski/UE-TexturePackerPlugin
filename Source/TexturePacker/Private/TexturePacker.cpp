#include "CoreMinimal.h"

#include "AssetRegistryModule.h"					 // FAssetRegistryModule
#include "ContentBrowserModule.h"					 // FContentBrowserModule
#include "Engine/Texture.h"							 // UTexture
#include "Engine/Texture2D.h"						 // UTexture2D
#include "Framework/Application/SlateApplication.h"	 // FSlateApplication
#include "Framework/MultiBox/MultiBoxBuilder.h"		 // FMenuBuilder
#include "IContentBrowserSingleton.h"				 // CreateModalSaveAssetDialog
#include "Misc/MessageDialog.h"						 // FMessageDialog
#include "Modules/ModuleManager.h"					 // FModuleManager
#include "Widgets/DeclarativeSyntaxSupport.h"		 // SLATE_BEGIN_ARGS
#include "Widgets/Input/SButton.h"					 // SButton
#include "Widgets/Input/SCheckBox.h"				 // SCheckBox
#include "Widgets/Input/SComboBox.h"				 // SComboBox
#include "Widgets/SBoxPanel.h"						 // SVerticalBox
#include "Widgets/SCompoundWidget.h"				 // SCompoundWidget
#include "Widgets/SWindow.h"						 // SWindow
#include "Widgets/Text/STextBlock.h"				 // STextBlock

#define LOCTEXT_NAMESPACE "TexturePacker"

enum class EChannel
{
	R = 0,
	G = 1,
	B = 2,
	A = 3,
	White,
	Black
};

FText ChannelToText(EChannel Channel)
{
	switch (Channel)
	{
		case EChannel::R:
			return LOCTEXT("RedChannel", "Red Channel");
		case EChannel::G:
			return LOCTEXT("GreenChannel", "Green Channel");
		case EChannel::B:
			return LOCTEXT("BlueChannel", "Blue Channel");
		case EChannel::A:
			return LOCTEXT("AlphaChannel", "Alpha Channel");
		case EChannel::White:
			return LOCTEXT("FillWhite", "Fill With White");
		case EChannel::Black:
			return LOCTEXT("FillBlack", "Fill With Black");
	}
	return LOCTEXT("Error", "Error");
};

using FChannelOptionsItemInner = TPair<UTexture*, EChannel>;
using FChannelOptionsItem = TSharedPtr<FChannelOptionsItemInner>;
using FChannelOptions = TArray<FChannelOptionsItem>;

void PackTexture(const TCHAR* PackagePath,
				 const TCHAR* TextureName,
				 int32 InSizeX,
				 int32 InSizeY,
				 FChannelOptionsItemInner Red,
				 FChannelOptionsItemInner Green,
				 FChannelOptionsItemInner Blue,
				 TOptional<FChannelOptionsItemInner> Alpha)
{
	const FString PackageName = FString(PackagePath) + TextureName;
	UPackage* Package = CreatePackage(nullptr, *PackageName);

	UTexture2D* Texture = NewObject<UTexture2D>(Package, TextureName, RF_Public | RF_Standalone);
	Texture->Source.Init(InSizeX, InSizeY, 1, 1, TSF_BGRA8);

	const int32 BytesPerPixel = Texture->Source.GetBytesPerPixel();

	const int32 Size = InSizeX * InSizeY;

	struct FChannelOptionBytes
	{
		TArray64<uint8> Bytes;
		int32 BytesPerPixel;
		int32 ChannelOffset;
	};
	auto ChannelOptionBytes = [Size](FChannelOptionsItemInner ChannelOption) -> FChannelOptionBytes {
		TArray64<uint8> Bytes;
		int32 BytesPerPixel = 1;
		if (ChannelOption.Key != nullptr)
		{
			ChannelOption.Key->Source.GetMipData(Bytes, 0);
			BytesPerPixel = ChannelOption.Key->Source.GetBytesPerPixel();
		}
		else
		{
			if (ChannelOption.Value == EChannel::Black)
			{
				Bytes.AddZeroed(Size);
			}
			else
			{
				Bytes.AddUninitialized(Size);
				FMemory::Memset(Bytes.GetData(), MAX_uint8, Size);
			}
		}

		int32 ChannelOffset = ChannelOption.Value < EChannel::White ? int32(ChannelOption.Value) : 0;

		return {MoveTemp(Bytes), BytesPerPixel, ChannelOffset};
	};

	auto [RedBytes, RedBytesPerPixel, RedOffset] = ChannelOptionBytes(Red);
	auto [GreenBytes, GreenBytesPerPixel, GreenOffset] = ChannelOptionBytes(Green);
	auto [BlueBytes, BlueBytesPerPixel, BlueOffset] = ChannelOptionBytes(Blue);
	auto [AlphaBytes, AlphaBytesPerPixel, AlphaOffset] =
		ChannelOptionBytes(Alpha ? Alpha.GetValue() : FChannelOptionsItemInner(nullptr, EChannel::Black));

	uint8* Bytes = Texture->Source.LockMip(0);

	for (int32 PixelIdx = 0; PixelIdx < Size; ++PixelIdx)
	{
		uint8* Pixel = &Bytes[PixelIdx * BytesPerPixel];

		Pixel[0] = BlueBytes[PixelIdx * BlueBytesPerPixel + BlueOffset];
		Pixel[1] = GreenBytes[PixelIdx * GreenBytesPerPixel + GreenOffset];
		Pixel[2] = RedBytes[PixelIdx * RedBytesPerPixel + RedOffset];
		Pixel[3] = AlphaBytes[PixelIdx * AlphaBytesPerPixel + AlphaOffset];
	}

	Texture->Source.UnlockMip(0);
	Texture->UpdateResource();

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Texture);
	FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(Package, Texture, RF_Public | RF_Standalone, *PackageFilename);
}

class SChannelComboBox final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SChannelComboBox) {}
		SLATE_ARGUMENT(FChannelOptions*, OptionsSource)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Options = *InArgs._OptionsSource;

		ComboBox = 
		   SNew(SComboBox<FChannelOptionsItem>)
			.OptionsSource(&Options)
			.OnGenerateWidget(this, &SChannelComboBox::OnGenerateWidget)
			.OnSelectionChanged(this, &SChannelComboBox::OnSelectionChanged)
			.InitiallySelectedItem(Options[0])
			[
				SNew(STextBlock).Text(this, &SChannelComboBox::GetCurrentLabel)
			];

		ChildSlot
		[
			ComboBox.ToSharedRef()
		];
	}

	FChannelOptionsItem GetSelectedItem() const
	{
		return Selected;
	}
	
	void OnSelectionChanged(FChannelOptionsItem InSelection, ESelectInfo::Type /*SelectInfo*/)
	{
		Selected = InSelection;
	}

	FText GetCurrentLabel() const
	{
		return OptionLabel(Selected);
	}

	FText OptionLabel(FChannelOptionsItem Item) const
	{
		if (!Item.IsValid())
		{
			return LOCTEXT("SelectOverride", "Select channel override");
		}
		if (Item->Key == nullptr)
		{
			return ChannelToText(Item->Value);
		}
		else
		{
			return FText::Format(FTextFormat::FromString(TEXT("{0} {1}")),
								 FText::FromString(Item->Key->GetName()),
								 ChannelToText(Item->Value));
		}
	}

	TSharedRef<SWidget> OnGenerateWidget(FChannelOptionsItem Item)
	{
		return SNew(STextBlock).Text(OptionLabel(Item));
	};


private:
	FChannelOptionsItem Selected;
	FChannelOptions Options;
	TSharedPtr<SComboBox<FChannelOptionsItem>> ComboBox;
};

class STexturePacker final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STexturePacker) {}
	SLATE_END_ARGS()

	FChannelOptions ChannelOptions;

	void Construct(const FArguments& InArgs, TSharedRef<SWindow>& Window, TArray<UTexture*> InTextures)
	{
		Textures = MoveTemp(InTextures);
		
		ChannelOptions.Add(MakeShared<FChannelOptionsItemInner>(nullptr, EChannel::Black));
		ChannelOptions.Add(MakeShared<FChannelOptionsItemInner>(nullptr, EChannel::White));
		for (UTexture* Texture : Textures)
		{
			const ETextureSourceFormat Format = Texture->Source.GetFormat();
			switch (Format)
			{
				case ETextureSourceFormat::TSF_BGRA8:
				case ETextureSourceFormat::TSF_BGRE8:
				case ETextureSourceFormat::TSF_RGBA16:
				case ETextureSourceFormat::TSF_RGBA16F:
				case ETextureSourceFormat::TSF_RGBA8:
				case ETextureSourceFormat::TSF_RGBE8:
				{
					ChannelOptions.Add(MakeShared<FChannelOptionsItemInner>(Texture, EChannel::R));
					ChannelOptions.Add(MakeShared<FChannelOptionsItemInner>(Texture, EChannel::G));
					ChannelOptions.Add(MakeShared<FChannelOptionsItemInner>(Texture, EChannel::B));
					ChannelOptions.Add(MakeShared<FChannelOptionsItemInner>(Texture, EChannel::A));
					break;
				}
				default:
					ChannelOptions.Add(MakeShared<FChannelOptionsItemInner>(Texture, EChannel::R));
			}
		}

		auto UseAlphaCheckbox = SNew(SCheckBox).IsChecked(ECheckBoxState::Unchecked);

		auto RedChannel = SNew(SChannelComboBox).OptionsSource(&ChannelOptions);
		auto GreenChannel = SNew(SChannelComboBox).OptionsSource(&ChannelOptions);
		auto BlueChannel =  SNew(SChannelComboBox).OptionsSource(&ChannelOptions);
		auto AlphaChannel =
			SNew(SChannelComboBox).OptionsSource(&ChannelOptions).Visibility_Lambda([UseAlphaCheckbox]() {
				return UseAlphaCheckbox->IsChecked() ? EVisibility::Visible : EVisibility::Collapsed;
			});

		ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock).Text(LOCTEXT("EnableAlpha", "Pack Alpha"))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					UseAlphaCheckbox
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				RedChannel
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				GreenChannel
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				BlueChannel
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				AlphaChannel
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(LOCTEXT("Pack", "Pack"))
				.OnClicked_Lambda([this, RedChannel, GreenChannel, BlueChannel, AlphaChannel, Window, UseAlphaCheckbox](){
					const FString Path = 
						FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser")
							.Get().CreateModalSaveAssetDialog({});	

					FString PathPart, FilenamePart, ExtensionPart;
					FPaths::Split(Path, PathPart, FilenamePart, ExtensionPart);

					PackTexture(*PathPart,
								*FilenamePart,
								Textures[0]->Source.GetSizeX(),
								Textures[0]->Source.GetSizeY(),
								*RedChannel->GetSelectedItem().Get(),
								*BlueChannel->GetSelectedItem().Get(),
								*GreenChannel->GetSelectedItem().Get(),
								UseAlphaCheckbox->IsChecked()
									? TOptional<FChannelOptionsItemInner>(*AlphaChannel->GetSelectedItem().Get())
									: TOptional<FChannelOptionsItemInner>());
					Window->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
		];
	}

private:
	TArray<UTexture*> Textures;
};

class FTexturePackerModule final : public IModuleInterface
{
	void StartupModule() override
	{
		FContentBrowserModule& ContentBrowserModule =
			FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		TArray<FContentBrowserMenuExtender_SelectedAssets>& MenuExtenders =
			ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

		MenuExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda(
			[](const TArray<FAssetData>& SelectedAssets) -> TSharedRef<FExtender> {
				TSharedRef<FExtender> Extender = MakeShared<FExtender>();

				auto MenuExtension = [&SelectedAssets](FMenuBuilder& MenuBuilder) {
					TArray<UTexture*> Textures = [&]() {
						TArray<UTexture*> Textures;

						for (const FAssetData& AssetData : SelectedAssets)	
						{
							if (UTexture* Texture = Cast<UTexture>(AssetData.GetAsset()))
							{
								Textures.Add(Texture);
							}
						}

						return Textures;
					}();

					if (!Textures.Num())
					{
						return;
					}

					auto ShowPackerWindow = [&SelectedAssets, Textures = MoveTemp(Textures)]() mutable {
						const int32 SizeX = Textures[0]->Source.GetSizeX();
						const int32 SizeY = Textures[0]->Source.GetSizeY();
						for (UTexture* Texture : Textures)
						{
							if (Texture->Source.GetSizeX() != SizeX || Texture->Source.GetSizeY() != SizeY)
							{
								FMessageDialog::Open(EAppMsgType::Ok,
													 LOCTEXT("ErrorSizeDialog",
															 "All textures need to be same size for texture packing."));
								return;
							}
						}

						TSharedRef<SWindow> PackerWindow = SNew(SWindow)
															   .Title(LOCTEXT("PackerWindow", "Texture Packer"))
															   .SizingRule(ESizingRule::Autosized);

						PackerWindow->SetContent(SNew(STexturePacker, PackerWindow, MoveTemp(Textures)));

						FSlateApplication::Get().AddWindow(PackerWindow);
					};

					FUIAction Action{FExecuteAction::CreateLambda(ShowPackerWindow)};

					MenuBuilder.AddMenuEntry(LOCTEXT("TexturePackerEntry", "Pack Textures"),
											 LOCTEXT("TexturePackerEntryTooltip",
													 "Channel pack selected textures"),
											 FSlateIcon(),
											 Action);
				};

				Extender->AddMenuExtension("CommonAssetActions",
										   EExtensionHook::After,
										   nullptr,
										   FMenuExtensionDelegate::CreateLambda(MenuExtension));

				return Extender;
			}));
	}

	FContentBrowserMenuExtender_SelectedAssets MenuExtenderHandle;
};

IMPLEMENT_MODULE(FTexturePackerModule, TexturePacker);

#undef LOCTEXT_NAMESPACE
