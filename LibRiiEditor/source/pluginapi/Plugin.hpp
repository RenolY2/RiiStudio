#pragma once

#include <string>
#include <memory>
#include <memory>

#include "ui/Window.hpp"
#include "core/WindowManager.hpp"

#include <oishii/reader/binary_reader.hxx>
#include "imgui/imgui.h"

namespace pl {

struct RichName
{
	std::string exposedName;
	std::string namespacedId;
	std::string commandName;
};



struct Package;
struct FileState;
struct Interface;


struct Package
{
	RichName mPackageName; // Command name unused

	std::vector<const FileState*> mEditors; // static pointer
};
enum class InterfaceID
{
	None,

	//! Acts as CLI (and GUI) generator.
	TransformStack,

	TextureList
};



struct AbstractInterface
{
	AbstractInterface(InterfaceID ID = InterfaceID::None) : mInterfaceId(ID) {}
	virtual ~AbstractInterface() = default;

	InterfaceID mInterfaceId;
};


//! A concrete state in memory that can blueprint an editor.
//! IO must be handled by a separate endpoint
//! Undecided on supporting merging with io.
//!
struct FileState
{
	virtual ~FileState() = default;
	FileState() = default;

	// TODO: These must be part of the child class itself!
	std::vector<AbstractInterface*> mInterfaces;
};

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

// Transform stack
struct TransformStack : public AbstractInterface
{
	TransformStack() : AbstractInterface(InterfaceID::TransformStack) {}
	~TransformStack() override = default;

	enum class ParamType
	{
		Flag, // No arguments
		String, // Simple string
		Enum, // Enumeration of values
	};

	struct EnumOptions
	{
		std::vector<RichName> enumeration;
		int mDefaultOptionIndex;
	};

	struct Param
	{
		RichName name;
		ParamType type;

		// Only for enum
		EnumOptions enum_opts;
	};

	struct XForm
	{
		RichName name;
		std::vector<Param> mParams;
		virtual void perform() {}
		virtual ~XForm() = default;
	};

	// Must exist inside this object -- never freed!
	std::vector<XForm*> mStack;
};

struct ITextureList : public AbstractInterface
{
	ITextureList() : AbstractInterface(InterfaceID::TextureList) {}
	~ITextureList() override = default;

	virtual u32 getNumTex() const = 0;
	virtual std::string getNameAt(int idx) const = 0;
	
};

}
