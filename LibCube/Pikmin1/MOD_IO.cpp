#include "MOD_IO.hpp"

namespace libcube { namespace pikmin1 {

bool MODImporter::tryRead(oishii::BinaryReader& bReader, pl::FileState& state)
{
	Model& mdl = static_cast<Model&>(state);

	// Chunk descriptor
	u32 cDescriptor = 0;
	// 0xFFFF means EoF for this file, aka (u16)-1
	while (/* bReader.tell() < bReader.endpos() && */(cDescriptor = bReader.read<u32>()) != (u16)-1)
	{
		// Chunk start
		const u32 cStart = bReader.tell() - 4;
		// Chunk length
		const u32 cLength = bReader.read<u32>();

		// Check if the chunk start is aligned to the nearest 32 bytes
		if (cStart & 0x1F)
		{
			bReader.warnAt("Chunk start isn't aligned to 32 bytes! Error!\n", cStart - 4, cStart);
			return false;
		}

		switch (static_cast<Chunks>(cDescriptor))
		{
		case Chunks::Header:
			readHeader(bReader, mdl);
			break;
		case Chunks::VertexPosition:
			readChunk(bReader, mdl.mVertexBuffers.mPosition);
			break;
		case Chunks::VertexNormal:
			readChunk(bReader, mdl.mVertexBuffers.mNormal);
			break;
		case Chunks::VertexNBT:
			readChunk(bReader, mdl.mVertexBuffers.mNBT);
			break;
		case Chunks::VertexColor:
			readChunk(bReader, mdl.mVertexBuffers.mColor);
			break;
		case Chunks::VertexUV0:
		case Chunks::VertexUV1:
		case Chunks::VertexUV2:
		case Chunks::VertexUV3:
		case Chunks::VertexUV4:
		case Chunks::VertexUV5:
		case Chunks::VertexUV6:
		case Chunks::VertexUV7:
			readChunk(bReader, mdl.mVertexBuffers.mTexCoords[cDescriptor - (int)Chunks::VertexUV0]);
			break;
		case Chunks::Texture:
			readChunk(bReader, mdl.mTextures);
			break;
		case Chunks::TextureAttribute:
			readChunk(bReader, mdl.mTexAttrs);
			break;
		case Chunks::VertexMatrix:
			readChunk(bReader, mdl.mVertexMatrices);
			break;
		case Chunks::Envelope:
			readChunk(bReader, mdl.mEnvelopes);
			break;
		case Chunks::Mesh:
			readChunk(bReader, mdl.mMeshes);
			break;
		case Chunks::Joint:
			mdl.mJoints.resize(bReader.read<u32>());

			skipPadding(bReader);

			for (auto& joint : mdl.mJoints) {
				// Read joints
				joint << bReader;
				// Expand the collision grid boundaries based on joints
				mdl.mCollisionModel.m_collisionGrid.m_collBounds.expandBound(joint.m_boundingBox);
			}

			skipPadding(bReader);
			break;
		case Chunks::JointName:
			readChunk(bReader, mdl.mJointNames);
			break;
		case Chunks::CollisionPrism:
			readCollisionPrism(bReader, mdl);
			break;
		case Chunks::CollisionGrid:
			mdl.mCollisionModel.m_collisionGrid << bReader;
			break;
		// Caught because 'EoF' is in every file, therefore it is not an unimplemented chunk
		case Chunks::EoF:
			break;
		default:
			// Only chunk left to do is the Material (0x30) chunk
			bReader.warnAt("Unimplemented chunk\n", cStart, cStart + 4);
			// Because we haven't implemented it, we'll have to skip it
			skipChunk(bReader, cLength);
			break;
		}

		if (bReader.tell() > bReader.endpos())
		{
			printf("Fatal error.. attempted to read beyond end of the stream\n");
			throw "Read beyond stream";
		}
	}

	// Construct joint relationships
	for (int i = 0; i < mdl.mJoints.size(); ++i)
	{
		auto& j = mdl.mJoints[i];

		j.mID = i;
		if (j.m_parentIndex != -1)
			mdl.mJoints[j.m_parentIndex].mChildren.push_back(j.mID);
		if (mdl.mJointNames.empty())
			j.mName = std::string("Joint #") + std::to_string(i);
	}

	// Usually, after the EoF chunk, it would be the end of the file
	// But! If there are still bytes after EoF it is assumed there is an INI
	if (bReader.tell() != bReader.endpos())
	{
		DebugReport("INI file found at end of file\n");
	}
	DebugReport("Done reading file\n");

	// PrintMODStats(context);

	return true;
}

void MODImporter::readHeader(oishii::BinaryReader& bReader, Model& mdl)
{
	skipPadding(bReader);

	mdl.mHeader.year = bReader.read<u16>();
	mdl.mHeader.month = bReader.read<u8>();
	mdl.mHeader.day = bReader.read<u8>();
	DebugReport("Creation date of model file (YYYY/MM/DD): %u/%u/%u\n", mdl.mHeader.year, mdl.mHeader.month, mdl.mHeader.day);
	// Used to identify what kind of system is being used,
	// Can be used to identify if using NBT or Classic/Softimage scaling system
	mdl.mHeader.systemsFlag = bReader.read<u32>();
	const bool usingEmboss = mdl.mHeader.systemsFlag & 1;
	const bool whichScaling = mdl.mHeader.systemsFlag & 8;
	if (usingEmboss)
		DebugReport("Model file using Emboss NBT!\n");
	if (whichScaling)
		DebugReport("Model file using Classic Scaling System!\n");
	else
		DebugReport("Model file using SoftImage Scaling System!\n");

	skipPadding(bReader);
}

void MODImporter::readCollisionPrism(oishii::BinaryReader& bReader, Model& mdl)
{
	DebugReport("Reading collision triangle information\n");
	mdl.mCollisionModel.m_baseCollTriInfo.resize(bReader.read<u32>());
	mdl.mCollisionModel.m_baseRoomInfo.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& info : mdl.mCollisionModel.m_baseRoomInfo)
		info << bReader;

	skipPadding(bReader);
	for (auto& collTri : mdl.mCollisionModel.m_baseCollTriInfo)
		collTri << bReader;

	skipPadding(bReader);
}

//	void MOD::read_materials(oishii::BinaryReader& bReader)
//	{
//		DebugReport("Reading materials\n");
//	}

static std::vector<glm::ivec3> ToTris(std::vector<u32>& polys, u32 opcode)
{
	std::vector<glm::ivec3> returnVal;

	if (polys.size() == 3)
	{
		returnVal.push_back(glm::ivec3(polys[0], polys[1], polys[2]));

		return returnVal;
	}

	if (opcode == 0x98)
	{
		u32 n = 2;
		for (u32 i = 0; i < polys.size() - 2; i++)
		{
			u32 tri[3];
			const bool isEven = (n % 2) == 0;
			tri[0] = polys[n - 2];
			tri[1] = isEven ? polys[n] : polys[n - 1];
			tri[2] = isEven ? polys[n - 1] : polys[n];

			if (tri[0] != tri[1] && tri[1] != tri[2] && tri[2] != tri[0])
			{
				returnVal.push_back(glm::ivec3(tri[0], tri[1], tri[2]));
			}

			n++;
		}
	}
	if (opcode == 0x98)
	{
		for (u32 n = 1; n < polys.size() - 1; n++)
		{
			u32 tri[3];
			tri[0] = polys[n];
			tri[1] = polys[n + 1];
			tri[2] = polys[0];

			if (tri[0] != tri[1] && tri[1] != tri[2] && tri[2] != tri[0])
			{
				returnVal.push_back(glm::ivec3(tri[0], tri[1], tri[2]));
			}
		}
	}

	return returnVal;
}

static void OutputFaces(std::ofstream& objFile, Model& toPrint)
{
#if 0
	for (u32 i = 0; i < toPrint.m_batches.size(); i++)
	{
		objFile << "o Mesh" << i << '\n';
		const u32 vcd = toPrint.m_batches[i].m_vcd.m_originalVCD;
		for (const auto& mtxGroup : toPrint.m_batches[i].m_mtxGroups)
		{
			for (const auto& dispList : mtxGroup.m_dispLists)
			{
				oishii::BinaryReader bReader(dispList.m_dispData, dispList.m_dispData.size());
				bReader.setEndian(true);
				bReader.seekSet(0);

				const u8 faceOpcode = bReader.read<u8>();
				if (faceOpcode == 0x98 || faceOpcode == 0xA0)
				{
					const u16 vCount = bReader.read<u16>();

					std::vector<u32> polys(vCount);
					u32 currentPolys = 0;
					for (u32 vC = 0; vC < vCount; vC++)
					{
						if ((vcd & 0x1) == 0x1)
							bReader.read<u8>(); // position matrix
						if ((vcd & 0x2) == 0x2)
							bReader.read<u8>(); // tex1 matrix
						bReader.read<u16>(); // vertex position index

						if (toPrint.m_vnorms.size() > 0)
							bReader.read<u16>(); // vertex normal index
						if ((vcd & 0x4) == 0x4)
							bReader.read<u16>(); // vertex colour index

						int tmpvar = vcd >> 3;
						for (u32 j = 0; j < 8; j++)
						{
							if ((tmpvar & 0x1) == 0x1)
								if (j == 0) bReader.read<u16>();
							tmpvar >>= 1;
						}

						polys[vC] = currentPolys;
						currentPolys++;
					}

					std::vector<glm::ivec3> toOutput = ToTris(polys, faceOpcode);
					for (const auto& elem : toOutput)
					{
						objFile << "f " << elem.x << ' ' << elem.y << ' ' << elem.z << '\n';
					}
				}
			}
		}
	}
#endif
}
#if 0
static void PrintMODStats(Model& toPrint)
{
	DebugReport("There are %zu %s\n", toPrint.m_vertices.size(), toPrint.m_vertices.name);
	DebugReport("There are %zu %s\n", toPrint.m_vnorms.size(), toPrint.m_vnorms.name);
	DebugReport("There are %zu %s\n", toPrint.m_nbt.size(), toPrint.m_nbt.name);
	DebugReport("There are %zu %s\n", toPrint.m_colours.size(), toPrint.m_colours.name);
	for (u32 i = 0; i < 8; i++)
		DebugReport("There are %zu Texture Coordinates in TEXCOORD%d\n", toPrint.m_texcoords[i].size(), i);
	DebugReport("There are %zu %s\n", toPrint.m_textures.size(), toPrint.m_textures.name);
	DebugReport("There are %zu %s\n", toPrint.m_texattrs.size(), toPrint.m_texattrs.name);
	DebugReport("There are %zu %s\n", toPrint.m_vtxmatrices.size(), toPrint.m_vtxmatrices.name);
	DebugReport("There are %zu %s\n", toPrint.m_envelopes.size(), toPrint.m_envelopes.name);
	DebugReport("There are %zu %s\n", toPrint.m_batches.size(), toPrint.m_batches.name);
	DebugReport("There are %zu %s\n", toPrint.m_joints.size(), toPrint.m_joints.name);
	DebugReport("There are %zu %s\n", toPrint.m_jointNames.size(), toPrint.m_jointNames.name);
	DebugReport("There are %zu groups of basic collision triangle information\n", toPrint.m_baseCollTriInfo.size());
	DebugReport("There are %zu groups of basic room information\n", toPrint.m_baseRoomInfo.size());

	std::ofstream objFile("test.obj");
	objFile << "# Made with RiiStudio, by Riidefi & Ambrosia\n";

	for (const auto& vertex : toPrint.m_vertices)
		objFile << "v " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << '\n';

	for (u32 i = 0; i < 8; i++)
	{
		for (const auto& UV : toPrint.m_texcoords[i])
			objFile << "vt " << UV.x << ' ' << UV.y << '\n';
	}

	for (const auto& vertex : toPrint.m_vnorms)
		objFile << "vn " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << '\n';

	if (toPrint.m_baseCollTriInfo.size() != 0)
	{
		objFile << "o CollisionMesh\n";
		for (u32 i = 0; i < toPrint.m_baseCollTriInfo.size(); i++)
		{
			objFile << "f " << toPrint.m_baseCollTriInfo[i].m_faceA + 1 << ' ' << toPrint.m_baseCollTriInfo[i].m_faceC + 1 << ' ' << toPrint.m_baseCollTriInfo[i].m_faceB + 1 << '\n';
		}
	}
	else
	{
		if (toPrint.m_batches.size())
			OutputFaces(objFile, toPrint);
	}

	objFile.close();
}
#endif
void Model::removeMtxDependancy()
{
	for (auto& batch : mMeshes)
	{
		batch.m_depMTXGroups = 0;
		for (auto& mtx_group : batch.m_mtxGroups)
		{
			mtx_group.m_dependant.clear();
		}
	}
}

} } // namespace libcube::pikmin1
