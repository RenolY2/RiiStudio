#pragma once

#include <vector>
#include <memory>

namespace oishii { class BinaryReader; }

namespace pl {

struct FileState;

//! Constructs a file state pointer for use in runtime polymorphic code.
//!
struct FileStateSpawner
{
	// EFE inspired
	enum class MatchResult
	{
		Magic,
		Contents,
		Mismatch
	};

	virtual ~FileStateSpawner() = default;

	virtual std::unique_ptr<FileState> spawn() const = 0;
	virtual MatchResult match(const std::string& fileName, oishii::BinaryReader& reader) const = 0;
};

} // namespace pl
