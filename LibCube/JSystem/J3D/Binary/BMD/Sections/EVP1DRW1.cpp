#include <LibCube/JSystem/J3D/Binary/BMD/Sections.hpp>
#include <LibCube/JSystem/J3D/Binary/BMD/OutputCtx.hpp>

namespace libcube::jsystem {

void readEVP1DRW1(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
    // We infer DRW1 -- one bone -> singlebound, else create envelope
	std::vector<DrawMatrix> envelopes;

	// First read our envelope data
	if (enterSection(ctx, 'EVP1'))
	{
		ScopedSection g(reader, "Envelopes");

		u16 size = reader.read<u16>();
		envelopes.resize(size);
		reader.read<u16>();

		const auto [ofsMatrixSize, ofsMatrixIndex, ofsMatrixWeight, ofsMatrixInvBind] = reader.readX<s32, 4>();

		int mtxId = 0;
		// int maxJointIndex = -1;

		reader.seekSet(g.start);

		for (int i = 0; i < size; ++i)
		{
			const auto num = reader.peekAt<u8>(ofsMatrixSize + i);

			for (int j = 0; j < num; ++j)
			{
				const auto index = reader.peekAt<u16>(ofsMatrixIndex + mtxId * 2);
				const auto influence = reader.peekAt<f32>(ofsMatrixWeight + mtxId * 4);

				envelopes[i].mWeights.emplace_back(index, influence);

				// While this assumption holds on runtime, the format stores them for all bones.
				//	if (index > maxJointIndex)
				//		maxJointIndex = index;

				++mtxId;
			}
		}

		for (int i = 0; i < ctx.mdl.mJoints.size(); ++i)
		{
			oishii::Jump<oishii::Whence::Set> gInv(reader, g.start + ofsMatrixInvBind + i * 0x30);

			auto& mtx = ctx.mdl.mJoints[i].inverseBindPoseMtx;

			for (auto& f : mtx)
				f = reader.read<f32>();
			// transferMatrix<oishii::BinaryReader>(ctx.mdl.mJoints[i].inverseBindPoseMtx, reader);
		}
	}

	// Now construct vertex draw matrices.
	if (enterSection(ctx, 'DRW1'))
	{
		ScopedSection g(reader, "Vertex Draw Matrix");

		ctx.mdl.mDrawMatrices.clear();
		ctx.mdl.mDrawMatrices.resize(reader.read<u16>() - envelopes.size()); // Bug in nintendo's tooling corrected on runtime
		reader.read<u16>();

		const auto [ofsPartialWeighting, ofsIndex] = reader.readX<s32, 2>();

		reader.seekSet(g.start);

		int i = 0;
		for (auto& mtx : ctx.mdl.mDrawMatrices)
		{

			const auto multipleInfluences = reader.peekAt<u8>(ofsPartialWeighting + i);
			const auto index = reader.peekAt<u16>(ofsIndex + i * 2);
			//	if (multipleInfluences)
			//		printf("multiple influences: DRW=%u, EVP=%u\n", i, index);

			mtx = multipleInfluences ? envelopes[index] : DrawMatrix{ std::vector<DrawMatrix::MatrixWeight>{DrawMatrix::MatrixWeight(index, 1.0f)} };
			i++;
		}
	}
}


struct EVP1Node
{
	static const char* getNameId() { return "EVP1 EnVeloPe"; }
	virtual const oishii::v2::Node& getSelf() const = 0;

	void write(oishii::v2::Writer& writer) const
	{
		writer.write<u32, oishii::EndianSelect::Big>('EVP1');
		writer.writeLink<s32>({ getSelf() }, { "DRW1" });

		writer.write<u16>(envelopesToWrite.size());
		writer.write<u16>(-1);
		// ofsMatrixSize, ofsMatrixIndex, ofsMatrixWeight, ofsMatrixInvBind
		writer.writeLink<s32>(
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("MatrixSizeTable"));
		writer.writeLink<s32>(
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("MatrixIndexTable"));
		writer.writeLink<s32>(
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("MatrixWeightTable"));
		writer.writeLink<s32>(
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("MatrixInvBindTable"));
	}
	struct SimpleEvpNode : public oishii::v2::Node
	{
		
		SimpleEvpNode(const std::vector<DrawMatrix>& from, const std::vector<int>& toWrite, const std::vector<float>& weightPool, const J3DModel* mdl = 0)
			: mFrom(from), mToWrite(toWrite), mWeightPool(weightPool), mMdl(mdl)
		{
			getLinkingRestriction().setFlag(oishii::v2::LinkingRestriction::Leaf);
		}
		Result gatherChildren(NodeDelegate&) const noexcept { return {}; }
		const std::vector<DrawMatrix>& mFrom;
		const std::vector<int>& mToWrite;
		const std::vector<float>& mWeightPool;
		const J3DModel* mMdl;
	};
	struct MatrixSizeTable : public SimpleEvpNode
	{
		MatrixSizeTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{
			mId = "MatrixSizeTable";
		}
		Result write(oishii::v2::Writer& writer) const noexcept
		{
			for (int i : mToWrite)
				writer.write<u8>(mFrom[i].mWeights.size());
			return {};
		}
	};
	struct MatrixIndexTable : public SimpleEvpNode
	{
		MatrixIndexTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{
			mId = "MatrixIndexTable";
		}
		Result write(oishii::v2::Writer& writer) const noexcept
		{
			for (int i : mToWrite)
			{
				const auto& mtx = mFrom[i].mWeights;
				for (const auto& w : mtx)
					writer.write<u16>(w.boneId);
			}
			return {};
		}
	};
	struct MatrixWeightTable : public SimpleEvpNode
	{
		MatrixWeightTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{
			mId = "MatrixWeightTable";
			mLinkingRestriction.alignment = 8;
		}
		Result write(oishii::v2::Writer& writer) const noexcept
		{
			for (f32 f : mWeightPool)
				writer.write<f32>(f);
			return {};
		}
	};
	struct MatrixInvBindTable : public SimpleEvpNode
	{
		MatrixInvBindTable(const EVP1Node& node, const J3DModel& md)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool, &md)
		{
			mId = "MatrixInvBindTable";
		}
		Result write(oishii::v2::Writer& writer) const noexcept
		{
			for (const auto& jnt : mMdl->mJoints)
			{
				for (const auto f : jnt.inverseBindPoseMtx)
					writer.write(f);
			}
			return {};
		}
	};
	void gatherChildren(oishii::v2::Node::NodeDelegate& ctx) const
	{
		ctx.addNode(std::make_unique<MatrixSizeTable>(*this));
		ctx.addNode(std::make_unique<MatrixIndexTable>(*this));
		ctx.addNode(std::make_unique<MatrixWeightTable>(*this));
		ctx.addNode(std::make_unique<MatrixInvBindTable>(*this, mdl));
	}

	EVP1Node(BMDExportContext& e)
		: mdl(e.mdl)
	{
		for (int i = 0; i < mdl.mDrawMatrices.size(); ++i)
		{
			if (mdl.mDrawMatrices[i].mWeights.size() <= 1)
			{
				assert(mdl.mDrawMatrices[i].mWeights[0].weight == 1.0f);
			}
			else
			{
				envelopesToWrite.push_back(i);
				for (const auto& it : mdl.mDrawMatrices[i].mWeights)
				{
					//if (std::find(weightPool.begin(), weightPool.end(), it.weight) == weightPool.end())
						weightPool.push_back(it.weight);
				}
			}
		}
	}
	std::vector<f32> weightPool;
	std::vector<int> envelopesToWrite;
	J3DModel& mdl;
};


struct DRW1Node
{
	static const char* getNameId() { return "DRW1"; }
	virtual const oishii::v2::Node& getSelf() const = 0;

	void write(oishii::v2::Writer& writer) const
	{
		writer.write<u32, oishii::EndianSelect::Big>('DRW1');
		writer.writeLink<s32>(oishii::v2::Link{
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("VTX1"/*getSelf(), oishii::Hook::EndOfChildren*/) });

		u32 evp = 0;
		for (const auto& drw : mdl.mDrawMatrices)
			if (drw.mWeights.size() > 1)
				evp += 1;

		writer.write<u16>(mdl.mDrawMatrices.size() + evp);
		writer.write<u16>(-1);
		writer.writeLink<s32>(oishii::v2::Link{
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("MatrixTypeTable")
			});
		writer.writeLink<s32>(oishii::v2::Link{
			oishii::v2::Hook(getSelf()),
			oishii::v2::Hook("DataTable")
			});
	}
	struct MatrixTypeTable : public oishii::v2::Node {
		MatrixTypeTable(const J3DModel& mdl)
			: mMdl(mdl)
		{
			mId = "MatrixTypeTable";
		}

		Result write(oishii::v2::Writer& writer) const noexcept
		{
			for (const auto& drw : mMdl.mDrawMatrices)
				writer.write<u8>(drw.mWeights.size() > 1);
			// Nintendo's bug
			for (const auto& drw : mMdl.mDrawMatrices)
				if (drw.mWeights.size() > 1)
					writer.write<u8>(1);
			return {};
		}

		const J3DModel& mMdl;
	};
	struct DataTable : public oishii::v2::Node {
		DataTable(const J3DModel& mdl)
			: mMdl(mdl)
		{
			mId = "DataTable";
			getLinkingRestriction().alignment = 2;
		}

		Result write(oishii::v2::Writer& writer) const noexcept
		{
			// TODO -- we can do much better
			std::vector<int> envelopesToWrite;
			for (int i = 0; i < mMdl.mDrawMatrices.size(); ++i)
			{
				if (mMdl.mDrawMatrices[i].mWeights.size() <= 1)
				{
					assert(mMdl.mDrawMatrices[i].mWeights[0].weight == 1.0f);
				}
				else
				{
					envelopesToWrite.push_back(i);
				}
			}

			int i = 0;
			for (const auto& drw : mMdl.mDrawMatrices)
			{
				assert(!drw.mWeights.empty());
				writer.write<u16>(drw.mWeights.size() > 1 ? std::find(envelopesToWrite.begin(), envelopesToWrite.end(), i) - envelopesToWrite.begin() : drw.mWeights[0].boneId);
				++i;
			}
			return {};
		}

		const J3DModel& mMdl;
	};
	void gatherChildren(oishii::v2::Node::NodeDelegate& ctx) const
	{
		ctx.addNode(std::make_unique<MatrixTypeTable>(mdl));
		ctx.addNode(std::make_unique<DataTable>(mdl));
	}

	DRW1Node(BMDExportContext& e)
		: mdl(e.mdl)
	{
		
	}
	J3DModel& mdl;
};

std::unique_ptr<oishii::v2::Node> makeEVP1Node(BMDExportContext& ctx)
{
	return std::make_unique<LinkNode<EVP1Node>>(ctx);
}
std::unique_ptr<oishii::v2::Node> makeDRW1Node(BMDExportContext& ctx)
{
	return std::make_unique<LinkNode<DRW1Node>>(ctx);
}
}