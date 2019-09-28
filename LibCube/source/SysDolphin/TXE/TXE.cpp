#include "TXE.hpp"

namespace libcube { namespace pikmin1 {

void TXE::onRead(oishii::BinaryReader& bReader, TXE& context)
{
	context.importTXE(bReader);
}

void TXE::importTXE(oishii::BinaryReader& bReader)
{
	m_width = bReader.read<u16>();
	m_height = bReader.read<u16>();
	m_unk1 = bReader.read<u16>();
	m_format = bReader.read<u16>();

	m_unk2 = bReader.read<u32>();

	for (u32 i = 0; i < 4; i++)
		bReader.read<u32>(); // padding

	m_imageData.resize(bReader.read<u32>());

	for (auto& pixelData : m_imageData)
		pixelData = bReader.read<u8>();
}

}

}
