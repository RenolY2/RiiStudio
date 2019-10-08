#pragma once

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>

#include <LibRiiEditor/pluginapi/FileStateSpawner.hpp>
#include "Model.hpp"

namespace libcube { namespace jsystem {

//! Represents the state of a J3D model and textures (BMD, BDL) as well as animations.
//! Unlike the binary equivalents, per-material texture settings have been moved to samplers in materials.
//!
struct J3DCollection : public pl::FileState, public pl::ITextureList
{
	std::vector<std::unique_ptr<J3DModel>> mModels;

	//
	// Interfaces
	//

	// ITextureList
	u32 getNumTex() const override
	{
		return 0;
	}
	std::string getNameAt(int idx) const override
	{
		return "TODO";
	}


	//
	// Construction/Destruction
	//

	~J3DCollection() override = default;
	J3DCollection()
	{
		// Register interfaces
		mInterfaces.push_back(static_cast<pl::ITextureList*>(this));
	}
};

struct J3DCollectionSpawner : public pl::FileStateSpawner
{
	~J3DCollectionSpawner() override = default;
	J3DCollectionSpawner()
	{
		mId = { "J3D Collection", "j3dcollection", "j3d_collection" };
	}

	std::unique_ptr<pl::FileState> spawn() const override
	{
		return std::make_unique<J3DCollection>();
	}
	std::unique_ptr<FileStateSpawner> clone() const override
	{
		return std::make_unique<J3DCollectionSpawner>(*this);
	}
};

} } // namespace libcube::jsystem