#pragma once
#define STRINGIFY(...) #__VA_ARGS__
#define STR(...) STRINGIFY(__VA_ARGS__)
#include "rendering/MeshAsset.h"
#include "core/GLStateManager.h"
#include "rendering/Textures.h"

#define ORNG_BASE_MATERIAL_UUID 0
#define ORNG_BASE_TEXTURE_UUID 0
#define ORNG_CUBE_MESH_UUID 0

namespace ORNG {
	class CodedAssets { // Contains resources that need to be hard-coded into the library.
	public:
		// Returns 1x1 cube
		static MeshAsset& GetCubeAsset() {
			return *m_cube_mesh;
		}

		// Returns solid white 1x1 RGB texture
		static Texture2D& GetBaseTexture() {
			return *m_base_tex;
		}

		static const Material& GetBaseMaterial() {
			return *m_base_material;
		}

		static void Init() {
			m_cube_mesh = std::make_unique<MeshAsset>("CodedCube");
			m_cube_mesh->m_vao.vertex_data.positions = {
				0.5, 0.5, -0.5,
				-0.5, 0.5, -0.5,
				-0.5, 0.5, 0.5,
				0.5, 0.5, 0.5,
				0.5, -0.5, 0.5,
				0.5, 0.5, 0.5,
				-0.5, 0.5, 0.5,
				-0.5, -0.5, 0.5,
				-0.5, -0.5, 0.5,
				-0.5, 0.5, 0.5,
				-0.5, 0.5, -0.5,
				-0.5, -0.5, -0.5,
				-0.5, -0.5, -0.5,
				0.5, -0.5, -0.5,
				0.5, -0.5, 0.5,
				-0.5, -0.5, 0.5,
				0.5, -0.5, -0.5,
				0.5, 0.5, -0.5,
				0.5, 0.5, 0.5,
				0.5, -0.5, 0.5,
				-0.5, -0.5, -0.5,
				-0.5, 0.5, -0.5,
				0.5, 0.5, -0.5,
				0.5, -0.5, -0.5,
			};


			m_cube_mesh->m_vao.vertex_data.normals = {
				-0, 1, -0,
				-0, 1, -0,
				-0, 1, -0,
				-0, 1, -0,
				-0, -0, 1,
				-0, -0, 1,
				-0, -0, 1,
				-0, -0, 1,
				-1, -0, -0,
				-1, -0, -0,
				-1, -0, -0,
				-1, -0, -0,
				-0, -1, -0,
				-0, -1, -0,
				-0, -1, -0,
				-0, -1, -0,
				1, -0, -0,
				1, -0, -0,
				1, -0, -0,
				1, -0, -0,
				-0, -0, -1,
				-0, -0, -1,
				-0, -0, -1,
				-0, -0, -1,
			};

			m_cube_mesh->m_vao.vertex_data.tangents = {
				-1, 0, 0,
				-1, 0, 0,
				-1, 0, 0,
				-1, 0, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				1, 0, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,
				0, 1, 0,

			};

			m_cube_mesh->m_vao.vertex_data.tex_coords = {
				1.0, 0.0, // Vertex 0
				1.0, 1.0, // Vertex 1
				0.0, 1.0, // Vertex 2
				0.0, 0.0, // Vertex 3

				1.0, 0.0, // Vertex 4
				1.0, 1.0, // Vertex 5
				0.0, 1.0, // Vertex 6
				0.0, 0.0, // Vertex 7

				1.0, 0.0, // Vertex 8
				1.0, 1.0, // Vertex 9
				0.0, 1.0, // Vertex 10
				0.0, 0.0, // Vertex 11

				1.0, 0.0, // Vertex 12
				1.0, 1.0, // Vertex 13
				0.0, 1.0, // Vertex 14
				0.0, 0.0, // Vertex 15

				1.0, 0.0, // Vertex 16
				1.0, 1.0, // Vertex 17
				0.0, 1.0, // Vertex 18
				0.0, 0.0, // Vertex 19

				1.0, 0.0, // Vertex 20
				1.0, 1.0, // Vertex 21
				0.0, 1.0, // Vertex 22
				0.0, 0.0  // Vertex 23
			};


			m_cube_mesh->m_vao.vertex_data.indices = {
				// Indices for each face (two triangles per face)
				0
				,1
				,2
				,0
				,2
				,3
				,4
				,5
				,6
				,4
				,6
				,7
				,8
				,9
				,10
				,8
				,10
				,11
				,12
				,13
				,14
				,12
				,14
				,15
				,16
				,17
				,18
				,16
				,18
				,19
				,20
				,21
				,22
				,20
				,22
				,23
			};
			m_cube_mesh->uuid = UUID(ORNG_CUBE_MESH_UUID);
			m_cube_mesh->m_vao.FillBuffers();
			m_cube_mesh->m_aabb.max = { 0.5, 0.5, 0.5 };
			m_cube_mesh->m_aabb.min = { -0.5, -0.5, -0.5 };
			MeshAsset::MeshEntry entry;
			entry.base_index = 0;
			entry.base_vertex = 0;
			entry.material_index = 0;
			entry.num_indices = m_cube_mesh->m_vao.vertex_data.indices.size();
			m_cube_mesh->m_submeshes.push_back(entry);

			m_base_tex = std::make_unique<Texture2D>("Base coded texture", 0);
			m_base_tex->uuid = UUID(ORNG_BASE_TEXTURE_UUID);
			Texture2DSpec spec;
			spec.format = GL_RGB;
			spec.internal_format = GL_RGB8;
			spec.srgb_space = true;
			spec.width = 1;
			spec.height = 1;
			spec.wrap_params = GL_CLAMP_TO_EDGE;
			spec.min_filter = GL_NEAREST;
			spec.mag_filter = GL_NEAREST;
			m_base_tex->SetSpec(spec);
			GL_StateManager::BindTexture(GL_TEXTURE_2D, m_base_tex->GetTextureHandle(), GL_TEXTURE0);
			unsigned char white_pixel[] = { 255, 255, 255, 255 };
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white_pixel);

			m_base_material = std::make_unique<Material>(&*m_base_tex);
			m_base_material->name = "NONE";
			m_base_material->uuid = UUID(ORNG_BASE_MATERIAL_UUID);
			m_cube_mesh->m_material_assets.push_back(&*m_base_material);
			m_cube_mesh->m_is_loaded = true;
		}


		inline static const char* GBufferVS = {
			#include "../res/shaders/GBufferVS.glsl"
		};

		inline static const char* BloomDownsampleCS = {
			#include "../res/shaders/BloomDownsampleCS.glsl"
		};

		inline static const char* BloomThresholdCS = {
			#include "../res/shaders/BloomThresholdCS.glsl"
		};

		inline static const char* BloomUpsampleCS = {
			#include "../res/shaders/BloomUpsampleCS.glsl"
		};

		inline static const char* BlurFS = {
			#include "../res/shaders/BlurFS.glsl"
		};

		inline static const char* BRDFConvolutionFS = {
			#include "../res/shaders/BRDFConvolutionFS.glsl"
		};


		inline static const char* CubemapShadowFS = {
		#include "../res/shaders/CubemapShadowFS.glsl"
		};

		inline static const char* CubemapShadowVS = {
		#include "../res/shaders/CubemapShadowVS.glsl"
		};

		inline static const char* CubemapVS = {
		#include "../res/shaders/CubemapVS.glsl"
		};

		inline static const char* DepthFS = {
		#include "../res/shaders/DepthFS.glsl"
		};

		inline static const char* DepthVS = {
		#include "../res/shaders/DepthVS.glsl"
		};

		inline static const char* FogCS = {
		#include "../res/shaders/FogCS.glsl"
		};

		inline static const char* GBufferFS = {
		#include "../res/shaders/GBufferFS.glsl"
		};


		inline static const char* HDR_ToCubemapFS = {
		#include "../res/shaders/HDR_ToCubemapFS.glsl"
		};

		inline static const char* LightingCS = {
		#include "../res/shaders/LightingCS.glsl"
		};

		inline static const char* PostProcessCS = {
		#include "../res/shaders/PostProcessCS.glsl"
		};

		inline static const char* PortalCS = {
		#include "../res/shaders/PortalCS.glsl"
		};

		inline static const char* QuadFS = {
		#include "../res/shaders/QuadFS.glsl"
		};

		inline static const char* QuadVS = {
		#include "../res/shaders/QuadVS.glsl"
		};

		inline static const char* ReflectionFS = {
		#include "../res/shaders/ReflectionFS.glsl"
		};

		inline static const char* ReflectionVS = {
		#include "../res/shaders/ReflectionVS.glsl"
		};

		inline static const char* SkyboxDiffusePrefilterFS = {
		#include "../res/shaders/SkyboxDiffusePrefilterFS.glsl"
		};


		inline static const char* SkyboxSpecularPrefilterFS = {
		#include "../res/shaders/SkyboxSpecularPrefilterFS.glsl"
		};


		inline static const char* TransformVS = {
		#include "../res/shaders/TransformVS.glsl"
		};


	private:
		// Material used as a replacement for any meshes without materials.
		inline static std::unique_ptr<Material> m_base_material;
		// Cube mesh for rendering cube maps, also used as a replacement for missing meshes
		inline static std::unique_ptr<MeshAsset> m_cube_mesh = nullptr;
		// White pixel texture as a replacement for any missing textures during runtime
		inline static std::unique_ptr<Texture2D> m_base_tex = nullptr;
	};

}
