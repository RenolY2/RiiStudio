// TODO Must be included first
#include <ThirdParty/FileDialogues.hpp>

#include "RiiCore.hpp"
#include <fstream>
#include <oishii/reader/binary_reader.hxx>
#include <LibCube/Export/Exports.hpp>

void RiiCore::drawRoot()
{
	mDockSpace.draw();
	{
		drawMenuBar();
	}
	ImGui::End();
	auto ctx = makeWindowContext();
	mThemeEd.draw(&ctx);
}

void RiiCore::drawMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open"))
			{
				openFile();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar(); // Placement?
	}
}

std::vector<std::string> RiiCore::fileDialogueOpen()
{
	return pfd::open_file("Open File").result();	
}
void RiiCore::openFile(OpenFilePolicy policy)
{
	// TODO: Support other policies
	if (policy != OpenFilePolicy::NewEditor)
		return;

	auto results = fileDialogueOpen();

	if (results.empty())
		return;

	assert(results.size() == 1);
	if (results.size() != 1)
		return;

	auto file = results[0];

	std::ifstream stream(file, std::ios::binary | std::ios::ate);

	if (!stream)
		return;

	const auto size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	std::vector<u8> data(size);
	if (stream.read(reinterpret_cast<char*>(data.data()), size))
	{
		auto reader = std::make_unique<oishii::BinaryReader>(std::move(data), size, file.c_str());

		auto importer = mPluginFactory.spawnImporter(file, *reader.get());

		if (!importer.has_value())
			return;

		if (policy == OpenFilePolicy::NewEditor)
		{
			auto fileState = mPluginFactory.spawnFileState(importer->fileStateId);
			if (fileState.get() == nullptr)
				return;

			// FIXME: encapsulate this functionality
			mCoreRes.currentPluginWindowIndex = mCoreRes.numPluginWindow;
			mCoreRes.numPluginWindow++;


			importer->importer->tryRead(*reader.get(), *fileState.get());

			auto edWindow = std::make_unique<EditorWindow>(std::move(fileState));

			attachWindow(std::move(edWindow));

			
		}

		// TODO -- Check filestate id against current
		// TODO -- Invoke spawned importer
	}
}


RiiCore::RiiCore()
{
	mPluginFactory.registerPlugin(libcube::Package());
}
RiiCore::~RiiCore()
{}
