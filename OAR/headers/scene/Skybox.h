#pragma once
#include "rendering/Textures.h"

namespace ORNG {
	class Framebuffer;
	class Shader;


	class Skybox {
		friend class SceneSerializer;
	public:
		friend class EditorLayer;
		friend class EnvMapLoader;
		Skybox() = default;
		void LoadEnvironmentMap(const std::string& filepath);
		const TextureCubemap& GetSkyboxTexture() const;
		const TextureCubemap& GetIrradianceTexture() const;
		const TextureCubemap& GetSpecularPrefilter() {
			return m_specular_prefilter_map;
		}
	private:
		std::string m_hdr_tex_filepath = "";
		unsigned int m_resolution = 4096;
		Texture2D m_brdf_convolution_lut{ "env_brdf_convolution" };
		TextureCubemap m_diffuse_prefilter_map{ "env_diffuse_prefilter" };
		TextureCubemap m_specular_prefilter_map{ "env_specular_prefilter" };
		TextureCubemap m_skybox_tex{ "env_skybox" };
	};
}