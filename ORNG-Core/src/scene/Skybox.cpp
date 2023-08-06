#include "pch/pch.h"
#include "scene/Skybox.h"
#include "rendering/Renderer.h"
#include "util/util.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "stb/stb_image_write.h"
#include "rendering/EnvMapLoader.h"

namespace ORNG {


	void Skybox::LoadEnvironmentMap(const std::string& filepath) {
		EnvMapLoader::LoadEnvironmentMap(filepath, *this, 4096);
		m_hdr_tex_filepath = filepath;
	}

	const TextureCubemap& Skybox::GetSkyboxTexture() const {
		return m_skybox_tex;
	}

	const TextureCubemap& Skybox::GetIrradianceTexture() const {
		return m_diffuse_prefilter_map;
	}
}