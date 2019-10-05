#include "PluginFactory.hpp"
#include <LibRiiEditor/common.hpp>

bool PluginFactory::registerPlugin(const pl::Package& package)
{
	if (package.mEditors.empty())
	{
		DebugReport("Plugin has no editors");
		return false;
	}

	
	return true;
}

std::unique_ptr<pl::FileState> PluginFactory::create(const std::string& fileName, oishii::BinaryReader& reader)
{
	using MatchResult = pl::FileStateSpawner::MatchResult;

	std::map<std::size_t, MatchResult> matched;

	// Unfortunate linear time search
	std::size_t i;
	for (const auto& plugin : mPlugins)
	{
		ScopedInc g(i);

		const auto match = plugin->match(fileName, reader);

		if (match != MatchResult::Mismatch)
			matched[i] = match;
	}

	if (matched.size() == 0)
	{
		DebugReport("No matches.\n");
		return nullptr;
	}
	else if (matched.size() > 1)
	{
		// FIXME: Find best match or prompt user
		DebugReport("Multiple matches.\n");
		assert(0); exit(1);
	}
	else
	{
		return mPlugins[matched.begin()->first]->spawn();
	}
}
