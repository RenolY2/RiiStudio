#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct Plane
{
	constexpr static const char name[] = "Plane";
	glm::vec3 m_unk1;
	f32 m_unk2;

	Plane() = default;
	~Plane() = default;

	static void onRead(oishii::BinaryReader& bReader, Plane& context)
	{
		read(bReader, context.m_unk1);
		context.m_unk2 = bReader.read<f32>();
	}
};
inline void read(oishii::BinaryReader& reader, Plane& clr)
{
	reader.dispatch<Plane, oishii::Direct, false>(clr);
}

}

}
