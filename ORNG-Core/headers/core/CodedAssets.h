#pragma once
#define STRINGIFY(...) #__VA_ARGS__
#define STR(...) STRINGIFY(__VA_ARGS__)
#include "rendering/MeshAsset.h"
#include "core/GLStateManager.h"

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
				{0.5, 0.5, -0.5},
				{-0.5, 0.5, -0.5},
				{-0.5, 0.5, 0.5},
				{0.5, 0.5, 0.5},
				{0.5, -0.5, 0.5},
				{0.5, 0.5, 0.5},
				{-0.5, 0.5, 0.5},
				{-0.5, -0.5, 0.5},
				{-0.5, -0.5, 0.5},
				{-0.5, 0.5, 0.5},
				{-0.5, 0.5, -0.5},
				{-0.5, -0.5, -0.5},
				{-0.5, -0.5, -0.5},
				{0.5, -0.5, -0.5},
				{0.5, -0.5, 0.5},
				{-0.5, -0.5, 0.5},
				{0.5, -0.5, -0.5},
				{0.5, 0.5, -0.5},
				{0.5, 0.5, 0.5},
				{0.5, -0.5, 0.5},
				{-0.5, -0.5, -0.5},
				{-0.5, 0.5, -0.5},
				{0.5, 0.5, -0.5},
				{0.5, -0.5, -0.5}
			};


			m_cube_mesh->m_vao.vertex_data.normals = {
				{-0, 1, -0},
				{-0, 1, -0},
				{-0, 1, -0},
				{-0, 1, -0},
				{-0, -0, 1},
				{-0, -0, 1},
				{-0, -0, 1},
				{-0, -0, 1},
				{-1, -0, -0},
				{-1, -0, -0},
				{-1, -0, -0},
				{-1, -0, -0},
				{-0, -1, -0},
				{-0, -1, -0},
				{-0, -1, -0},
				{-0, -1, -0},
				{1, -0, -0},
				{1, -0, -0},
				{1, -0, -0},
				{1, -0, -0},
				{-0, -0, -1},
				{-0, -0, -1},
				{-0, -0, -1},
				{-0, -0, -1},
			};

			m_cube_mesh->m_vao.vertex_data.tangents = {
				{-1, 0, 0},
				{-1, 0, 0},
				{-1, 0, 0},
				{-1, 0, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{1, 0, 0},
				{1, 0, 0},
				{1, 0, 0},
				{1, 0, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},
				{0, 1, 0},

			};

			m_cube_mesh->m_vao.vertex_data.tex_coords = {
				{0.625, 0.5},
				{0.875, 0.5},
				{0.875, 0.75},
				{0.625, 0.75},
				{0.375, 0.75},
				{0.625, 0.75},
				{0.625, 1},
				{0.375, 1},
				{0.375, 0},
				{0.625, 0},
				{0.625, 0.25},
				{0.375, 0.25},
				{0.125, 0.5},
				{0.375, 0.5},
				{0.375, 0.75},
				{0.125, 0.75},
				{0.375, 0.5},
				{0.625, 0.5},
				{0.625, 0.75},
				{0.375, 0.75},
				{0.375, 0.25},
				{0.625, 0.25},
				{0.625, 0.5},
				{0.375, 0.5},
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
			m_cube_mesh->m_original_materials.push_back(*m_base_material);
			m_cube_mesh->m_scene_materials.push_back(&m_cube_mesh->m_original_materials[0]);
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
		inline static std::unique_ptr<Material> m_base_material;
		inline static std::unique_ptr<MeshAsset> m_cube_mesh = nullptr;
		inline static std::unique_ptr<Texture2D> m_base_tex = nullptr;
	};

}
