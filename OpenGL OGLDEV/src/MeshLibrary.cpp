#include <glew.h>
#include <iostream>
#include "MeshLibrary.h"

void MeshLibrary::RenderBasicShaderMeshes(const WorldData& data) {
	shaderLibrary.basicShader.ActivateProgram();
	shaderLibrary.basicShader.InitUniforms();

	for (unsigned int i = 0; i < basicShaderMeshes.size(); i++) {
		glm::fmat4 WVP = basicShaderMeshes[i].GetWorldTransform().GetMatrix() * data.cameraMatrix * data.projectionMatrix;
		glUniformMatrix4fv(shaderLibrary.basicShader.GetWVPLocation(), 1, GL_TRUE, &WVP[0][0]);
		basicShaderMeshes[i].Render();
	}
}

void MeshLibrary::Init() {
	shaderLibrary.Init();
}


void MeshLibrary::RenderTestShaderMeshes() {
	for (unsigned int i = 0; i < testShaderMeshes.size(); i++) {
		testShaderMeshes[i].Render();
	}
}
void MeshLibrary::RenderLightingShaderMeshes() {
	for (unsigned int i = 0; i < lightingShaderMeshes.size(); i++) {
		lightingShaderMeshes[i].Render();
	}
}

void MeshLibrary::RenderAllMeshes(const WorldData& data) {
	RenderBasicShaderMeshes(data);
	RenderTestShaderMeshes();
	RenderLightingShaderMeshes();
}
