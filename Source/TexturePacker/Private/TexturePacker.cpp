#include "CoreMinimal.h"

#include "AssetRegistryModule.h"					 // FAssetRegistryModule
#include "ContentBrowserModule.h"					 // FContentBrowserModule
#include "Engine/Texture.h"							 // UTexture
#include "Engine/Texture2D.h"						 // UTexture2D
#include "Framework/Application/SlateApplication.h"	 // FSlateApplication
#include "Framework/MultiBox/MultiBoxBuilder.h"		 // FMenuBuilder
#include "IContentBrowserSingleton.h"				 // CreateModalSaveAssetDialog
#include "ImageUtils.h"								 // FImageUtils
#include "Misc/MessageDialog.h"						 // FMessageDialog
#include "Modules/ModuleManager.h"					 // FModuleManager
#include "Widgets/DeclarativeSyntaxSupport.h"		 // SLATE_BEGIN_ARGS
#include "Widgets/Input/SButton.h"					 // SButton
#include "Widgets/Input/SCheckBox.h"				 // SCheckBox
#include "Widgets/Input/SComboBox.h"				 // SComboBox
#include "Widgets/Layout/SSeparator.h"				 // SSeparator
#include "Widgets/SBoxPanel.h"						 // SVerticalBox
#include "Widgets/SCompoundWidget.h"				 // SCompoundWidget
#include "Widgets/SWindow.h"						 // SWindow
#include "Widgets/Text/STextBlock.h"				 // STextBlock

#define LOCTEXT_NAMESPACE "TexturePacker"

// Copied from FImageUtils to fix alpha channel not being resized
void ImageResize(int32 SrcWidth,
				 int32 SrcHeight,
				 const TArrayView<const FColor>& SrcData,
				 int32 DstWidth,
				 int32 DstHeight,
				 const TArrayView<FColor>& DstData)
{
	check(SrcData.Num() >= SrcWidth * SrcHeight);
	check(DstData.Num() >= DstWidth * DstHeight);

	float SrcX = 0;
	float SrcY = 0;

	const float StepSizeX = SrcWidth / (float)DstWidth;
	const float StepSizeY = SrcHeight / (float)DstHeight;

	for(int32 Y=0; Y<DstHeight;Y++)
	{
		int32 PixelPos = Y * DstWidth;
		SrcX = 0.0f;	
	
		for(int32 X=0; X<DstWidth; X++)
		{
			int32 PixelCount = 0;
			float EndX = SrcX + StepSizeX;
			float EndY = SrcY + StepSizeY;
			
			// Generate a rectangular region of pixels and then find the average color of the region.
			int32 PosY = FMath::TruncToInt(SrcY+0.5f);
			PosY = FMath::Clamp<int32>(PosY, 0, (SrcHeight - 1));

			int32 PosX = FMath::TruncToInt(SrcX+0.5f);
			PosX = FMath::Clamp<int32>(PosX, 0, (SrcWidth - 1));

			int32 EndPosY = FMath::TruncToInt(EndY+0.5f);
			EndPosY = FMath::Clamp<int32>(EndPosY, 0, (SrcHeight - 1));

			int32 EndPosX = FMath::TruncToInt(EndX+0.5f);
			EndPosX = FMath::Clamp<int32>(EndPosX, 0, (SrcWidth - 1));

			FLinearColor LinearStepColor(0.0f, 0.0f, 0.0f, 0.0f);
			for (int32 PixelX = PosX; PixelX <= EndPosX; PixelX++)
			{
				for (int32 PixelY = PosY; PixelY <= EndPosY; PixelY++)
				{
					int32 StartPixel = PixelX + PixelY * SrcWidth;

					// Convert from gamma space to linear space before the addition.
					LinearStepColor += SrcData[StartPixel];
					PixelCount++;
				}
			}
			LinearStepColor /= (float) PixelCount;

			// Convert back from linear space to gamma space.
			FColor FinalColor = LinearStepColor.ToFColor(true);

			DstData[PixelPos] = FinalColor;

			SrcX = EndX;
			PixelPos++;
		}

		SrcY += StepSizeY;
	}
}

enum class EChannel
{
	B = 0,
	G = 1,
	R = 2,
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

struct FChannelOption
{
	FChannelOption(UTexture* InTexture, EChannel InChannel, bool bInInvert = false)
		: Texture(InTexture), Channel(InChannel), bInvert(bInInvert){};

	UTexture* Texture;
	EChannel Channel;
	bool bInvert;
};
using FChannelOptionsItem = TSharedPtr<FChannelOption>;
using FChannelOptions = TArray<FChannelOptionsItem>;

void PackTexture(const TCHAR* PackagePath,
				 const TCHAR* TextureName,
				 int32 InSizeX,
				 int32 InSizeY,
				 FChannelOption Red,
				 FChannelOption Green,
				 FChannelOption Blue,
				 TOptional<FChannelOption> Alpha)
{
	const FString PackageName = FString(PackagePath) + TextureName;
	UPackage* Package = CreatePackage(nullptr, *PackageName);
	Package->FullyLoad();

	UTexture2D* Texture = NewObject<UTexture2D>(Package, TextureName, RF_Public | RF_Standalone);
	Texture->Source.Init(InSizeX, InSizeY, 1, 1, TSF_BGRA8);
	Texture->CompressionSettings = TextureCompressionSettings::TC_Masks;
	Texture->CompressionNoAlpha = !Alpha.IsSet();

	const int32 BytesPerPixel = Texture->Source.GetBytesPerPixel();

	const int32 Size = InSizeX * InSizeY;

	struct FChannelOptionBytes
	{
		TArray64<uint8> Bytes;
		int32 BytesPerPixel;
		int32 ChannelOffset;
		bool bInvert;
	};
	auto ChannelOptionBytes = [Size, InSizeX, InSizeY](FChannelOption ChannelOption) -> FChannelOptionBytes {
		TArray64<uint8> Bytes;
		int32 BytesPerPixel = 1;
		if (ChannelOption.Texture != nullptr)
		{
			FTextureSource& Texture = ChannelOption.Texture->Source;
			Texture.GetMipData(Bytes, 0);
			BytesPerPixel = Texture.GetBytesPerPixel();

			if (Texture.GetSizeX() != InSizeY || Texture.GetSizeY() != InSizeY)
			{
				TArray64<uint8> Resized;
				Resized.AddUninitialized(Size * BytesPerPixel);
				const bool bLinearSpace = true;
				ImageResize(Texture.GetSizeX(),
							Texture.GetSizeY(),
							TArrayView<FColor>((FColor*) Bytes.GetData(), Bytes.Num() / 4),
							InSizeX,
							InSizeY,
							TArrayView<FColor>((FColor*) Resized.GetData(), Resized.Num() / 4));
				Bytes = MoveTemp(Resized);
			}
		}
		else
		{
			if (ChannelOption.Channel == EChannel::Black)
			{
				Bytes.AddZeroed(Size);
			}
			else
			{
				Bytes.AddUninitialized(Size);
				FMemory::Memset(Bytes.GetData(), MAX_uint8, Size);
			}
		}

		int32 ChannelOffset = ChannelOption.Channel < EChannel::White ? int32(ChannelOption.Channel) : 0;

		return {MoveTemp(Bytes), BytesPerPixel, ChannelOffset, ChannelOption.bInvert};
	};

	auto [RedBytes, RedBytesPerPixel, RedOffset, bRedInvert] = ChannelOptionBytes(Red);
	auto [GreenBytes, GreenBytesPerPixel, GreenOffset, bGreenInvert] = ChannelOptionBytes(Green);
	auto [BlueBytes, BlueBytesPerPixel, BlueOffset, bBlueInvert] = ChannelOptionBytes(Blue);
	auto [AlphaBytes, AlphaBytesPerPixel, AlphaOffset, bAlphaInvert] =
		ChannelOptionBytes(Alpha ? Alpha.GetValue() : FChannelOption{nullptr, EChannel::Black, false});

	uint8* Bytes = Texture->Source.LockMip(0);

	for (int32 PixelIdx = 0; PixelIdx < Size; ++PixelIdx)
	{
		uint8* Pixel = &Bytes[PixelIdx * BytesPerPixel];

		Pixel[0] = GreenBytes[PixelIdx * GreenBytesPerPixel + GreenOffset];
		Pixel[0] = bGreenInvert ? MAX_uint8 - Pixel[0] : Pixel[0];

		Pixel[1] = BlueBytes[PixelIdx * BlueBytesPerPixel + BlueOffset];
		Pixel[1] = bBlueInvert ? MAX_uint8 - Pixel[1] : Pixel[1];

		Pixel[2] = RedBytes[PixelIdx * RedBytesPerPixel + RedOffset];
		Pixel[2] = bRedInvert ? MAX_uint8 - Pixel[2] : Pixel[2];

		Pixel[3] = AlphaBytes[PixelIdx * AlphaBytesPerPixel + AlphaOffset];
		Pixel[3] = bAlphaInvert ? MAX_uint8 - Pixel[3] : Pixel[3];
	}

	Texture->Source.UnlockMip(0);
	Texture->UpdateResource();

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Texture);
	FString PackageFilename =
		FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(Package, Texture, RF_Public | RF_Standalone, *PackageFilename);
}

class SChannelComboBox final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SChannelComboBox)
	{
	}
	SLATE_ARGUMENT(FChannelOptions*, OptionsSource)
	SLATE_ARGUMENT(FChannelOptionsItem, InitialSelection)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Options = *InArgs._OptionsSource;
		Selected = InArgs._InitialSelection.IsValid() ? InArgs._InitialSelection : Options[0];

		// clang-format off
		ComboBox = 
		   SNew(SComboBox<FChannelOptionsItem>)
			.OptionsSource(&Options)
			.OnGenerateWidget(this, &SChannelComboBox::OnGenerateWidget)
			.OnSelectionChanged(this, &SChannelComboBox::OnSelectionChanged)
			.InitiallySelectedItem(Selected)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock).Text(this, &SChannelComboBox::GetCurrentLabel)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SSeparator)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock).Text(LOCTEXT("Invert", "Invert"))
						.Visibility(this, &SChannelComboBox::GetInvertCheckBoxVisibility)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.IsChecked(this, &SChannelComboBox::GetCurrentInverted)
						.Visibility(this, &SChannelComboBox::GetInvertCheckBoxVisibility)
						.OnCheckStateChanged(this, &SChannelComboBox::OnInvertCheckStateChangedateChanged)
					]
				]
			];

		ChildSlot
		[
			ComboBox.ToSharedRef()
		];
		// clang-format on
	}

	FChannelOptionsItem GetSelectedItem() const
	{
		return Selected;
	}

private:
	EVisibility GetInvertCheckBoxVisibility() const
	{
		if (Selected.IsValid() && Selected->Texture != nullptr)
		{
			return EVisibility::Visible;
		}
		return EVisibility::Collapsed;
	}

	void OnSelectionChanged(FChannelOptionsItem InSelection, ESelectInfo::Type /*SelectInfo*/)
	{
		Selected = InSelection;
	}

	void OnInvertCheckStateChangedateChanged(ECheckBoxState CheckState)
	{
		Selected->bInvert = CheckState == ECheckBoxState::Checked ? true : false;
	}

	ECheckBoxState GetCurrentInverted() const
	{
		return Selected->bInvert ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	FText GetCurrentLabel() const
	{
		return OptionLabel(Selected, false);
	}

	FText OptionLabel(FChannelOptionsItem Item, bool bCheckInvertedState = true) const
	{
		if (!Item.IsValid())
		{
			return LOCTEXT("SelectOverride", "Select channel override");
		}

		FText Label = [&]() {
			if (Item->Texture == nullptr)
			{
				return ChannelToText(Item->Channel);
			}
			else
			{
				return FText::Format(FTextFormat::FromString(TEXT("{0} {1}")),
									 FText::FromString(Item->Texture->GetName()),
									 ChannelToText(Item->Channel));
			}
		}();

		if (Item->bInvert && bCheckInvertedState)
		{
			Label = FText::Format(FTextFormat::FromString(TEXT("{0} Inverted")), Label);
		}

		return Label;
	}

	TSharedRef<SWidget> OnGenerateWidget(FChannelOptionsItem Item)
	{
		return SNew(STextBlock).Text(OptionLabel(Item));
	};

	FChannelOptionsItem Selected;
	FChannelOptions Options;
	TSharedPtr<SComboBox<FChannelOptionsItem>> ComboBox;
};

class STexturePacker final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STexturePacker)
	{
	}
	SLATE_END_ARGS()

	FChannelOptions ChannelOptions;

	void Construct(const FArguments& InArgs, TSharedRef<SWindow>& Window, TArray<UTexture*> InTextures)
	{
		Textures = MoveTemp(InTextures);

		ChannelOptions.Add(MakeShared<FChannelOption>(nullptr, EChannel::Black));
		ChannelOptions.Add(MakeShared<FChannelOption>(nullptr, EChannel::White));
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
					ChannelOptions.Add(MakeShared<FChannelOption>(Texture, EChannel::R));
					ChannelOptions.Add(MakeShared<FChannelOption>(Texture, EChannel::G));
					ChannelOptions.Add(MakeShared<FChannelOption>(Texture, EChannel::B));
					ChannelOptions.Add(MakeShared<FChannelOption>(Texture, EChannel::A));
					break;
				}
				default:
					ChannelOptions.Add(MakeShared<FChannelOption>(Texture, EChannel::R));
			}
		}

		auto UseAlphaCheckbox = SNew(SCheckBox).IsChecked(ECheckBoxState::Unchecked);

		auto RedChannel = SNew(SChannelComboBox).OptionsSource(&ChannelOptions);
		auto GreenChannel = SNew(SChannelComboBox).OptionsSource(&ChannelOptions);
		auto BlueChannel = SNew(SChannelComboBox).OptionsSource(&ChannelOptions);
		auto AlphaChannel =
			SNew(SChannelComboBox)
				.OptionsSource(&ChannelOptions)
				.InitialSelection(ChannelOptions[1])
				.Visibility_Lambda([UseAlphaCheckbox]() {
					return UseAlphaCheckbox->IsChecked() ? EVisibility::Visible : EVisibility::Collapsed;
				});

		// clang-format off
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

					PathPart += TEXT("/");

					int32 MinX = MAX_int32;
					int32 MinY = MAX_int32;

					auto FindMin = [&MinX, &MinY](const TSharedRef<SChannelComboBox>& Combo) {
						if (UTexture* Texture = Combo->GetSelectedItem().Get()->Texture)
						{
							MinX = FMath::Min(MinX, Texture->Source.GetSizeX());
							MinY = FMath::Min(MinY, Texture->Source.GetSizeY());
						}
					};

					FindMin(RedChannel);
					FindMin(GreenChannel);
					FindMin(BlueChannel);
					if (UseAlphaCheckbox->IsChecked())
					{
						FindMin(AlphaChannel);
					}

					if (!ensure(MinX != MAX_int32 && MinY != MAX_int32))
					{
						return FReply::Handled();
					}

					PackTexture(*PathPart,
								*FilenamePart,
								MinX,
								MinY,
								*RedChannel->GetSelectedItem().Get(),
								*BlueChannel->GetSelectedItem().Get(),
								*GreenChannel->GetSelectedItem().Get(),
								UseAlphaCheckbox->IsChecked()
									? TOptional<FChannelOption>(*AlphaChannel->GetSelectedItem().Get())
									: TOptional<FChannelOption>());
					Window->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
		];
		// clang-format on
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
						TSharedRef<SWindow> PackerWindow = SNew(SWindow)
															   .Title(LOCTEXT("PackerWindow", "Texture Packer"))
															   .SizingRule(ESizingRule::Autosized);

						PackerWindow->SetContent(SNew(STexturePacker, PackerWindow, MoveTemp(Textures)));

						FSlateApplication::Get().AddWindow(PackerWindow);
					};

					FUIAction Action{FExecuteAction::CreateLambda(ShowPackerWindow)};

					MenuBuilder.AddMenuEntry(LOCTEXT("TexturePackerEntry", "Pack Textures"),
											 LOCTEXT("TexturePackerEntryTooltip", "Channel pack selected textures"),
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
