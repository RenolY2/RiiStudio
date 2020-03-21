#include "../Sections.hpp"
#include <vendor/ogc/texture.h>
#include <map>
#include <string.h>

namespace riistudio::j3d {

struct Tex
{
	u8 mFormat;
	bool bTransparent;
	u16 mWidth, mHeight;
	u8 mWrapU, mWrapV;
	u8 mPaletteFormat;
	u16 nPalette;
	u32 ofsPalette;
	bool bMipMap;
	bool bEdgeLod;
	bool bBiasClamp;
	u8 mMaxAniso;
	u8 mMinFilter;
	u8 mMagFilter;
	s8 mMinLod;
	s8 mMaxLod;
	u8 mMipmapLevel;
	s16 mLodBias;
	u32 ofsTex;

	void transfer(oishii::BinaryReader& stream)
	{
		oishii::DebugExpectSized dbg(stream, 0x20);

		stream.transfer(mFormat);
		stream.transfer(bTransparent);
		stream.transfer(mWidth);
		stream.transfer(mHeight);
		stream.transfer(mWrapU);
		stream.transfer(mWrapV);
		stream.seek(1);
		stream.transfer(mPaletteFormat);
		stream.transfer(nPalette);
		stream.transfer(ofsPalette);
		// assert(ofsPalette == 0 && "No palette support..");
		stream.transfer(bMipMap);
		stream.transfer(bEdgeLod);
		stream.transfer(bBiasClamp);
		stream.transfer(mMaxAniso);
		stream.transfer(mMinFilter);
		stream.transfer(mMagFilter);
		stream.transfer(mMinLod);
		stream.transfer(mMaxLod);
		stream.transfer(mMipmapLevel);
		stream.seek(1);
		stream.transfer(mLodBias);
		stream.transfer(ofsTex);
	}
	void write(oishii::v2::Writer& stream) const
	{
		oishii::DebugExpectSized dbg(stream, 0x20 - 4);

		stream.write(mFormat);
		stream.write(bTransparent);
		stream.write(mWidth);
		stream.write(mHeight);
		stream.write(mWrapU);
		stream.write(mWrapV);
		stream.seek(1);
		stream.write(mPaletteFormat);
		stream.write(nPalette);
		stream.write<u32>(0);
		// stream.transfer(ofsPalette);
		stream.write(bMipMap);
		stream.write(bEdgeLod);
		stream.write(bBiasClamp);
		stream.write(mMaxAniso);
		stream.write(mMinFilter);
		stream.write(mMagFilter);
		stream.write(mMinLod);
		stream.write(mMaxLod);
		stream.write(mMipmapLevel);
		stream.seek(1);
		stream.write(mLodBias);
		// stream.transfer(ofsTex);
	}
};


void readTEX1(BMDOutputContext& ctx)
{
	auto& reader = ctx.reader;
	if (!enterSection(ctx, 'TEX1')) return;

	ScopedSection g(ctx.reader, "Textures");

	u16 size = reader.read<u16>();
	reader.read<u16>();

	const auto [ofsHeaders, ofsNameTable] = reader.readX<s32, 2>();
	reader.seekSet(g.start);

	std::vector<std::string> nameTable;
	{
		oishii::Jump j(reader, ofsNameTable);
		nameTable = readNameTable(reader);
	}

	std::vector<std::pair<std::unique_ptr<Texture>, std::pair<u32, u32>>> texRaw;

	reader.seek(ofsHeaders);
	for (int i = 0; i < size; ++i)
	{
		Tex tex;
		tex.transfer(reader);
		for (int j = 0; j < ctx.mdl.getMaterials().size(); ++j)
		{
			auto& mat = ctx.mdl.getMaterial(j).get();
			for (int k = 0; k < mat.samplers.size(); ++k)
			{
				auto& samp = (Material::J3DSamplerData&)*mat.samplers[k].get();
				if (samp.btiId == i)
				{
					samp.mTexture = nameTable[i];
					samp.mWrapU = static_cast<libcube::gx::TextureWrapMode>(tex.mWrapU);
					samp.mWrapV = static_cast<libcube::gx::TextureWrapMode>(tex.mWrapV);
					// samp.bMipMap = tex.bMipMap;
					samp.bEdgeLod = tex.bEdgeLod;
					samp.bBiasClamp = tex.bBiasClamp;
					samp.mMaxAniso = static_cast<libcube::gx::AnisotropyLevel>(tex.mMaxAniso);
					samp.mMinFilter = static_cast<libcube::gx::TextureFilter>(tex.mMinFilter);
					samp.mMagFilter = static_cast<libcube::gx::TextureFilter>(tex.mMagFilter);
					samp.mLodBias = static_cast<f32>(tex.mLodBias) / 100.0f;
				}
			}
		}
		auto& inf = texRaw.emplace_back(std::make_unique<Texture>(), std::pair<u32, u32>{0, 0});
		auto& data = *inf.first.get();

		data.mName = nameTable[i];
		data.mFormat = tex.mFormat;
		data.mWidth = tex.mWidth;
		data.mHeight = tex.mHeight;
		data.mPaletteFormat = tex.mPaletteFormat;
		data.nPalette = tex.nPalette;
		data.ofsPalette = tex.ofsPalette;
		data.mMinLod = tex.mMinLod;
		data.mMaxLod = tex.mMaxLod;
		data.mMipmapLevel = tex.mMipmapLevel;

		// ofs:size
		inf.second.first = g.start + ofsHeaders + i * 32 + tex.ofsTex;
		inf.second.second = GetTexBufferSize(tex.mWidth, tex.mHeight, tex.mFormat, tex.bMipMap, tex.mMaxLod);
	}

	// Deduplicate and read.
	// Assumption: Data will not be spliced

	std::vector<std::pair<u32, int>> uniques; // ofs : index
	for (int i = 0; i < size; ++i)
	{
		const auto found = std::find_if(uniques.begin(), uniques.end(), [&](const auto& it) { return it.first == texRaw[i].second.first; });
		if (found == uniques.end())
			uniques.emplace_back(texRaw[i].second.first, i);
	}

	int i = 0;
	for (const auto& it : uniques)
	{
		auto& texpair = texRaw[it.second];
		const auto [ofs, size] = texpair.second;
		std::unique_ptr<Texture> data = std::move(texpair.first);

		data->mData.resize(size);
		assert(ofs + size < reader.endpos());
		// memcpy_s(data->mData.data(), data->mData.size(), reader.getStreamStart() + ofs, size);
		memcpy(data->mData.data(), reader.getStreamStart() + ofs, size);
		ctx.col.addTexture();
		ctx.col.getTexture(ctx.col.getTextures().size() - 1).get() = *data.get();

		++i;
	}

	for (int j = 0; j < ctx.mdl.getMaterials().size(); ++j)
	{
		auto& mat = ctx.mdl.getMaterial(j).get();
		for (int k = 0; k < mat.samplers.size(); ++k)
		{
			auto& samp = mat.samplers[k];
			if (samp->mTexture.empty())
			{
				printf("Material %s: Sampler %u is invalid.\n", mat.getName().c_str(), (u32)i);
				assert(!samp->mTexture.empty());
			}
		}
	}
}
struct TEX1Node final : public oishii::v2::Node
{
	TEX1Node(const ModelAccessor model, const CollectionAccessor col)
		: mModel(model), mCol(col)
	{
		mId = "TEX1";
		mLinkingRestriction.alignment = 32;
	}

	struct TexNames : public oishii::v2::Node
	{
		TexNames(const Model& mdl, const CollectionAccessor col)
			: mMdl(mdl), mCol(col)
		{
			mId = "TexNames";
			getLinkingRestriction().setLeaf();
			getLinkingRestriction().alignment = 4;
		}

		Result write(oishii::v2::Writer& writer) const noexcept
		{
			std::vector<std::string> names;
			
			for (int i = 0; i < mCol.getTextures().size(); ++i)
				names.push_back(mCol.getTexture(i).node().getName());
			writeNameTable(writer, names);

			return {};
		}

		const Model& mMdl;
		const CollectionAccessor mCol;
	};

	struct TexHeaders : public oishii::v2::Node
	{
		TexHeaders(const ModelAccessor mdl, const CollectionAccessor col)
			: mMdl(mdl), mCol(col)
		{
			mId = "TexHeaders";
			getLinkingRestriction().setLeaf();
			getLinkingRestriction().alignment = 32;
		}

		Result write(oishii::v2::Writer& writer) const noexcept
		{
			std::vector<Material::SamplerData> unique_samplers;
			const u32 mat_size = mMdl.getMaterials().size();
			for (u32 i = 0; i < mat_size; ++i)
			{
				const auto& mat = mMdl.getMaterial(i).get();

				for (int j = 0; j < mat.samplers.size(); ++j)
				{
					const auto& sampl = *mat.samplers[j].get();
					assert(!sampl.mTexture.empty());

					if (std::find(unique_samplers.begin(), unique_samplers.end(), sampl) == unique_samplers.end())
						unique_samplers.push_back(sampl);
					else
						DebugReport("Compressed.\n");
				}
			}

			const auto& texFolder = mCol.getTextures();
			for (const auto& sampl : unique_samplers)
			{
				const Texture* tex_data = nullptr;
				for (int i = 0; i < texFolder.size(); ++i)
					if (mCol.getTexture(i).get().getName() == sampl.mTexture)
						tex_data = &mCol.getTexture(i).get();

				assert(tex_data && "Link failure");

				Tex cur_tex;
				cur_tex.mFormat = tex_data->mFormat;
				cur_tex.bTransparent = tex_data->bTransparent;
				cur_tex.mWidth = tex_data->mWidth;
				cur_tex.mHeight = tex_data->mHeight;
				cur_tex.mWrapU = static_cast<u8>(sampl.mWrapU);
				cur_tex.mWrapV = static_cast<u8>(sampl.mWrapV);
				cur_tex.mPaletteFormat = tex_data->mPaletteFormat;
				cur_tex.nPalette = tex_data->nPalette;
				cur_tex.ofsPalette = 0;
				cur_tex.bMipMap = sampl.mMinFilter != libcube::gx::TextureFilter::linear && sampl.mMinFilter != libcube::gx::TextureFilter::near;
				cur_tex.bEdgeLod = sampl.bEdgeLod;
				cur_tex.bBiasClamp = sampl.bBiasClamp;
				cur_tex.mMaxAniso = static_cast<u8>(sampl.mMaxAniso);
				cur_tex.mMinFilter = static_cast<u8>(sampl.mMinFilter);
				cur_tex.mMagFilter = static_cast<u8>(sampl.mMagFilter);
				cur_tex.mMinLod = tex_data->mMinLod;
				cur_tex.mMaxLod = tex_data->mMaxLod;
				cur_tex.mMipmapLevel = tex_data->mMipmapLevel;
				cur_tex.mLodBias = roundf(sampl.mLodBias * 100.0f);
				cur_tex.ofsTex = -1;
				cur_tex.write(writer);
				writer.writeLink<s32>(*this, "TEX1::" + sampl.mTexture);
			}

			return {};
		}

		const ModelAccessor mMdl;
		const CollectionAccessor mCol;
	};
	struct TexEntry : public oishii::v2::Node
	{
		TexEntry(const Model& mdl, const CollectionAccessor col, u32 texIdx)
			: mMdl(mdl), mCol(col), mIdx(texIdx)
		{
			mId = col.getTexture(texIdx).get().mName;
			getLinkingRestriction().setLeaf();
			getLinkingRestriction().alignment = 32;
		}

		Result write(oishii::v2::Writer & writer) const noexcept
		{
			const auto& tex = mCol.getTexture(mIdx).get();
			const auto before = writer.tell();

			writer.seek(tex.mData.size() - 1);
			writer.write<u8>(0);
			writer.seek(-tex.mData.size());

			// memcpy_s(writer.getDataBlockStart() + before, writer.endpos(), tex.mData.data(), tex.mData.size());
			memcpy(writer.getDataBlockStart() + before, tex.mData.data(), tex.mData.size());
			writer.seek(tex.mData.size());
			return {};
		}

		const Model & mMdl;
		const CollectionAccessor mCol;
		const u32 mIdx;
	};
	Result write(oishii::v2::Writer& writer) const noexcept override
	{
		writer.write<u32, oishii::EndianSelect::Big>('TEX1');
		writer.writeLink<s32>({ *this }, { *this, oishii::v2::Hook::EndOfChildren });

		writer.write<u16>((u16)mModel.getJoints().size());
		writer.write<u16>(-1);

		writer.writeLink<s32>(
			oishii::v2::Hook(*this),
			oishii::v2::Hook("TexHeaders"));
		writer.writeLink<s32>(
			oishii::v2::Hook(*this),
			oishii::v2::Hook("TexNames"));
		
		return eResult::Success;
	}

	Result gatherChildren(NodeDelegate& d) const noexcept override
	{
		d.addNode(std::make_unique<TexHeaders>(mModel, mCol));
		d.addNode(std::make_unique<TexNames>(mModel, mCol));

		for (int i = 0; i < mCol.getTextures().size(); ++i)
			d.addNode(std::make_unique<TexEntry>(mModel, mCol, i));

		return {};
	}

private:
	const ModelAccessor mModel;
	const CollectionAccessor mCol;
};

std::unique_ptr<oishii::v2::Node> makeTEX1Node(BMDExportContext& ctx)
{
	return std::make_unique<TEX1Node>(ctx.mdl, ctx.col);
}


}
