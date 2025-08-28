#pragma once
#include "rendering/Textures.h"

namespace ORNG {
	class Framebuffer;
	class Shader;

	class Skybox {
		friend class SceneSerializer;
		friend class EditorLayer;
		friend class EnvMapLoader;
		friend class EnvMapSystem;
	public:
		Skybox() = default;

		[[nodiscard]] const TextureCubemap& GetSkyboxTexture() const noexcept {
			return m_skybox_tex;
		}

		[[nodiscard]] const std::unique_ptr<TextureCubemap>& GetIrradianceTexture() const noexcept {
			return m_diffuse_prefilter_map;
		}

		[[nodiscard]] const std::unique_ptr<TextureCubemap>& GetSpecularPrefilter() const noexcept {
			return m_specular_prefilter_map;
		}

		[[nodiscard]] const std::string& GetSrcFilepath() const noexcept {
			return m_hdr_tex_filepath;
		}

		[[nodiscard]] int GetResolution() const noexcept {
			return m_resolution;
		}

		// If false LoadEnvironmentMap will only load a cubemap image but no IBL textures.
		bool using_env_map = false;
	private:
		std::string m_hdr_tex_filepath = "";
		int m_resolution = 4096;

		std::unique_ptr<TextureCubemap> m_diffuse_prefilter_map{ nullptr };
		std::unique_ptr<TextureCubemap> m_specular_prefilter_map{ nullptr };

		TextureCubemap m_skybox_tex{ "env_skybox" };
	};
}
