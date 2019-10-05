#pragma once

#include "essential_functions.hpp"
#include "Colour.hpp"

namespace libcube { namespace pikmin1 {

/* TODO: PVWCombiner
 * Contains:
 * 12 u8's
 */
struct PVWCombiner
{
	u8 m_unk1;
	u8 m_unk2;
	u8 m_unk4;
	u8 m_unk5;
	u8 m_unk6;
	u8 m_unk7;
	u8 m_unk8;
	u8 m_unk9;
	u8 m_unk10;
	u8 m_unk11;
	u8 m_unk12;
};


/* TODO: PVWTevStage
 * Contains:
 * 8 u8's
 * 2 PVWCombiner's
 */

 /* TODO: PVWTevColReg
  * Contains:
  * 1 ShortColour (#include "Colour.hpp")
  * 1 u32
  * 1 f32
  * 1 un-named class
  * 1 un-named class (seperate from the one before)
  */
struct PVWTevInfo
{
	// TODO: PVWTevColReg
	Colour m_CPREV;
	Colour m_CREG0;
	Colour m_CREG1;
	Colour m_CREG2;

	u32 m_tevStageCount = 0;
	// TODO: PVWTevStage

	PVWTevInfo() = default;
	~PVWTevInfo() = default;

	static void onRead(oishii::BinaryReader& bReader, PVWTevInfo& context)
	{
		// read 3 PVWTevColReg's
		context.m_CPREV << bReader;
		context.m_CREG0 << bReader;
		context.m_CREG1 << bReader;
		context.m_CREG2 << bReader;

		context.m_tevStageCount = bReader.read<u32>();
	}
};



struct Material
{
	// TODO: Material::Colour
	// TODO: PVWPolygonColourInfo
	// TODO: PVWLightingInfo
	// TODO: PVWPeInfo
	// TODO: PVWTextureInfo
	
	Material() = default;
	~Material() = default;
};

}

}
