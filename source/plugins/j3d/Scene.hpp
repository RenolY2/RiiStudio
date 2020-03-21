#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>

#include "VertexBuffer.hpp"
#include "DrawMatrix.hpp"
#include "Material.hpp"
#include "Joint.hpp"
#include "Shape.hpp"
#include "Texture.hpp"

namespace riistudio::j3d {

struct Collection : public lib3d::Scene {
	bool operator==(const Collection& rhs) const { return true;}
};

struct Model : public lib3d::Model {
	virtual ~Model() = default;
    // Shallow comparison
	bool operator==(const Model& rhs) const {
		return false;
    }
	struct Information
	{
		// For texmatrix calculations
		enum class ScalingRule
		{
			Basic,
			XSI,
			Maya
		};

		ScalingRule mScalingRule = ScalingRule::Basic;
		// nPacket, nVtx

		// Hierarchy data is included in joints.
	};

	Information info;

	struct Bufs
	{
		// FIXME: Good default values
		VertexBuffer<glm::vec3, VBufferKind::position> pos{ VQuantization() };
		VertexBuffer<glm::vec3, VBufferKind::normal> norm{ VQuantization() };
		std::array<VertexBuffer<libcube::gx::Color, VBufferKind::color>, 2> color;
		std::array<VertexBuffer<glm::vec2, VBufferKind::textureCoordinate>, 8> uv;

		Bufs() {}
	} mBufs = Bufs();

	
	std::vector<DrawMatrix> mDrawMatrices;
};

struct ModelAccessor : public kpi::NodeAccessor<Model> {
	KPI_NODE_FOLDER_SIMPLE(Material);
	KPI_NODE_FOLDER_SIMPLE(Joint);
	KPI_NODE_FOLDER_SIMPLE(Shape);
};

struct CollectionAccessor : public kpi::NodeAccessor<Collection> {
	using super = kpi::NodeAccessor<Collection>;
	using super::super;

    KPI_NODE_FOLDER(Model, ModelAccessor);
	KPI_NODE_FOLDER_SIMPLE(Texture);
};

} // namespace riistudio::j3d
