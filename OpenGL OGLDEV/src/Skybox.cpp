#include <glew.h>
#include <freeglut.h>
#include <glm/glm.hpp>
#include "Texture.h"
#include "Skybox.h"
#include "shaders/SkyboxShader.h"

void Skybox::Init() {
	faces.push_back("res/textures/clouds1_east.bmp");
	faces.push_back("res/textures/clouds1_west.bmp");
	faces.push_back("res/textures/clouds1_up.bmp");
	faces.push_back("res/textures/clouds1_down.bmp");
	faces.push_back("res/textures/clouds1_north.bmp");
	faces.push_back("res/textures/clouds1_south.bmp");
	cubemapTexture = Texture::LoadCubeMap(faces);

	skyboxShader.Init();
	skyboxShader.ActivateProgram();
	skyboxShader.InitUniforms();

	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
}

void Skybox::Draw(const glm::fmat4& WVP) {
	skyboxShader.ActivateProgram();
	glUniformMatrix4fv(skyboxShader.GetWVPLocation(), 1, GL_TRUE, &WVP[0][0]);
	glDepthFunc(GL_LEQUAL);
	glBindVertexArray(skyboxVAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}
