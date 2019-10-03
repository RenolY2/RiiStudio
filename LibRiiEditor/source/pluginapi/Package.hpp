#pragma once

#include "RichName.hpp"
#include <vector>

namespace pl {

struct FileState;

struct Package
{
	RichName mPackageName; // Command name unused

	std::vector<const FileState*> mEditors; // static pointer
};

} // namespace pl
