#pragma once

#include <vector>
#include <string>
#include <oishii/reader/binary_reader.hxx>

namespace pl {

struct FileState;

//! No longer an interface, but a separate node in the data mesh.
//!
struct Importer
{
	Importer() = default;
	virtual ~Importer() = default;

	struct Options
	{
		std::vector<std::string> mExtensions;
		std::vector<u32> mMagics;

		std::string mPinId; //! The file state being imported--for the linker
	} mOptions;

	virtual bool tryRead(oishii::BinaryReader& reader, FileState& state) = 0;
};

// TODO: Adapter
//	interface AbstractImporter
//	{
//		Importer::Options mOptions;
//	
//		// Where TState inherits FileState
//		bool import(oishii::BinaryReader& reader, TState& state);
//	};

} // namespace pl
