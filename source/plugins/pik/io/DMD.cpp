#include "DMD.hpp"

#include <core/common.h>
#include <core/kpi/Node.hpp>

#include <oishii/v2/writer/binary_writer.hxx>
#include <oishii/reader/binary_reader.hxx>

#include <string>

#include <plugins/pik/Scene.hpp>

#include <plugins/pik/util/Parser.hpp>

namespace riistudio::pik {

inline bool ends_with(const std::string& value, const std::string& ending) {
	return ending.size() <= value.size() && std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


class DMD {
public:
	std::string canRead(const std::string& file, oishii::BinaryReader& reader) const {
		return ends_with(file, "dmd") ? typeid(PikminCollection).name() : "";
	}

	void readModel(PikminModelAccessor model, Parser& parser) const {
		model.get().nJoints = 3;
	}

	void read(kpi::IDocumentNode& node, oishii::BinaryReader& reader) const {
		assert(dynamic_cast<PikminCollection*>(&node) != nullptr);
		PikminCollectionAccessor collection(&node);

		// Awful code but not a real issue...
		std::string data;
		data.resize(reader.endpos());
		memcpy(data.data(), reader.getStreamStart(), data.size());
		Parser parser(std::stringstream{ data });

		readModel(collection.addPikminModel(), parser);
	}
};

void InstallDMD(kpi::ApplicationPlugins& installer) {
	installer.addDeserializer<DMD>();
}

} // namespace riistudio::pik
