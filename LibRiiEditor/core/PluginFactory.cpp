#include "PluginFactory.hpp"
#include <LibRiiEditor/common.hpp>

bool PluginFactory::registerPlugin(const pl::Package& package)
{
	if (package.mEditors.empty() && package.mImporters.empty())
	{
		DebugReport("Plugin is empty.");
		return true;
	}

	for (const auto ptr : package.mEditors)
		mPlugins.emplace_back(ptr->clone());

	for (const auto ptr : package.mImporters)
		mImporters.emplace_back(ptr->clone());

	for (const auto ptr : package.mExporters)
		mExporters.emplace_back(ptr->clone());

	
	return true;
}

std::optional<PluginFactory::SpawnedImporter> PluginFactory::spawnImporter(const std::string& fileName, oishii::BinaryReader& reader)
{
	using MatchResult = pl::ImporterSpawner::MatchResult;

	std::map<std::size_t, std::pair<MatchResult, std::string>> matched;

	// Unfortunate linear time search
	std::size_t i = 0;
	for (const auto& plugin : mImporters)
	{
		ScopedInc incrementor(i);
		oishii::JumpOut reader_guard(reader, reader.tell());

		const auto match = plugin->match(fileName, reader);

		if (match.first != MatchResult::Mismatch)
			matched[i] = match;
	}

	if (matched.size() == 0)
	{
		DebugReport("No matches.\n");
		return {};
	}
	else if (matched.size() > 1)
	{
		// FIXME: Find best match or prompt user
		DebugReport("Multiple matches.\n");
		return {};
	}
	else
	{
		return std::optional<PluginFactory::SpawnedImporter> {
			SpawnedImporter {
				matched.begin()->second.second,
				mImporters[matched.begin()->first]->spawn()
			}
		};
	}
}

std::unique_ptr<pl::FileState> PluginFactory::spawnFileState(const std::string& fileStateId)
{
	for (const auto& it : mPlugins)
	{
		if (it->mId.namespacedId == fileStateId)
			return it->spawn();
	}

	return nullptr;
}
