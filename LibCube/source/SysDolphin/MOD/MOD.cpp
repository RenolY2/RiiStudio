#define OISHII_ALIGNMENT_CHECK 0
#include "MOD.hpp"

namespace libcube { namespace pikmin1 {


void MOD::read_header(oishii::BinaryReader& bReader)
{
	skipPadding(bReader);

	m_header.m_year = bReader.read<u16>();
	m_header.m_month = bReader.read<u8>();
	m_header.m_day = bReader.read<u8>();
	// unsure as to what m_unk is, changes from file to file
	m_header.m_unk = bReader.read<u32>();
	DebugReport("Creation date of model file (YYYY/MM/DD): %u/%u/%u\n", m_header.m_year, m_header.m_month, m_header.m_day);

	skipPadding(bReader);
}

void MOD::read_vertices(oishii::BinaryReader& bReader)
{
	DebugReport("Reading vertices\n");
	m_vertices.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& vertex : m_vertices)
	{
		read(bReader, vertex);
	}
	skipPadding(bReader);
}

void MOD::read_vertexnormals(oishii::BinaryReader& bReader)
{
	DebugReport("Reading vertex normals\n");
	m_vnorms.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& vnorm : m_vnorms)
	{
		read(bReader, vnorm);
	}
	skipPadding(bReader);
}

void MOD::read_vertexcolours(oishii::BinaryReader& bReader)
{
	DebugReport("Reading mesh colours\n");
	m_colours.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& colour : m_colours)
	{
		colour.read(bReader);
	}
	skipPadding(bReader);
}


void MOD::read_textures(oishii::BinaryReader& bReader)
{
	DebugReport("Reading textures\n");
	m_textures.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& texture : m_textures)
	{
		texture.readModFile(bReader);
	}
	skipPadding(bReader);
}

void MOD::read_materials(oishii::BinaryReader& bReader)
{
	DebugReport("Reading materials!\n");
	m_materials.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& material : m_materials)
	{
		material.read(bReader);
	}
	skipPadding(bReader);
}

void MOD::read_texcoords(oishii::BinaryReader& bReader, u32 opcode)
{
	const u32 texIndex = opcode - 0x18;
	DebugReport("Reading texcoord%d\n", texIndex);
	m_texcoords[texIndex].resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& coords : m_texcoords[texIndex])
	{
		coords << bReader;
	}
	skipPadding(bReader);
}

void MOD::read_basecolltriinfo(oishii::BinaryReader& bReader)
{
	DebugReport("Reading collision triangle information\n");
	m_baseCollTriInfo.resize(bReader.read<u32>());
	m_baseRoomInfo.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& info : m_baseRoomInfo)
	{
		bReader.dispatch<BaseRoomInfo, oishii::Direct, false>(info);
	}

	skipPadding(bReader);
	for (auto& collTri : m_baseCollTriInfo)
	{
		bReader.dispatch<BaseCollTriInfo, oishii::Direct, false>(collTri);
	}

	skipPadding(bReader);
}

void MOD::read_collisiongrid(oishii::BinaryReader& bReader)
{
	DebugReport("Reading collision grid\n");
	skipPadding(bReader);
	// TODO: find a way to implement a std::vector so it fits in with every other chunk
	bReader.dispatch<CollGroup, oishii::Direct, false>(m_collisionGrid);
	skipPadding(bReader);
}

void MOD::read_jointnames(oishii::BinaryReader& bReader)
{
	DebugReport("Reading joint names\n");
	m_jointNames.resize(bReader.read<u32>());

	skipPadding(bReader);
	for (auto& name : m_jointNames)
	{
		bReader.dispatch<String, oishii::Direct, false>(name);
	}
	skipPadding(bReader);
}

template<typename T>
inline void readChunk(oishii::BinaryReader& reader, T& out)
{
	reader.dispatch<T, oishii::Direct, false>(out);
}

void MOD::parse(oishii::BinaryReader& bReader)
{	
	bReader.setEndian(true); // big endian

	for (u32 cDescriptor = 0;
		cDescriptor != 0xFFFF; )
	{
		const u32 cStart = bReader.tell(); // get the offset of the chunk start
		cDescriptor = bReader.read<u32>();
		const u32 cLength = bReader.read<u32>();

		if (cStart & 0x1f)
		{
			bReader.warnAt("bReader.tell() isn't aligned with 0x20! ERROR!\n", cStart - 4 , cStart);
			return;
		}

		switch (static_cast<MODCHUNKS>(cDescriptor))
		{
		case MODCHUNKS::MOD_HEADER:
			read_header(bReader);
			break;
		case MODCHUNKS::MOD_VERTEX:
			readChunk(bReader, m_vertices);
			break;
		case MODCHUNKS::MOD_VERTEXNORMAL:
			readChunk(bReader, m_vnorms);
			break;
		case MODCHUNKS::MOD_NBT:
			readChunk(bReader, m_nbt);
			break;
		case MODCHUNKS::MOD_VERTEXCOLOUR:
			readChunk(bReader, m_colours);
			break;
		case MODCHUNKS::MOD_TEXCOORD0:
		case MODCHUNKS::MOD_TEXCOORD1:
		case MODCHUNKS::MOD_TEXCOORD2:
		case MODCHUNKS::MOD_TEXCOORD3:
		case MODCHUNKS::MOD_TEXCOORD4:
		case MODCHUNKS::MOD_TEXCOORD5:
		case MODCHUNKS::MOD_TEXCOORD6:
		case MODCHUNKS::MOD_TEXCOORD7:
			read_texcoords(bReader, cDescriptor);
			break;
		case MODCHUNKS::MOD_TEXTURE:
			read_textures(bReader);
			break;
		case MODCHUNKS::MOD_TEXTURE_ATTRIBUTE:
			read_texattr(bReader);
			break;
		case MODCHUNKS::MOD_VTXMATRIX:
			readChunk(bReader, m_vtxmatrices);
			break;
		case MODCHUNKS::MOD_ENVELOPE:
			readChunk(bReader, m_envelopes);
			break;
		case MODCHUNKS::MOD_MESH:
			readChunk(bReader, m_meshes);
			break;
		case MODCHUNKS::MOD_JOINT:
			readChunk(bReader, m_joints);
			break;
		case MODCHUNKS::MOD_JOINT_NAME:
			read_jointnames(bReader);
			break;
		case MODCHUNKS::MOD_COLLISION_TRIANGLE:
			read_basecolltriinfo(bReader);
			break;
		case MODCHUNKS::MOD_COLLISION_GRID:
			read_collisiongrid(bReader);
			break;

		case MODCHUNKS::MOD_EOF: // caught because it's not a valid chunk to read, so don't even bother warning user and just break
			break;
		default:
			bReader.warnAt("Unimplemented chunk\n", cStart, cStart + 4);
			skipChunk(bReader, cLength);
			break;
		}
	}

	if (bReader.tell() != bReader.endpos())
	{
		DebugReport("INI file found at end of file\n");
	}

	DebugReport("Done reading file\n");
}

} // pikmin1

} // libcube
