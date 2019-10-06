/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include <LibRiiEditor/common.hpp>
#include "Pikmin1Model.hpp"
#include <LibRiiEditor/pluginapi/IO/Importer.hpp>

namespace libcube { namespace pikmin1 {

class MODImporter;

class MODImporterSpawner : public pl::ImporterSpawner
{
	std::pair<MatchResult, std::string> match(const std::string& fileName, oishii::BinaryReader& reader) const override
	{
		// Unfortunately no magic -- just compare extension for now.

		const std::string ending = ".mod";

		if (fileName.size() > 4 && std::equal(ending.rbegin(), ending.rend(), fileName.rbegin()))
		{
			return { MatchResult::Contents, "MOD" };
		}
		return { MatchResult::Mismatch, "" };
	}
	std::unique_ptr<pl::Importer> spawn() const override
	{
		return std::make_unique<MODImporter>();
	}
	std::unique_ptr<ImporterSpawner> clone() const override
	{
		return std::make_unique<MODImporterSpawner>();
	}

};

class MODImporter : public pl::Importer
{
	bool tryRead(oishii::BinaryReader& reader, pl::FileState& state) override
	{
		Model& mdl = static_cast<Model&>(state);

		

		return true;
	}
};

} } // namespace libcube::pikmin1
