#include <plugins/gc/Util/DisplayList.hpp>

namespace libcube {

void DecodeMeshDisplayList(
    oishii::BinaryReader& reader, u32 start, u32 size,
    IMeshDLDelegate& delegate, const VertexDescriptor& descriptor,
    std::map<gx::VertexBufferAttribute, u32>* optUsageMap) {
  oishii::Jump<oishii::Whence::Set> g(reader, start);

  const u32 end = reader.tell() + size;
  while (reader.tell() < end) {
    const u8 tag = reader.readUnaligned<u8>();
    if (tag == 0)
      return;
    assert(tag & 0x80 && "Unexpected GPU command in display list.");

    u16 nVerts = reader.readUnaligned<u16>();
    IndexedPrimitive& prim = delegate.addIndexedPrimitive(
        gx::DecodeDrawPrimitiveCommand(tag), nVerts);

    for (u16 vi = 0; vi < nVerts; ++vi) {
      for (int a = 0; a < (int)gx::VertexAttribute::Max; ++a) {
        if (descriptor.mBitfield & (1 << a)) {
          u16 val = 0;

          switch (
              descriptor.mAttributes.at(static_cast<gx::VertexAttribute>(a))) {
          case gx::VertexAttributeType::None:
            break;
          case gx::VertexAttributeType::Byte:
            val = reader.read<u8, oishii::EndianSelect::Current, true>();
            assert(val != 0xff);
            break;
          case gx::VertexAttributeType::Short:
            val = reader.read<u16, oishii::EndianSelect::Current, true>();
            if (val == 0xffff) {
              printf("Index: %u, Attribute: %x\n", (u32)vi, (u32)a);
              reader.warnAt("Disabled vertex", reader.tell() - 2,
                            reader.tell());
            }
            assert(val != 0xffff);
            break;
          case gx::VertexAttributeType::Direct:
            if (((gx::VertexAttribute)a) !=
                    gx::VertexAttribute::PositionNormalMatrixIndex &&
                ((gx::VertexAttribute)a) !=
                    gx::VertexAttribute::Texture0MatrixIndex &&
				((gx::VertexAttribute)a) !=
				gx::VertexAttribute::Texture1MatrixIndex) {
              assert(!"Direct vertex data is unsupported.");
              throw "";
            }
            // As PNM indices are always direct, we
            // still use them in an all-indexed vertex
            val = reader.read<u8, oishii::EndianSelect::Current, true>();
            assert(val != 0xff);
            break;
          default:
            assert("!Unknown vertex attribute format.");
            throw "";
            break;
          }

          prim.mVertices[vi][(gx::VertexAttribute)a] = val;

          if (optUsageMap &&
              (*optUsageMap)[(gx::VertexBufferAttribute)a] <= val)
            (*optUsageMap)[(gx::VertexBufferAttribute)a] = val;
        }
      }
    }
  }
}

} // namespace libcube
