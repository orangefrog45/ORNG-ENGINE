#include <glew.h>
#include <iostream>
#include <execution>
#include <glm/gtx/matrix_major_storage.hpp>
#include "MeshLibrary.h"

void MeshLibrary::RenderBasicShaderMeshes(const WorldData& data) {
	shaderLibrary.basic_shader.ActivateProgram();
	shaderLibrary.basic_shader.SetProjection(glm::colMajor4(data.projectionMatrix));
	shaderLibrary.basic_shader.SetCamera(glm::colMajor4(data.cameraMatrix));


	for (BasicMesh& mesh : basicShaderMeshes) {

		mesh.UpdateTransformBuffers(data);
		mesh.Render();
	}
}

void MeshLibrary::Init() {
	shaderLibrary.Init();
}

void MeshLibrary::RenderLightingShaderMeshes(const WorldData& data) {

	shaderLibrary.lighting_shader.ActivateProgram();
	shaderLibrary.lighting_shader.SetProjection(glm::colMajor4(data.projectionMatrix));
	shaderLibrary.lighting_shader.SetCamera(glm::colMajor4(data.cameraMatrix));

	BaseLight base_light = BaseLight();
	if (lightColor.x > 1.0f || lightColor.x < 0.0f) {
		deltaX *= -1.0f;
	}
	if (lightColor.y > 1.0f || lightColor.y < 0.0f) {
		deltaY *= -1.0f;
	}
	if (lightColor.z > 1.0f || lightColor.z < 0.0f) {
		deltaZ *= -1.0f;
	}
	lightColor = glm::fvec3(lightColor.x + deltaX, lightColor.y + deltaY, lightColor.z + deltaZ);
	base_light.color = lightColor;
	base_light.ambient_intensity = 1.0f;
	shaderLibrary.lighting_shader.SetTextureUnit(GL_TEXTURE0);
	shaderLibrary.lighting_shader.SetLight(base_light);

	for (BasicMesh& mesh : lightingShaderMeshes) {

		mesh.UpdateTransformBuffers(data);
		mesh.Render();
	}
}

void MeshLibrary::RenderAllMeshes(const WorldData& data) {
	RenderBasicShaderMeshes(data);
	RenderLightingShaderMeshes(data);
}
