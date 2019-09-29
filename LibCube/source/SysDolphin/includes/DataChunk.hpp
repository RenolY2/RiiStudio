#pragma once

#include "essential_functions.hpp"
#include <oishii/writer/binary_writer.hxx>
#include "CmdStream.hpp"

namespace libcube { namespace pikmin1 {

struct DataChunk
{
	std::vector<float> m_data;

	DataChunk() = default;
	~DataChunk() = default;

	void onRead(oishii::BinaryReader& bReader, DataChunk& context)
	{
		m_data.resize(bReader.read<u32>());
		for (auto& data : m_data)
			data = bReader.read<f32>();
	}

	void write(oishii::Writer & bWriter)
	{
		bWriter.write<u32>(m_data.size());
		for (auto& data : m_data)
			bWriter.write<f32>(data);
	}
};
inline void read(oishii::BinaryReader& bReader, DataChunk& context)
{
	bReader.dispatch<DataChunk, oishii::Direct, false>(context);
}

}

}
