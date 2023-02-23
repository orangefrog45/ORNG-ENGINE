#pragma once
#include "ShaderLibrary.h"
#include "BasicMesh.h"
#include "ViewData.h"
#include "GridMesh.h"


class MeshLibrary {
public:
	MeshLibrary() {};
	void Init();
	std::vector<std::shared_ptr<BasicMesh>> lightingShaderMeshes;
	std::vector<std::shared_ptr<BasicMesh>> active_meshes;


	glm::fvec3 lightColor = glm::fvec3(1.0f, 0.0f, 0.5f);

	void AnimateGeometry();
	void RenderLightingShaderMeshes(const ViewData& data);
	void DrawGrid(const ViewData& data);
	void RenderAllMeshes(const ViewData& data);
	void LoadActiveMeshes();
private:
	GridMesh grid_mesh;
	ShaderLibrary shaderLibrary;

};