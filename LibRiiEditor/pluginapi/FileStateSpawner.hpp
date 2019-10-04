#pragma once

#include <vector>
#include <memory>

namespace pl {

struct FileState;

//! Constructs a file state pointer for use in runtime polymorphic code.
//!
struct FileStateSpawner
{
	virtual ~FileStateSpawner() = default;

	virtual std::unique_ptr<FileState> spawn() const = 0;
};

} // namespace pl
