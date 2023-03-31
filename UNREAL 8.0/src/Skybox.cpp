#include <glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include "Texture2D.h"
#include "Skybox.h"
#include "shaders/SkyboxShader.h"
#include "RendererResources.h"

void Skybox::Init() {
	/*faces.push_back("res/textures/kurt/mountain/posx.jpg");
	faces.push_back("res/textures/kurt/mountain/negx.jpg");
	faces.push_back("res/textures/kurt/mountain/posy.jpg");
	faces.push_back("res/textures/kurt/mountain/negy.jpg");
	faces.push_back("res/textures/kurt/mountain/posz.jpg");
	faces.push_back("res/textures/kurt/mountain/negz.jpg");*/

	faces.push_back("res/textures/clouds1_east.bmp");
	faces.push_back("res/textures/clouds1_west.bmp");
	faces.push_back("res/textures/clouds1_up.bmp");
	faces.push_back("res/textures/clouds1_down.bmp");
	faces.push_back("res/textures/clouds1_north.bmp");
	faces.push_back("res/textures/clouds1_south.bmp");
	cubemap_texture.SetFaces(faces);
	cubemap_texture.Load();

	/*
	--------------------------------------------------------------

	faces.push_back("res/textures/kurt/xpos.png");
	faces.push_back("res/textures/kurt/xneg.png");
	faces.push_back("res/textures/kurt/ypos.png");
	faces.push_back("res/textures/kurt/yneg.png");
	faces.push_back("res/textures/kurt/zpos.png");
	faces.push_back("res/textures/kurt/zneg.png");
	*/

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), &m_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
}
