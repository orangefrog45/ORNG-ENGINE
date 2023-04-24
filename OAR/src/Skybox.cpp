#include "pch/pch.h"

#include "rendering/Skybox.h"
#include "util/util.h"

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
		"res/textures/kurt/mountain/posx.jpg",
		"res/textures/kurt/mountain/negx.jpg",
		"res/textures/kurt/mountain/posy.jpg",
		"res/textures/kurt/mountain/negy.jpg",
		"res/textures/kurt/mountain/posz.jpg",
		"res/textures/kurt/mountain/negz.jpg",
	};

	spec.generate_mipmaps = false;
	spec.internal_format = GL_SRGB8;
	spec.format = GL_RGB;
	spec.wrap_params = GL_CLAMP_TO_EDGE;

	m_cubemap_texture.SetSpec(spec);
	m_cubemap_texture.Load();


	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), &m_vertices, GL_STATIC_DRAW));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
}
