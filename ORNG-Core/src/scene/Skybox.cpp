#include "pch/pch.h"
#include "scene/Skybox.h"
#include "rendering/EnvMapLoader.h"

namespace ORNG {


	void Skybox::Load(const std::string& filepath, unsigned cubemap_resolution, bool using_ibl) {
		EnvMapLoader loader{};

		loader.LoadSkybox(filepath, *this, cubemap_resolution, using_ibl);
		m_hdr_tex_filepath = filepath;
		using_env_map = using_ibl;
		m_resolution = cubemap_resolution;

		if (!using_ibl) {
			m_diffuse_prefilter_map = nullptr;
			m_specular_prefilter_map = nullptr;
		}
	}

}