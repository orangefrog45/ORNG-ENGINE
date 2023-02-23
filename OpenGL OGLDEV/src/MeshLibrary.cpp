#include <glew.h>
#include <iostream>
#include <execution>
#include <glm/gtx/matrix_major_storage.hpp>
#include "MeshLibrary.h"
#include "ExtraMath.h"
#include <future>


void MeshLibrary::Init() {
	shaderLibrary.Init();
	grid_mesh.Init();
}

void MeshLibrary::DrawGrid(const ViewData& data) {
	shaderLibrary.grid_shader.ActivateProgram();
	shaderLibrary.grid_shader.SetProjection(glm::colMajor4(data.projectionMatrix));
	shaderLibrary.grid_shader.SetCamera(glm::colMajor4(data.cameraMatrix));
	shaderLibrary.grid_shader.SetCameraPos(data.camera_pos);
	grid_mesh.CheckBoundary(data.camera_pos);
	grid_mesh.Draw();
}

void MeshLibrary::AnimateGeometry() {
	auto& transforms = lightingShaderMeshes[0]->GetWorldTransforms();
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float angle = 0.0f;
	static double offset = 0.0f;
	float rowOffset = 0.0f;
	static float coefficient = 1.0f;

	for (unsigned int i = 0; i < transforms.size(); i++) {
		angle += 10.0f;
		x += 4 * cosf(ExtraMath::ToRadians((angle + rowOffset) * coefficient));
		y += 4 * sinf(ExtraMath::ToRadians((angle + rowOffset) * coefficient));
		if (angle == 180.0f) {
			//coefficient *= -1.0;
		}
		if (angle >= 360) {
			coefficient *= 1.0f;
			angle = 0.0f;
			z += 4.0f;
			x = 0.0f;
			y = 0.0f;
			rowOffset += 9.0f;
		}
		offset += 0.00005f;
		transforms[i].SetPosition((cosf(ExtraMath::ToRadians(offset)) * x) - (sinf(ExtraMath::ToRadians(offset)) * y), (sinf(ExtraMath::ToRadians(offset)) * x) + (cosf(ExtraMath::ToRadians(offset)) * y), z);
		transforms[i].SetRotation(offset, -offset, offset);
		transforms[i].SetScale(1.0f, 10.0f, 1.f);

	}
}


void MeshLibrary::RenderLightingShaderMeshes(const ViewData& data) {

	shaderLibrary.lighting_shader.ActivateProgram();
	shaderLibrary.lighting_shader.SetProjection(glm::colMajor4(data.projectionMatrix));
	shaderLibrary.lighting_shader.SetCamera(glm::colMajor4(data.cameraMatrix));

	BaseLight base_light = BaseLight();
	PointLight point_light = PointLight(glm::fvec3(100.0f, 0.0f, 0.0f), glm::fvec3(0.0f, 0.5f, 0.0f));

	base_light.color = glm::fvec3(1.0f, 1.0f, 1.0f);
	base_light.ambient_intensity = 0.2f;
	shaderLibrary.lighting_shader.SetTextureUnit(GL_TEXTURE0);
	shaderLibrary.lighting_shader.SetPointLight(point_light);
	shaderLibrary.lighting_shader.SetAmbientLight(base_light);

	for (std::shared_ptr<BasicMesh> mesh : lightingShaderMeshes) {
		shaderLibrary.lighting_shader.SetMaterial(mesh->GetMaterial());

		mesh->UpdateTransformBuffers(data);
		mesh->Render();
	}
}

void MeshLibrary::RenderAllMeshes(const ViewData& data) {
	DrawGrid(data);
	RenderLightingShaderMeshes(data);
}

