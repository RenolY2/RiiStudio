#pragma once

#include "RichName.hpp"
#include <vector>
#include <memory>

#include "FileStateSpawner.hpp"

namespace pl {

struct FileState;
struct Importer;

struct Package
{
	RichName mPackageName; // Command name unused

	std::vector<const FileStateSpawner*> mEditors; // point to member of class
	std::vector<const Importer*> mImporters;
};

} // namespace pl
