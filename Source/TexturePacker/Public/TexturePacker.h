#pragma once

#include "CoreMinimal.h"

class UTexture;

namespace TexturePacker
{
enum class EChannel
{
	B = 0,
	G = 1,
	R = 2,
	A = 3,
	White,
	Black
};

struct FChannelOption
{
	FChannelOption(UTexture* InTexture, EChannel InChannel, bool bInInvert = false, bool bInKeepSrgb = false)
		: Texture(InTexture), Channel(InChannel), bInvert(bInInvert), bKeepSrgb(bInKeepSrgb){};

	UTexture* Texture;
	EChannel Channel;
	bool bInvert;
	bool bKeepSrgb;
};

TEXTUREPACKER_API void PackTexture(const TCHAR* PackagePath,
								   const TCHAR* TextureName,
								   const int32 InSizeX,
								   const int32 InSizeY,
								   const FChannelOption Red,
								   const FChannelOption Green,
								   const FChannelOption Blue,
								   TOptional<FChannelOption> Alpha);
}  // namespace TexturePacker