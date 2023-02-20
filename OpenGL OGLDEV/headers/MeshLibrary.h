#pragma once
#include "ShaderLibrary.h"
#include "BasicMesh.h"
#include "WorldData.h"

class MeshLibrary {
public:
	MeshLibrary() {};
	void Init();
	std::vector<BasicMesh> basicShaderMeshes;
	std::vector<BasicMesh> lightingShaderMeshes;

	glm::fvec3 lightColor = glm::fvec3(1.0f, 0.0f, 0.5f);
	float deltaX = 0.01f;
	float deltaY = 0.01f;
	float deltaZ = 0.01f;

	void RenderBasicShaderMeshes(const WorldData& data);
	void RenderLightingShaderMeshes(const WorldData& data);
	void RenderAllMeshes(const WorldData& data);
private:

	ShaderLibrary shaderLibrary;

};