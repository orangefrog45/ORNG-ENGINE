#pragma once
#include "rendering/Textures.h"

namespace ORNG {
	class Framebuffer;
	class Shader;


	class Skybox {
	public:
		friend class EditorLayer;
		Skybox() = default;
		static void Init();
		void LoadEnvironmentMap(const std::string& filepath);
		const TextureCubemap& GetSkyboxTexture() const;
		const TextureCubemap& GetIrradianceTexture() const;
	private:
		unsigned int m_resolution = 4096;
		inline static Framebuffer* mp_output_fb = nullptr;
		inline static Shader* mp_hdr_converter_shader = nullptr;
		inline static Shader* mp_brdf_convolution_shader = nullptr;
		inline static Shader* mp_specular_prefilter_shader = nullptr;
		inline static Shader* mp_diffuse_prefilter_shader = nullptr;
		inline static bool ms_is_initialized = false;
		Texture2D m_brdf_convolution_lut{ "env_brdf_convolution" };
		TextureCubemap m_diffuse_prefilter_map{ "env_diffuse_prefilter" };
		TextureCubemap m_specular_prefilter_map{ "env_specular_prefilter" };
		TextureCubemap m_skybox_tex{ "env_skybox" };
	};
}