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

// Copied form Math\Color.cpp. Wasn't in exported symbols
float sRGBToLinearTable[256] =
{
	0.0f,
	0.000303526983548838f, 0.000607053967097675f, 0.000910580950646512f, 0.00121410793419535f, 0.00151763491774419f,
	0.00182116190129302f, 0.00212468888484186f, 0.0024282158683907f, 0.00273174285193954f, 0.00303526983548838f,
	0.00334653564113713f, 0.00367650719436314f, 0.00402471688178252f, 0.00439144189356217f, 0.00477695332960869f,
	0.005181516543916f, 0.00560539145834456f, 0.00604883284946662f, 0.00651209061157708f, 0.00699540999852809f,
	0.00749903184667767f, 0.00802319278093555f, 0.0085681254056307f, 0.00913405848170623f, 0.00972121709156193f,
	0.0103298227927056f, 0.0109600937612386f, 0.0116122449260844f, 0.012286488094766f, 0.0129830320714536f,
	0.0137020827679224f, 0.0144438433080002f, 0.0152085141260192f, 0.0159962930597398f, 0.0168073754381669f,
	0.0176419541646397f, 0.0185002197955389f, 0.0193823606149269f, 0.0202885627054049f, 0.0212190100154473f,
	0.0221738844234532f, 0.02315336579873f, 0.0241576320596103f, 0.0251868592288862f, 0.0262412214867272f,
	0.0273208912212394f, 0.0284260390768075f, 0.0295568340003534f, 0.0307134432856324f, 0.0318960326156814f,
	0.0331047661035236f, 0.0343398063312275f, 0.0356013143874111f, 0.0368894499032755f, 0.0382043710872463f,
	0.0395462347582974f, 0.0409151963780232f, 0.0423114100815264f, 0.0437350287071788f, 0.0451862038253117f,
	0.0466650857658898f, 0.0481718236452158f, 0.049706565391714f, 0.0512694577708345f, 0.0528606464091205f,
	0.0544802758174765f, 0.0561284894136735f, 0.0578054295441256f, 0.0595112375049707f, 0.0612460535624849f,
	0.0630100169728596f, 0.0648032660013696f, 0.0666259379409563f, 0.0684781691302512f, 0.070360094971063f,
	0.0722718499453493f, 0.0742135676316953f, 0.0761853807213167f, 0.0781874210336082f, 0.0802198195312533f,
	0.0822827063349132f, 0.0843762107375113f, 0.0865004612181274f, 0.0886555854555171f, 0.0908417103412699f,
	0.0930589619926197f, 0.0953074657649191f, 0.0975873462637915f, 0.0998987273569704f, 0.102241732185838f,
	0.104616483176675f, 0.107023102051626f, 0.109461709839399f, 0.1119324268857f, 0.114435372863418f,
	0.116970666782559f, 0.119538426999953f, 0.122138771228724f, 0.124771816547542f, 0.127437679409664f,
	0.130136475651761f, 0.132868320502552f, 0.135633328591233f, 0.138431613955729f, 0.141263290050755f,
	0.144128469755705f, 0.147027265382362f, 0.149959788682454f, 0.152926150855031f, 0.155926462553701f,
	0.158960833893705f, 0.162029374458845f, 0.16513219330827f, 0.168269398983119f, 0.171441099513036f,
	0.174647402422543f, 0.17788841473729f, 0.181164242990184f, 0.184474993227387f, 0.187820771014205f,
	0.191201681440861f, 0.194617829128147f, 0.198069318232982f, 0.201556252453853f, 0.205078735036156f,
	0.208636868777438f, 0.212230756032542f, 0.215860498718652f, 0.219526198320249f, 0.223227955893977f,
	0.226965872073417f, 0.23074004707378f, 0.23455058069651f, 0.238397572333811f, 0.242281120973093f,
	0.246201325201334f, 0.250158283209375f, 0.254152092796134f, 0.258182851372752f, 0.262250655966664f,
	0.266355603225604f, 0.270497789421545f, 0.274677310454565f, 0.278894261856656f, 0.283148738795466f,
	0.287440836077983f, 0.291770648154158f, 0.296138269120463f, 0.300543792723403f, 0.304987312362961f,
	0.309468921095997f, 0.313988711639584f, 0.3185467763743f, 0.323143207347467f, 0.32777809627633f,
	0.332451534551205f, 0.337163613238559f, 0.341914423084057f, 0.346704054515559f, 0.351532597646068f,
	0.356400142276637f, 0.361306777899234f, 0.36625259369956f, 0.371237678559833f, 0.376262121061519f,
	0.381326009488037f, 0.386429431827418f, 0.39157247577492f, 0.396755228735618f, 0.401977777826949f,
	0.407240209881218f, 0.41254261144808f, 0.417885068796976f, 0.423267667919539f, 0.428690494531971f,
	0.434153634077377f, 0.439657171728079f, 0.445201192387887f, 0.450785780694349f, 0.456411021020965f,
	0.462076997479369f, 0.467783793921492f, 0.473531493941681f, 0.479320180878805f, 0.485149937818323f,
	0.491020847594331f, 0.496932992791578f, 0.502886455747457f, 0.50888131855397f, 0.514917663059676f,
	0.520995570871595f, 0.527115123357109f, 0.533276401645826f, 0.539479486631421f, 0.545724458973463f,
	0.552011399099209f, 0.558340387205378f, 0.56471150325991f, 0.571124827003694f, 0.577580437952282f,
	0.584078415397575f, 0.590618838409497f, 0.597201785837643f, 0.603827336312907f, 0.610495568249093f,
	0.617206559844509f, 0.623960389083534f, 0.630757133738175f, 0.637596871369601f, 0.644479679329661f,
	0.651405634762384f, 0.658374814605461f, 0.665387295591707f, 0.672443154250516f, 0.679542466909286f,
	0.686685309694841f, 0.693871758534824f, 0.701101889159085f, 0.708375777101046f, 0.71569349769906f,
	0.723055126097739f, 0.730460737249286f, 0.737910405914797f, 0.745404206665559f, 0.752942213884326f,
	0.760524501766589f, 0.768151144321824f, 0.775822215374732f, 0.783537788566466f, 0.791297937355839f,
	0.799102735020525f, 0.806952254658248f, 0.81484656918795f, 0.822785751350956f, 0.830769873712124f,
	0.838799008660978f, 0.846873228412837f, 0.854992605009927f, 0.863157210322481f, 0.871367116049835f,
	0.879622393721502f, 0.887923114698241f, 0.896269350173118f, 0.904661171172551f, 0.913098648557343f,
	0.921581853023715f, 0.930110855104312f, 0.938685725169219f, 0.947306533426946f, 0.955973349925421f,
	0.964686244552961f, 0.973445287039244f, 0.982250546956257f, 0.991102093719252f, 1.0f
};

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
	Texture->SRGB = false;

	const int32 BytesPerPixel = Texture->Source.GetBytesPerPixel();

	const int32 Size = InSizeX * InSizeY;

	struct FChannelOptionBytes
	{
		TArray64<uint8> Bytes;
		int32 BytesPerPixel;
		int32 ChannelOffset;
		bool bInvert;
		bool bSRGB;
	};
	auto ChannelOptionBytes = [Size, InSizeX, InSizeY](FChannelOption ChannelOption) -> FChannelOptionBytes {
		TArray64<uint8> Bytes;
		int32 BytesPerPixel = 1;
		bool bSRGB = false;
		if (ChannelOption.Texture != nullptr)
		{
			FTextureSource& Texture = ChannelOption.Texture->Source;
			bSRGB = ChannelOption.Texture->SRGB && ChannelOption.Channel != EChannel::A;
			Texture.GetMipData(Bytes, 0);
			BytesPerPixel = Texture.GetBytesPerPixel();

			if (Texture.GetSizeX() != InSizeY || Texture.GetSizeY() != InSizeY)
			{
				TArray64<uint8> Resized;
				Resized.AddUninitialized(Size * BytesPerPixel);
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

		return {MoveTemp(Bytes), BytesPerPixel, ChannelOffset, ChannelOption.bInvert, bSRGB};
	};

	const FChannelOptionBytes RedBytes = ChannelOptionBytes(Red);
	const FChannelOptionBytes GreenBytes = ChannelOptionBytes(Green);
	const FChannelOptionBytes BlueBytes = ChannelOptionBytes(Blue);
	const FChannelOptionBytes AlphaBytes =
		ChannelOptionBytes(Alpha ? Alpha.GetValue() : FChannelOption{nullptr, EChannel::Black, false});

	uint8* Bytes = Texture->Source.LockMip(0);

	auto GetByte = [](const int32 PixelIdx, const FChannelOptionBytes& Channel) -> uint8 {
		const uint8 B = Channel.Bytes[PixelIdx * Channel.BytesPerPixel + Channel.ChannelOffset];
		if (Channel.bSRGB)
		{
			float BLin = sRGBToLinearTable[B];
			BLin = Channel.bInvert ? 1.f - BLin : BLin;

			return (uint8)FMath::FloorToInt(BLin * 255.999f);
		}
		return Channel.bInvert ? MAX_uint8 - B : B;
	};

	for (int32 PixelIdx = 0; PixelIdx < Size; ++PixelIdx)
	{
		uint8* Pixel = &Bytes[PixelIdx * BytesPerPixel];

		Pixel[0] = GetByte(PixelIdx, BlueBytes);
		Pixel[1] = GetByte(PixelIdx, GreenBytes);
		Pixel[2] = GetByte(PixelIdx, RedBytes);
		Pixel[3] = GetByte(PixelIdx, AlphaBytes);
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
								*GreenChannel->GetSelectedItem().Get(),
								*BlueChannel->GetSelectedItem().Get(),
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
