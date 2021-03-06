/*!
 * @file
 * @brief TODO
 */

#pragma once

#include <array>
#include <iterator>
#include <string>
#include <tuple>

#include <vendor/glm/mat4x4.hpp>
#include <vendor/glm/vec3.hpp>
#include <vendor/glm/vec4.hpp>

#include <plugins/gc/Export/Material.hpp>
#include <plugins/gc/GX/VertexTypes.hpp>

typedef int TODO;

namespace libcube {

struct LightingChannelControl {
  gx::ChannelControl colorChannel;
  gx::ChannelControl alphaChannel;
};

//! @brief Based on noclip's implementation.
//!
struct GXMaterial {
  int index = -1;   //!<
  std::string name; //!< For debugging.

  IGCMaterial& mat;

  bool usePnMtxIdx = true;
  bool useTexMtxIdx[16]{false};
  bool hasPostTexMtxBlock = false;
  bool hasLightsBlock = true;

  inline u32 calcParamsBlockSize() const noexcept {
    u32 size = 4 * 2 + 4 * 2 + 4 * 4 + 4 * 4 + 4 * 3 * 10 + 4 * 2 * 3 + 4 * 8;
    if (hasPostTexMtxBlock)
      size += 4 * 3 * 20;
    if (hasLightsBlock)
      size += 4 * 5 * 8;

    return size;
  }
};

static glm::vec4 scratchVec4;

//----------------------------------
// Lighting
struct Light {
  glm::vec4 Position = {0, 0, 0, 0};
  glm::vec4 Direction = {0, 0, -1, 0};
  glm::vec4 DistAtten = {0, 0, 0, 0};
  glm::vec4 CosAtten = {0, 0, 0, 0};
  glm::vec4 Color = {1, 1, 1, 1};

  void setWorldPositionViewMatrix(const glm::mat4& viewMatrix, float x, float y,
                                  float z) {
    Position = glm::vec4{x, y, z, 1.0f} * viewMatrix;
  }

  void setWorldDirectionNormalMatrix(const glm::mat4& normalMatrix, float x,
                                     float y, float z) {
    Direction = glm::normalize(glm::vec4(x, y, z, 0.0f) * normalMatrix);
  }
};

//----------------------------------
// Uniforms
enum class UniformStorage {
  UINT,
  VEC2,
  VEC3,
  VEC4,
};

//----------------------------------
// Vertex attribute generation
struct VertexAttributeGenDef {
  gx::VertexAttribute attrib;
  const char* name;

  u32 format;
  u32 size;
};
static_assert(sizeof(VertexAttributeGenDef) == sizeof(VAOEntry));

std::pair<const VertexAttributeGenDef&, std::size_t>
getVertexAttribGenDef(gx::VertexAttribute vtxAttrib);
int getVertexAttribLocation(gx::VertexAttribute vtxAttrib);

std::string generateBindingsDefinition(bool postTexMtxBlock = false,
                                       bool lightsBlock = false);

extern const std::array<VertexAttributeGenDef, 15> vtxAttributeGenDefs;

struct Mat42 {
  glm::vec4 a, b;
};
struct Mat43 {
  glm::vec4 a, b, c;
};
// ROW-MAJOR
struct UniformMaterialParams {
  std::array<glm::vec4, 2> ColorMatRegs;
  std::array<glm::vec4, 2> ColorAmbRegs;
  std::array<glm::vec4, 4> KonstColor;
  std::array<glm::vec4, 4> Color;
  std::array<glm::mat3x4, 10> TexMtx; // 4x3
  // sizex, sizey, 0, bias
  std::array<glm::vec4, 8> TexParams;
  std::array<glm::mat2x4, 3> IndTexMtx; // 2x3
  // Optional: Not optional for now.
  Light u_LightParams[8];
};
static_assert(sizeof(UniformMaterialParams) == 1536, "Bad sized UParam");

static_assert(sizeof(UniformMaterialParams) ==
              896 + sizeof(std::array<Light, 8>));
constexpr u32 ub = sizeof(UniformMaterialParams);
} // namespace libcube
