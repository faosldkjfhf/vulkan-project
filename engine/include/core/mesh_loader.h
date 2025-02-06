#pragma once

#include "gpu/gpu_mesh_buffers.h"
#include "pch.h"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include "engine.h"

namespace bisky {

class Engine;

struct GeoSurface {
  uint32_t startIndex;
  uint32_t count;
};

struct MeshAsset {
  std::string name;

  Vector<GeoSurface> surfaces;
  GPUMeshBuffers meshBuffers;
};

namespace core {

class MeshLoader {
public:
  MeshLoader();
  ~MeshLoader();

  static std::optional<Vector<Pointer<MeshAsset>>> loadGltfMeshes(Engine *engine, std::filesystem::path filePath) {
    fastgltf::GltfDataBuffer data;
    data.FromPath(filePath);

    constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser;

    auto load = parser.loadGltfBinary(data, filePath.parent_path(), gltfOptions);
    if (load) {
      gltf = std::move(load.get());
    } else {
      return {};
    }

    Vector<Pointer<MeshAsset>> meshes;
    Vector<Vertex> vertices;
    Vector<uint32_t> indices;
    for (fastgltf::Mesh &mesh : gltf.meshes) {
      MeshAsset newMesh;

      newMesh.name = mesh.name;

      indices.clear();
      vertices.clear();

      for (auto &&p : mesh.primitives) {
        GeoSurface surface;
        surface.startIndex = (uint32_t)indices.size();
        surface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

        size_t initialVertex = vertices.size();

        {
          fastgltf::Accessor &indexAccessor = gltf.accessors[p.indicesAccessor.value()];
          indices.reserve(indices.size() + indexAccessor.count);
          fastgltf::iterateAccessor<uint32_t>(gltf, indexAccessor,
                                              [&](uint32_t idx) { indices.push_back(idx + initialVertex); });
        }
        {
          fastgltf::Accessor &posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
          vertices.resize(vertices.size() + posAccessor.count);
          fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index) {
            Vertex vtx;
            vtx.position = v;
            vtx.normal = {1, 0, 0};
            vtx.color = glm::vec4(1.0f);
            vtx.uv_x = 0;
            vtx.uv_y = 0;
            vertices[initialVertex + index] = vtx;
          });
        }

        auto normals = p.findAttribute("NORMAL");
        if (normals != p.attributes.end()) {

          fastgltf::iterateAccessorWithIndex<glm::vec3>(
              gltf, gltf.accessors[(*normals).accessorIndex],
              [&](glm::vec3 v, size_t index) { vertices[initialVertex + index].normal = v; });
        }

        auto uv = p.findAttribute("TEXCOORD_0");
        if (uv != p.attributes.end()) {

          fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex],
                                                        [&](glm::vec2 v, size_t index) {
                                                          vertices[initialVertex + index].uv_x = v.x;
                                                          vertices[initialVertex + index].uv_y = v.y;
                                                        });
        }

        auto colors = p.findAttribute("COLOR_0");
        if (colors != p.attributes.end()) {

          fastgltf::iterateAccessorWithIndex<glm::vec4>(
              gltf, gltf.accessors[(*colors).accessorIndex],
              [&](glm::vec4 v, size_t index) { vertices[initialVertex + index].color = v; });
        }

        newMesh.surfaces.push_back(surface);
      }

      constexpr bool overrideColors = true;
      if (overrideColors) {
        for (Vertex &vtx : vertices) {
          vtx.color = glm::vec4(vtx.normal, 1.0f);
        }
      }

      newMesh.meshBuffers = engine->uploadMesh(indices, vertices);
      meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
    }

    return meshes;
  }

private:
};

} // namespace core
} // namespace bisky
