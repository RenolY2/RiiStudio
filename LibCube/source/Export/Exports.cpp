#pragma once

#include "Exports.hpp"
#include "LibDolphin/TPL/TPL.hpp"
#include <SysDolphin/MOD/MOD.hpp>
#include <string>

namespace libcube {

// Static initialization unfortunate

struct TPLEditor : public pl::FileEditor, public pl::Readable, public pl::ITextureList, public pl::TransformStack
{
	~TPLEditor() override = default;
	// public pl::TransformStack
	struct GenStats : public pl::TransformStack::XForm
	{
		GenStats()
		{
			name = pl::RichName{"Generate Statistics", "GenStats", "stats"};
			mParams.push_back(pl::TransformStack::Param{
				pl::RichName{"Verbose", "verbose", "verbose"},
				pl::TransformStack::ParamType::Flag,
				{}});
		}

		void perform() override
		{
			printf("TODO: Generate stats!\n");
		}
	};

	// public pl::Readable
	bool tryRead(oishii::BinaryReader& reader) override
	{
		reader.dispatch<DolphinTPL>(coreRes);
		return true;
	}
	// public pl::ITextureList
	u32 getNumTex() const override
	{
		return coreRes.mTextures.size();
	}
	std::string getNameAt(int idx) const override
	{
		return std::string("Texture #") + std::to_string(idx);
	}
	//	std::unique_ptr<FileEditor> cloneFileEditor() override
	//	{
	//		return std::make_unique<FileEditor>(*this);
	//	}
	
	TPLEditor()
	{
		mMagics.push_back(0x0020AF30);
		mExtensions.push_back(".tpl");
		mInterfaces.push_back((pl::TransformStack*)this);
		mInterfaces.push_back((pl::Readable*)this);
		mInterfaces.push_back((pl::ITextureList*)this);
	}

private:
	DolphinTPL coreRes;
};

namespace pkmin = libcube::pikmin1;
struct MODEditor : public pl::FileEditor, public pl::Readable, public pl::TransformStack, public pl::ITextureList
{
	~MODEditor() override = default;
	// public pl::TransformStack
	struct GenStats : public pl::TransformStack::XForm
	{
		GenStats()
		{
			name = pl::RichName{ "Generate Statistics", "GenStats", "stats" };
			mParams.push_back(pl::TransformStack::Param{
				pl::RichName{"Verbose", "verbose", "verbose"},
				pl::TransformStack::ParamType::Flag,
				{} });
		}

		void perform() override
		{
			printf("TODO: Generate stats!\n");
		}
	};

	// public pl::ITextureList
	u32 getNumTex() const
	{
		if (coreRes.m_textures.empty() != false)
		{
			return coreRes.m_textures.size();
		}
	}
	// public pl::ITextureList
	std::string getNameAt(int idx) const
	{
		return "";
	}

	// public pl::Readable
	bool tryRead(oishii::BinaryReader& reader) override
	{
		coreRes.parse(reader);
		return true;
	}

	MODEditor()
	{
		mMagics.push_back(0x00000000);
		mExtensions.push_back(".mod");
		mInterfaces.push_back((pl::TransformStack*)this);
		mInterfaces.push_back((pl::Readable*)this);
		mInterfaces.push_back((pl::ITextureList*)this);
	}

private:
	pkmin::MOD coreRes;
};

TPLEditor __TPLEditor;
MODEditor __MODEditor;

pl::Package PluginPackage
{
	// Package name
	{
		"LibCube",
		"libcube",
		"gc"
	},
	// Editors
	{
		&__TPLEditor,
		&__MODEditor
	}
};

} // namespace libcube
