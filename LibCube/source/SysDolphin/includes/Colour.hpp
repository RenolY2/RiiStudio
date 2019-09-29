#pragma once

#include "essential_functions.hpp"

namespace libcube { namespace pikmin1 {

struct Colour
{
	constexpr static const char name[] = "Colour";

	u8 m_R, m_G, m_B, m_A;
	Colour() { m_R = m_G = m_B = m_A = 0; }
	Colour(u8 _r, u8 _g, u8 _b, u8 _a) : m_R(_r), m_G(_g), m_B(_b), m_A(_a) {}
	~Colour() = default;

	static void onRead(oishii::BinaryReader& bReader, Colour& context)
	{
		context.set(bReader.read<u8>(),
			bReader.read<u8>(),
			bReader.read<u8>(),
			bReader.read<u8>());
	}

	inline void set(u8 _r, u8 _g, u8 _b, u8 _a)
	{
		m_R = _r;
		m_G = _g;
		m_B = _b;
		m_A = _a;
	}

	// function below was decompiled from sysCore.dll
	void lerp(Colour& lerpTo, float t)
	{
		m_R = ((lerpTo.m_R - m_R) * t + m_R);
		m_G = ((lerpTo.m_G - m_G) * t + m_G);
		m_B = ((lerpTo.m_B - m_B) * t + m_B);
		m_A = ((lerpTo.m_A - m_A) * t + m_A);
	}
};
inline void read(oishii::BinaryReader& reader, Colour& clr)
{
	reader.dispatch<Colour, oishii::Direct, false>(clr);
}

struct ShortColour
{
	constexpr static const char name[] = "Short Colour";

	u16 m_R, m_G, m_B, m_A;
	ShortColour() { m_R = m_G = m_B = m_A = 0; }
	ShortColour(u16 _r, u16 _g, u16 _b, u16 _a) : m_R(_r), m_G(_g), m_B(_b), m_A(_a) {}

	static void onRead(oishii::BinaryReader& bReader, ShortColour& context)
	{
		context.set(bReader.read<u16>(),
			bReader.read<u16>(),
			bReader.read<u16>(),
			bReader.read<u16>());
	}

	inline void set(u16 _r, u16 _g, u16 _b, u16 _a)
	{
		m_R = _r;
		m_G = _g;
		m_B = _b;
		m_A = _a;
	}
};
inline void read(oishii::BinaryReader& reader, ShortColour& clr)
{
	reader.dispatch<ShortColour, oishii::Direct, false>(clr);
}

}

}
