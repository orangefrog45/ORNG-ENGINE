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

		void Load(const std::string& filepath, unsigned cubemap_resolution, bool using_ibl);

		const TextureCubemap& GetSkyboxTexture() const noexcept {
			return m_skybox_tex;
		}

		const std::unique_ptr<TextureCubemap>& GetIrradianceTexture() const noexcept {
			return m_diffuse_prefilter_map;
		}

		const std::unique_ptr<TextureCubemap>& GetSpecularPrefilter() const noexcept {
			return m_specular_prefilter_map;
		}

		const std::string& GetSrcFilepath() const noexcept {
			return m_hdr_tex_filepath;
		}

		// If false LoadEnvironmentMap will only load a cubemap image but no IBL textures.
		bool using_env_map = false;
	private:
		std::string m_hdr_tex_filepath = "";
		unsigned int m_resolution = 4096;

		std::unique_ptr<TextureCubemap> m_diffuse_prefilter_map{ nullptr };
		std::unique_ptr<TextureCubemap> m_specular_prefilter_map{ nullptr };

		TextureCubemap m_skybox_tex{ "env_skybox" };
	};
}