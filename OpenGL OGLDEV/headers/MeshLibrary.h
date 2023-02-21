#pragma once
#include "ShaderLibrary.h"
#include "BasicMesh.h"
#include "ViewData.h"
#include "GridMesh.h"


class MeshLibrary {
public:
	MeshLibrary() {};
	void Init();
	std::vector<BasicMesh> lightingShaderMeshes;

	glm::fvec3 lightColor = glm::fvec3(1.0f, 0.0f, 0.5f);

	void AnimateGeometry();
	void RenderLightingShaderMeshes(const ViewData& data);
	void DrawGrid(const ViewData& data);
	void RenderAllMeshes(const ViewData& data);
private:
	GridMesh grid_mesh;
	ShaderLibrary shaderLibrary;

};