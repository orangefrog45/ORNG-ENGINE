#pragma once
#include "ShaderLibrary.h"
#include "BasicMesh.h"
#include "WorldData.h"

class MeshLibrary {
public:
	void Init();
	std::vector<BasicMesh> basicShaderMeshes;
	std::vector<BasicMesh> lightingShaderMeshes;

	void RenderBasicShaderMeshes(const WorldData& data);
	void RenderLightingShaderMeshes();
	void RenderAllMeshes(const WorldData& data);

	ShaderLibrary shaderLibrary;

	void LoadMeshes();
};