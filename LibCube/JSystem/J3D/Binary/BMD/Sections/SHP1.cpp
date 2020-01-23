#include <LibCube/JSystem/J3D/Binary/BMD/Sections.hpp>

namespace libcube::jsystem {

void readSHP1(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
	
    if (!enterSection(ctx, 'SHP1'))
        return;

    ScopedSection g(reader, "Shapes");

    u16 size = reader.read<u16>();
    ctx.mdl.mShapes.resize(size);
    ctx.shapeIdLut.resize(size);
    reader.read<u16>();

    const auto [ofsShapeData, ofsShapeLut, ofsStringTable,
        // Describes the vertex buffers: GX_VA_POS, GX_INDEX16
        // (We can get this from VTX1)
        ofsVcdList, // "attr table"

        // DRW indices
        ofsDrwIndices,

        // Just a display list
        ofsDL, // "prim data"

        // struct MtxData {
        // s16 current_matrix; // -1 for singlebound
        // u16 matrix_list_size; // Vector of matrix indices; limited by hardware
        // int matrix_list_start;
        // }
        // Each sampled by matrix primitives
        ofsMtxData, // "mtx data"

        // size + offset?
        // matrix primitive splits
        ofsMtxPrimAccessor // "mtx grp"
    ] = reader.readX<s32, 8>();
    reader.seekSet(g.start);

    // Compressible resources in J3D have a relocation table (necessary for interop with other animations that access by index)
    reader.seekSet(g.start + ofsShapeLut);

    bool sorted = true;
    for (int i = 0; i < size; ++i)
    {
        ctx.shapeIdLut[i] = reader.read<u16>();

        if (ctx.shapeIdLut[i] != i)
            sorted = false;
    }

    if (!sorted)
        DebugReport("Shape IDS are remapped.\n");


    // Unused
    // reader.seekSet(ofsStringTable + g.start);
    // const auto nameTable = readNameTable(reader);

    for (int si = 0; si < size; ++si)
    {
        auto& shape = ctx.mdl.mShapes[si];
        reader.seekSet(g.start + ofsShapeData + ctx.shapeIdLut[si] * 0x28);
        shape.id = ctx.shapeIdLut[si];
		printf("Shape (index=%u, id=%u) {\n", si, shape.id);
        // shape.name = nameTable[si];
        shape.mode = static_cast<ShapeData::Mode>(reader.read<u8>());
        assert(shape.mode < ShapeData::Mode::Max);
        reader.read<u8>();
        // Number of matrix primitives (mtxGrpCnt)
        auto num_matrix_prims = reader.read<u16>();
		printf("   num_matrix_prims=%u\n", (u32)num_matrix_prims);
        auto ofs_vcd_list = reader.read<u16>();
		printf("   ofs_vcd_list=%u\n", (u32)ofs_vcd_list);
        // current mtx/mtx list
        auto first_matrix_list = reader.read<u16>();
		printf("   first_matrix_list=%u\n", (u32)first_matrix_list);
		// "Packet" or mtx prim summary (accessor) idx
        auto first_mtx_prim_idx = reader.read<u16>();
		printf("   first_mtx_prim_idx=%u\n", (u32)first_mtx_prim_idx);
		assert(first_matrix_list == first_mtx_prim_idx);
        reader.read<u16>();
        shape.bsphere = reader.read<f32>();
        shape.bbox << reader;
		printf("}\n");

        // Read VCD attributes
        reader.seekSet(g.start + ofsVcdList + ofs_vcd_list);
        gx::VertexAttribute attr = gx::VertexAttribute::Undefined;
        while ((attr = reader.read<gx::VertexAttribute>()) != gx::VertexAttribute::Terminate)
            shape.mVertexDescriptor.mAttributes[attr] = reader.read<gx::VertexAttributeType>();

        // Calculate the VCD bitfield
        shape.mVertexDescriptor.calcVertexDescriptorFromAttributeList();
        const u32 vcd_size = shape.mVertexDescriptor.getVcdSize();

        // Read the two-layer primitives

        
        for (u16 i = 0; i < num_matrix_prims; ++i)
        {
            const u32 prim_idx = first_mtx_prim_idx + i;
            reader.seekSet(g.start + ofsMtxPrimAccessor + prim_idx * 8);
            const auto [dlSz, dlOfs] = reader.readX<u32, 2>();

            struct MatrixData
            {
                s16 current_matrix;
                std::vector<s16> matrixList; // DRW1
            };
            auto readMatrixData = [&, ofsDrwIndices=ofsDrwIndices, ofsMtxData=ofsMtxData]()
            {
				// Assumption: Indexed by raw index *not* potentially remapped ID
                oishii::Jump<oishii::Whence::At> j(reader, g.start + ofsMtxData + (first_matrix_list + i)* 8);
                MatrixData out;
                out.current_matrix = reader.read<s16>();
                u16 list_size = reader.read<u16>();
                s32 list_start = reader.read<s32>();
                out.matrixList.resize(list_size);
                {
                    oishii::Jump<oishii::Whence::At> d(reader, g.start + ofsDrwIndices + list_start * 2);
                    for (u16 i = 0; i < list_size; ++i)
                    {
                        out.matrixList[i] = reader.read<s16>();
                    }
                }
                return out;
            };

            // Mtx Prim Data
            MatrixData mtxPrimHdr = readMatrixData();
            MatrixPrimitive& mprim = shape.mMatrixPrimitives.emplace_back(mtxPrimHdr.current_matrix, mtxPrimHdr.matrixList);
            
            // Now read prim data..
            // (Stripped down display list interpreter function)
            
            reader.seekSet(g.start + ofsDL + dlOfs);
            while (reader.tell() < g.start + ofsDL + dlOfs + dlSz)
            {
                const u8 tag = reader.read<u8, oishii::EndianSelect::Current, true>();
                if (tag == 0) break;
                assert(tag & 0x80 && "Unexpected GPU command in display list.");
                const gx::PrimitiveType type = gx::DecodeDrawPrimitiveCommand(tag);
                u16 nVerts = reader.read<u16, oishii::EndianSelect::Current, true>();
                IndexedPrimitive& prim = mprim.mPrimitives.emplace_back(type, nVerts);

                for (u16 vi = 0; vi < nVerts; ++vi)
                {
                    for (int a = 0; a < (int)gx::VertexAttribute::Max; ++a)
                    {
                        if (shape.mVertexDescriptor[(gx::VertexAttribute)a])
                        {
                            u16 val = 0;

                            switch (shape.mVertexDescriptor.mAttributes[(gx::VertexAttribute)a])
                            {
                            case gx::VertexAttributeType::None:
                                break;
                            case gx::VertexAttributeType::Byte:
                                val = reader.read<u8, oishii::EndianSelect::Current, true>();
                                break;
                            case gx::VertexAttributeType::Short:
                                val = reader.read<u16, oishii::EndianSelect::Current, true>();
                                break;
                            case gx::VertexAttributeType::Direct:
                                if (((gx::VertexAttribute)a) != gx::VertexAttribute::PositionNormalMatrixIndex)
                                {
                                    assert(!"Direct vertex data is unsupported.");
                                    throw "";
                                }
                                val = reader.read<u8, oishii::EndianSelect::Current, true>(); // As PNM indices are always direct, we still use them in an all-indexed vertex
                                break;
                            default:
                                assert("!Unknown vertex attribute format.");
                                throw "";
                            }

                            prim.mVertices[vi][(gx::VertexAttribute)a] = val;
                        }
                    }
                }
            }
        }
    }
}


static void operator>>(const glm::vec3& vec, oishii::v2::Writer& writer)
{
	writer.write(vec.x);
	writer.write(vec.y);
	writer.write(vec.z);
}

struct SHP1Node final : public oishii::v2::Node
{
	SHP1Node(const J3DModel& model)
		: mModel(model)
	{
		mId = "SHP1";
		mLinkingRestriction.alignment = 32;
	}
	Result write(oishii::v2::Writer& writer) const noexcept override
	{
		writer.write<u32, oishii::EndianSelect::Big>('SHP1');
		writer.writeLink<s32>({ *this }, { *this, oishii::v2::Hook::EndOfChildren });

		writer.write<u16>(mModel.mShapes.size());
		writer.write<u16>(-1);

		writer.writeLink<u32>({ *this }, { "ShapeData" });
		writer.writeLink<u32>({ *this }, { "LUT" });
		writer.write<u32>(0); // writer.writeLink<u32>({ *this }, { "NameTable" });
		writer.writeLink<u32>({ *this }, { "VCDList" });
		writer.writeLink<u32>({ *this }, { "MTXList" });
		writer.writeLink<u32>({ *this }, { "DLData" });
		writer.writeLink<u32>({ *this }, { "MTXData" });
		writer.writeLink<u32>({ *this }, { "MTXGrpHdr" });
		// Shape Data
		// Remap table
		// Unused Name table
		// String
		// VCD List
		// DRW Table
		// DL data
		// MTX data
		// MTX Prim hdrs
		return {};
	}
	enum class SubNodeID
	{
		ShapeData,
		LUT,
		NameTable,
		VCDList,
		MTXList,
		DLData,
		MTXData,
		MTXGrpHdr,
		Max,

		_VCDChild = Max,
		_DLChild,
		_MTXChild,
		_MTXGrpChild,
		_DLChildMPrim,
		_MTXDataChild,
		_MTXListChild,
		_MTXListChildMPrim
	};
	struct SubNode : public oishii::v2::Node
	{
		SubNode(const J3DModel& mdl, SubNodeID id, int polyId=-1, int MPrimId = -1)
			: mMdl(mdl), mSID(id), mPolyId(polyId), mMpId(MPrimId)
		{
			u32 align = 4;
			bool leaf = true;
			switch (id)
			{
			case SubNodeID::ShapeData:
				mId = "ShapeData";
				align = 4;
				break;
			case SubNodeID::LUT:
				mId = "LUT";
				align = 2;
				break;
			case SubNodeID::NameTable:
				mId = "NameTable";
				break;
			case SubNodeID::VCDList:
				mId = "VCDList";
				align = 16;
				leaf = false;
				break;
			case SubNodeID::MTXList:
				mId = "MTXList";
				align = 4;
				leaf = false;
				break;
			case SubNodeID::DLData:
				mId = "DLData";
				align = 32;
				leaf = false;
				break;
			case SubNodeID::MTXData:
				mId = "MTXData";
				align = 4;
				leaf = false;
				break;
			case SubNodeID::MTXGrpHdr:
				mId = "MTXGrpHdr";
				align = 2;
				leaf = false;
				break;
			case SubNodeID::_VCDChild:
				mId = std::to_string(mPolyId);
				align = 16;
				leaf = true;
				break;
			case SubNodeID::_DLChild:
				mId = std::to_string(mPolyId);
				align = 32;
				leaf = false;
				break;
			case SubNodeID::_MTXChild:
				mId = std::to_string(mPolyId);
				align = 4;
				leaf = true;
				break;
			case SubNodeID::_MTXGrpChild:
				mId = std::to_string(mPolyId);
				align = 4;
				leaf = true;
				break;
			case SubNodeID::_DLChildMPrim:
				mId = std::to_string(MPrimId);
				align = 32;
				leaf = true;
				break;
			case SubNodeID::_MTXDataChild:
				mId = std::to_string(mPolyId);
				align = 4;
				leaf = true;
				break;

			case SubNodeID::_MTXListChild:
				mId = std::to_string(mPolyId);
				align = 4;
				leaf = false;
				break;
			case SubNodeID::_MTXListChildMPrim:
				mId = std::to_string(MPrimId);
				align = 2;
				leaf = true;
				break;
			}
			if (leaf)
				getLinkingRestriction().setLeaf();
			getLinkingRestriction().alignment = align;
		}

		Result write(oishii::v2::Writer& writer) const noexcept
		{
			switch (mSID)
			{
			case SubNodeID::ShapeData:
			{
				int i = 0;
				for (const auto& shp : mMdl.mShapes)
				{
					writer.write<u8>(static_cast<u8>(shp.mode));
					writer.write<u8>(0xff);
					writer.write<u16>(shp.mMatrixPrimitives.size());
					writer.writeLink<u16>({ "SHP1::VCDList" }, { std::string("SHP1::VCDList::") + std::to_string(i) }); // offset into VCD list
					// TODO -- we don't support compression..
					int mpi = 0;
					for (int j = 0; j < i; ++j)
					{
						mpi += mMdl.mShapes[j].mMatrixPrimitives.size();
					}
					writer.write<u16>(mpi); // Matrix list index of this prim
					writer.write<u16>(mpi); // Matrix primitive index
					writer.write<u16>(0xffff);
					writer.write<f32>(shp.bsphere);
					shp.bbox.m_minBounds >> writer;
					shp.bbox.m_maxBounds >> writer;

					++i;
				}

				break;
			}
			case SubNodeID::LUT:
				// TODO...
				for (int i = 0; i < mMdl.mShapes.size(); ++i)
					writer.write<u16>(i);
				break;
			case SubNodeID::NameTable:
				break; // Always zero
			case SubNodeID::VCDList:
				break; // Children do the writing.
			case SubNodeID::_VCDChild:
				for (auto& x : mMdl.mShapes[mPolyId].mVertexDescriptor.mAttributes)
				{
					writer.write<gx::VertexAttribute>(x.first);
					writer.write<gx::VertexAttributeType>(x.second);
				}
				writer.write<u32>(static_cast<u32>(gx::VertexAttribute::Terminate));
				writer.write<u32>(0);
				break;
			case SubNodeID::MTXList:
			case SubNodeID::_MTXListChild:
				break; // Children write
			
			case SubNodeID::_MTXListChildMPrim:
				for (auto& x : mMdl.mShapes[mPolyId].mMatrixPrimitives[mMpId].mDrawMatrixIndices)
					// TODO: Move -1 to here..
					writer.write<u16>(x);
				break;
			case SubNodeID::DLData:
				break; // Children write
			case SubNodeID::_DLChild:
				break; // MPrims write..
			case SubNodeID::_DLChildMPrim:
			{
				for (auto& prim : mMdl.mShapes[mPolyId].mMatrixPrimitives[mMpId].mPrimitives)
				{
					writer.write<u8>(gx::EncodeDrawPrimitiveCommand(prim.mType));
					writer.write<u16>(prim.mVertices.size());
					for (const auto& v : prim.mVertices)
					{
						for (int a = 0; a < (int)gx::VertexAttribute::Max; ++a)
						{
							if (mMdl.mShapes[mPolyId].mVertexDescriptor[(gx::VertexAttribute)a])
							{
								switch (mMdl.mShapes[mPolyId].mVertexDescriptor.mAttributes.at((gx::VertexAttribute)a))
								{
								case gx::VertexAttributeType::None:
									break;
								case gx::VertexAttributeType::Byte:
									writer.write<u8>(v[(gx::VertexAttribute)a]);
									break;
								case gx::VertexAttributeType::Short:
									writer.write<u16>(v[(gx::VertexAttribute)a]);
									break;
								case gx::VertexAttributeType::Direct:
									if (((gx::VertexAttribute)a) != gx::VertexAttribute::PositionNormalMatrixIndex)
									{
										assert(!"Direct vertex data is unsupported.");
										throw "";
									}
									writer.write<u8>(v[(gx::VertexAttribute)a]);
									break;
								default:
									assert("!Unknown vertex attribute format.");
									throw "";
								}
							}
						}
					}
				}
				// DL pad
				while (writer.tell() % 32)
					writer.write<u8>(0);
				break;
			}
			case SubNodeID::MTXData:
				break; // Children write
			case SubNodeID::_MTXDataChild:
			{
				int i = 0;
				for (const auto& x : mMdl.mShapes[mPolyId].mMatrixPrimitives)
				{
					writer.write<u16>(x.mCurrentMatrix);
					// listSize, listStartIndex
					writer.write<u16>(x.mDrawMatrixIndices.size());
					writer.writeLink<u32>({
						{"SHP1::MTXList"},
						{std::string("SHP1::MTXList::") + std::to_string(mPolyId) + "::" + std::to_string(i) },
						2 });
					++i;
				}
				break;
			}
			case SubNodeID::MTXGrpHdr:
				break; // Children write
			case SubNodeID::_MTXGrpChild:
				for (int i = 0; i < mMdl.mShapes[mPolyId].mMatrixPrimitives.size(); ++i)
				{
					std::string front = std::string("SHP1::DLData::") + std::to_string(mPolyId) + "::";
					// DL size
					writer.writeLink<u32>(
						{ front + std::to_string(i) },
						{ front + std::to_string(i), oishii::v2::Hook::RelativePosition::End });
					// Relative DL offset
					writer.writeLink<u32>(
						{ "SHP1::DLData" },
						{ front + std::to_string(i) });
				}
				break;
			}
			return {};
		}

		Result gatherChildren(NodeDelegate& d) const noexcept override
		{
			switch (mSID)
			{
			case SubNodeID::ShapeData:
			case SubNodeID::LUT:
			case SubNodeID::NameTable:
			case SubNodeID::_VCDChild:
			case SubNodeID::_MTXChild:
			case SubNodeID::_MTXGrpChild:
			case SubNodeID::_MTXDataChild:
				break;
			case SubNodeID::VCDList:
				for (int i = 0; i < mMdl.mShapes.size(); ++i)
					d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_VCDChild, i));
				break;
			case SubNodeID::MTXList:
				for (int i = 0; i < mMdl.mShapes.size(); ++i)
					d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_MTXListChild, i));
				break;
			case SubNodeID::DLData:
				for (int i = 0; i < mMdl.mShapes.size(); ++i)
					d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_DLChild, i));
				break;
			case SubNodeID::MTXData:
				for (int i = 0; i < mMdl.mShapes.size(); ++i)
					d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_MTXDataChild, i));
				break;
			case SubNodeID::MTXGrpHdr:
				for (int i = 0; i < mMdl.mShapes.size(); ++i)
					d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_MTXGrpChild, i));
				break;
			case SubNodeID::_DLChild:
				for (int i = 0; i < mMdl.mShapes[mPolyId].mMatrixPrimitives.size(); ++i)
					d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_DLChildMPrim, mPolyId, i));
				break;
			case SubNodeID::_MTXListChild:
				for (int i = 0; i < mMdl.mShapes[mPolyId].mMatrixPrimitives.size(); ++i)
					d.addNode(std::make_unique<SubNode>(mMdl, SubNodeID::_MTXListChildMPrim, mPolyId, i));
				break;

			}

			return eResult::Success;
		}

		const J3DModel& mMdl;
		const SubNodeID mSID;
		int mPolyId = -1;
		int mMpId = -1;
	};

	Result gatherChildren(NodeDelegate& d) const noexcept override
	{
		auto addSubNode = [&](SubNodeID ID)
		{
			d.addNode(std::make_unique<SubNode>(mModel, ID));
		};

		for (int i = 0; i < static_cast<int>(SubNodeID::Max); ++i)
			addSubNode(static_cast<SubNodeID>(i));

		return {};
	}
	const J3DModel& mModel;
};


std::unique_ptr<oishii::v2::Node> makeSHP1Node(BMDExportContext& ctx)
{
	return std::make_unique<SHP1Node>(ctx.mdl);
}

}
