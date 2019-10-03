#pragma once

#include <vector>
#include <string>
#include <oishii/reader/binary_reader.hxx>

namespace pl {

struct FileState;

//! No longer an interface, but a separate node in the data mesh.
//!
struct Readable
{
	Readable() = default;
	virtual ~Readable() = default;

	std::vector<std::string> mExtensions;
	std::vector<u32> mMagics;

	virtual bool tryRead(oishii::BinaryReader& reader, FileState& state) = 0;
};

} // namespace pl
