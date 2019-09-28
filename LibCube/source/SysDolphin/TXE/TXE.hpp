#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <vector>

namespace libcube {

namespace pikmin1 {

enum class TXEFormats : u16
{
	RGB565 = 0,
	CMPR, // 1
	RGB5A3, // 2
	I4, // 3
	I8, // 4
	IA4, // 5
	IA8, // 6
	RGBA8 //7
};

struct TXE
{
	u16 m_width = 0;
	u16 m_height = 0;
	u16 m_format;
	u32 m_dataSize = 0;
	std::vector<u8> m_imageData;

	TXE() = default;
	~TXE() = default;

	constexpr static const char name[] = "Pikmin 1 Texture File";

	static void onRead(oishii::BinaryReader&, TXE&);

	void importTXE(oishii::BinaryReader&);
	void importModTXE(oishii::BinaryReader&);
};
inline void read(oishii::BinaryReader& reader, TXE& clr)
{
	reader.dispatch<TXE, oishii::Direct, false>(clr);
}

}

}
