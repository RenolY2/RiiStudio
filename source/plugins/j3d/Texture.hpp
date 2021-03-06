#pragma once

#include <core/common.h>
#include <plugins/gc/GX/VertexTypes.hpp>
#include <vector>

#include <plugins/gc/Export/Texture.hpp>

#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::j3d {

struct TextureData {
  std::string mName; // For linking

  u8 mFormat = 6;
  bool bTransparent = false;
  u16 mWidth, mHeight;

  u8 mPaletteFormat;
  // Not resolved or supported currently
  u16 nPalette;
  u32 ofsPalette;

  s8 mMinLod;
  s8 mMaxLod;
  u8 mMipmapLevel = 0;

  std::vector<u8> mData;

  bool operator==(const TextureData& rhs) const {
    return mName == rhs.mName && mFormat == rhs.mFormat &&
           bTransparent == rhs.bTransparent && mWidth == rhs.mWidth &&
           mHeight == rhs.mHeight && mPaletteFormat == rhs.mPaletteFormat &&
           nPalette == rhs.nPalette && ofsPalette == rhs.ofsPalette &&
           mMinLod == rhs.mMinLod && mMaxLod == rhs.mMaxLod &&
           mMipmapLevel == rhs.mMipmapLevel && mData == rhs.mData;
  }
};

struct Texture : public TextureData, public libcube::Texture {
  // PX_TYPE_INFO_EX("J3D Texture", "j3d_tex", "J::Texture", ICON_FA_IMAGES,
  // ICON_FA_IMAGE);

  std::string getName() const override { return mName; }
  void setName(const std::string& name) override { mName = name; }

  u32 getTextureFormat() const override { return mFormat; }

  void setTextureFormat(u32 format) override { mFormat = format; }
  u32 getMipmapCount() const override {
    assert(mMipmapLevel > 0);
    return mMipmapLevel - 1;
  }
  void setMipmapCount(u32 c) override { mMipmapLevel = c + 1; }
  const u8* getData() const override { return mData.data(); }
  u8* getData() override { return mData.data(); }
  void resizeData() override { mData.resize(getEncodedSize(true)); }

  const u8* getPaletteData() const override { return nullptr; }
  u32 getPaletteFormat() const override { return 0; }

  u16 getWidth() const override { return mWidth; }
  void setWidth(u16 width) override { mWidth = width; }
  u16 getHeight() const override { return mHeight; }
  void setHeight(u16 height) override { mHeight = height; }
};

} // namespace riistudio::j3d
