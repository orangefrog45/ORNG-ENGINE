#include <glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include "Texture.h"
#include "Skybox.h"
#include "shaders/SkyboxShader.h"
#include "RendererData.h"

void Skybox::Init() {
	faces.push_back("res/textures/kurt/mountain/posx.jpg");
	faces.push_back("res/textures/kurt/mountain/negx.jpg");
	faces.push_back("res/textures/kurt/mountain/posy.jpg");
	faces.push_back("res/textures/kurt/mountain/negy.jpg");
	faces.push_back("res/textures/kurt/mountain/posz.jpg");
	faces.push_back("res/textures/kurt/mountain/negz.jpg");
	cubemapTexture = Texture::LoadCubeMap(faces);

	/*	faces.push_back("res/textures/clouds1_east.bmp");
	faces.push_back("res/textures/clouds1_west.bmp");
	faces.push_back("res/textures/clouds1_up.bmp");
	faces.push_back("res/textures/clouds1_down.bmp");
	faces.push_back("res/textures/clouds1_north.bmp");
	faces.push_back("res/textures/clouds1_south.bmp");

	--------------------------------------------------------------

	faces.push_back("res/textures/kurt/xpos.png");
	faces.push_back("res/textures/kurt/xneg.png");
	faces.push_back("res/textures/kurt/ypos.png");
	faces.push_back("res/textures/kurt/yneg.png");
	faces.push_back("res/textures/kurt/zpos.png");
	faces.push_back("res/textures/kurt/zneg.png");
	*/

	skyboxShader.Init();
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
}

void Skybox::Draw(const glm::fmat3& view) {
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	skyboxShader.ActivateProgram();
	glActiveTexture(RendererData::TextureUnits::COLOR);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glUniformMatrix4fv(skyboxShader.GetViewLoc(), 1, GL_TRUE, &glm::fmat4(view)[0][0]);
	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}
