#define _USE_MATH_DEFINES
#include <cmath>

#include "../Sections.hpp"
#include <plugins/gc/Util/glm_serialization.hpp>

namespace riistudio::j3d {

using namespace libcube;

void readJNT1(BMDOutputContext& ctx) {
  auto& reader = ctx.reader;
  if (!enterSection(ctx, 'JNT1'))
    return;

  ScopedSection g(ctx.reader, "Joints");

  u16 size = reader.read<u16>();

  for (u32 i = 0; i < size; ++i)
    ctx.mdl.addJoint();
  ctx.jointIdLut.resize(size);
  reader.read<u16>();

  const auto [ofsJointData, ofsRemapTable, ofsStringTable] =
      reader.readX<s32, 3>();
  reader.seekSet(g.start);

  // Compressible resources in J3D have a relocation table (necessary for
  // interop with other animations that access by index)
  // FIXME: Note and support saving identically
  reader.seekSet(g.start + ofsRemapTable);

  bool sorted = true;
  for (int i = 0; i < size; ++i) {
    ctx.jointIdLut[i] = reader.read<u16>();

    if (ctx.jointIdLut[i] != i)
      sorted = false;
  }

  if (!sorted) {
    DebugReport("Joint IDS are remapped.\n");
    throw "";
  }

  // FIXME: unnecessary allocation of a vector.
  reader.seekSet(ofsStringTable + g.start);
  const auto nameTable = readNameTable(reader);

  for (int i = 0; i < size; ++i) {
    auto& joint = ctx.mdl.getJoint(i).get();
    reader.seekSet(g.start + ofsJointData + ctx.jointIdLut[i] * 0x40);
    joint.id = ctx.jointIdLut[i]; // TODO
    joint.name = nameTable[i];
    const u16 flag = reader.read<u16>();
    joint.flag = flag & 0xf;
    joint.bbMtxType = static_cast<Joint::MatrixType>(flag >> 4);
    const u8 mayaSSC = reader.read<u8>();
    // TODO -- keep track of this fallback behavior
    joint.mayaSSC = mayaSSC == 0xff ? false : mayaSSC;
    u8 pad = reader.read<u8>();
    assert(pad == 0xff);
    joint.scale << reader;
    joint.rotate.x = static_cast<f32>(reader.read<s16>()) / f32(0x7fff) * 180.0f;
    joint.rotate.y = static_cast<f32>(reader.read<s16>()) / f32(0x7fff) * 180.0f;
    joint.rotate.z = static_cast<f32>(reader.read<s16>()) / f32(0x7fff) * 180.0f;
    reader.read<u16>();
    joint.translate << reader;
    joint.boundingSphereRadius = reader.read<f32>();
    joint.boundingBox.min << reader;
    joint.boundingBox.max << reader;
  }
}

struct JNT1Node final : public oishii::v2::Node {
  JNT1Node(const ModelAccessor model) : mModel(model) {
    mId = "JNT1";
    mLinkingRestriction.alignment = 32;
  }

  struct JointData : public oishii::v2::Node {
    JointData(const ModelAccessor mdl) : mMdl(mdl) {
      mId = "JointData";
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 4;
    }

    Result write(oishii::v2::Writer& writer) const noexcept {
      for (int i = 0; i < mMdl.getJoints().size(); ++i) {
        const auto& jnt = mMdl.getJoint(i).get();
        writer.write<u16>((jnt.flag & 0xf) |
                          (static_cast<u32>(jnt.bbMtxType) << 4));
        writer.write<u8>(jnt.mayaSSC);
        writer.write<u8>(0xff);
        jnt.scale >> writer;
        auto rotCvt = [](float x) -> s16 {
          return roundf(x * f32(0x7fff) / 180.0f);
        };
        writer.write(rotCvt(jnt.rotate.x));
        writer.write(rotCvt(jnt.rotate.y));
        writer.write(rotCvt(jnt.rotate.z));
        writer.write<u16>(0xffff);
        jnt.translate >> writer;
        writer.write<f32>(jnt.boundingSphereRadius);
        jnt.boundingBox.min >> writer;
        jnt.boundingBox.max >> writer;
      }

      return {};
    }

    const ModelAccessor mMdl;
  };
  struct JointLUT : public oishii::v2::Node {
    JointLUT(const ModelAccessor mdl) : mMdl(mdl) {
      mId = "JointLUT";
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 2;
    }

    Result write(oishii::v2::Writer& writer) const noexcept {
      for (int i = 0; i < mMdl.getJoints().size(); ++i)
        writer.write<u16>(mMdl.getJoint(i).get().id);

      return {};
    }

    const ModelAccessor mMdl;
  };
  struct JointNames : public oishii::v2::Node {
    JointNames(const ModelAccessor mdl) : mMdl(mdl) {
      mId = "JointNames";
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 4;
    }

    Result write(oishii::v2::Writer& writer) const noexcept {
      auto bones = mMdl.getJoints();

      std::vector<std::string> names(bones.size());

      for (int i = 0; i < bones.size(); ++i)
        names[i] = mMdl.getJoint(i).get().name;
      writeNameTable(writer, names);

      return {};
    }

    const ModelAccessor mMdl;
  };
  Result write(oishii::v2::Writer& writer) const noexcept override {
    writer.write<u32, oishii::EndianSelect::Big>('JNT1');
    writer.writeLink<s32>({*this}, {"SHP1"});

    writer.write<u16>(mModel.getJoints().size());
    writer.write<u16>(-1);

    // TODO: We don't compress joints. I haven't seen this out in the wild yet.

    writer.writeLink<s32>(oishii::v2::Hook(*this),
                          oishii::v2::Hook("JointData"));
    writer.writeLink<s32>(oishii::v2::Hook(*this),
                          oishii::v2::Hook("JointLUT"));
    writer.writeLink<s32>(oishii::v2::Hook(*this),
                          oishii::v2::Hook("JointNames"));

    return eResult::Success;
  }

  Result gatherChildren(NodeDelegate& d) const noexcept override {
    d.addNode(std::make_unique<JointData>(mModel));
    d.addNode(std::make_unique<JointLUT>(mModel));
    d.addNode(std::make_unique<JointNames>(mModel));

    return {};
  }

private:
  const ModelAccessor mModel;
};

std::unique_ptr<oishii::v2::Node> makeJNT1Node(BMDExportContext& ctx) {
  return std::make_unique<JNT1Node>(ctx.mdl);
}

} // namespace riistudio::j3d
