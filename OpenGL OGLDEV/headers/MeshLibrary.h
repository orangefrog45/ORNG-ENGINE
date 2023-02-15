#pragma once
#include "BasicMesh.h"
#include "ShaderLibrary.h"

struct WorldData {
	WorldData(const glm::fmat4& camMat, const glm::fmat4& invCamMat, const glm::fmat4& projMat) : cameraMatrix(camMat), inverseCameraMatrix(invCamMat), projectionMatrix(projMat) {}
	glm::fmat4 cameraMatrix;
	glm::fmat4 inverseCameraMatrix;
	glm::fmat4 projectionMatrix;
};

class MeshLibrary {
public:
	void Init();
	std::vector<BasicMesh> basicShaderMeshes;
	std::vector<BasicMesh> testShaderMeshes;
	std::vector<BasicMesh> lightingShaderMeshes;

	void RenderBasicShaderMeshes(const WorldData& data);
	void RenderTestShaderMeshes();
	void RenderLightingShaderMeshes();
	void RenderAllMeshes(const WorldData& data);


	ShaderLibrary shaderLibrary;


	void LoadMeshes();
};