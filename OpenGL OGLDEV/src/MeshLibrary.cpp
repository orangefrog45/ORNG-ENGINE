#include <glew.h>
#include <iostream>
#include <execution>
#include <glm/gtx/matrix_major_storage.hpp>
#include "MeshLibrary.h"

void MeshLibrary::RenderBasicShaderMeshes(const WorldData& data) {
	shaderLibrary.basicShader.ActivateProgram();


	for (BasicMesh& mesh : basicShaderMeshes) {
		mesh.UpdateTransformBuffers(data);
		mesh.Render();
	}
}

void MeshLibrary::Init() {
	shaderLibrary.Init();
}

void MeshLibrary::RenderLightingShaderMeshes() {
	/*for (BasicMesh& mesh : lightingShaderMeshes) {
		mesh.Render();
	}*/
}

void MeshLibrary::RenderAllMeshes(const WorldData& data) {
	RenderBasicShaderMeshes(data);
	RenderLightingShaderMeshes();
}
