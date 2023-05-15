#include "pch/pch.h"

#include "scene/Skybox.h"
#include "util/util.h"

namespace ORNG {

	void Skybox::Init() {
		/*faces.push_back("res/textures/kurt/mountain/posx.jpg");
		faces.push_back("res/textures/kurt/mountain/negx.jpg");
		faces.push_back("res/textures/kurt/mountain/posy.jpg");
		faces.push_back("res/textures/kurt/mountain/negy.jpg");
		faces.push_back("res/textures/kurt/mountain/posz.jpg");
		faces.push_back("res/textures/kurt/mountain/negz.jpg");
		/*faces.push_back("res/textures/skybox/right.png");
		faces.push_back("res/textures/skybox/left.png");
		faces.push_back("res/textures/skybox/top.png");
		faces.push_back("res/textures/skybox/bottom.png");
		faces.push_back("res/textures/skybox/front.png");
		faces.push_back("res/textures/skybox/back.png");

		/*faces.push_back("res/textures/kurt/xpos.png");
		faces.push_back("res/textures/kurt/xneg.png");
		faces.push_back("res/textures/kurt/ypos.png");
		faces.push_back("res/textures/kurt/yneg.png");
		faces.push_back("res/textures/kurt/zpos.png");
		faces.push_back("res/textures/kurt/zneg.png");*/


		TextureCubemapSpec spec;

		spec.filepaths = {
			"res/textures/clouds1_east.bmp",
			"res/textures/clouds1_west.bmp",
			"res/textures/clouds1_up.bmp",
			"res/textures/clouds1_down.bmp",
			"res/textures/clouds1_north.bmp",
			"res/textures/clouds1_south.bmp",
		};

		spec.generate_mipmaps = false;
		spec.internal_format = GL_SRGB8;
		spec.format = GL_RGB;
		spec.min_filter = GL_LINEAR;
		spec.mag_filter = GL_LINEAR;
		spec.wrap_params = GL_CLAMP_TO_EDGE;
		spec.storage_type = GL_UNSIGNED_BYTE;

		m_cubemap_texture.SetSpec(spec);
		m_cubemap_texture.LoadFromFile();
	}
}