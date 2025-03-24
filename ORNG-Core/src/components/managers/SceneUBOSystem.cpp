#include "pch/pch.h"
#include "core/FrameTiming.h"
#include "components/systems/SceneUBOSystem.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"
#include "components/systems/CameraSystem.h"

using namespace ORNG;

void SceneUBOSystem::OnLoad() {
	m_matrix_ubo.Init();
	m_matrix_ubo.Resize(m_matrix_ubo_size);
	glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo.GetHandle());

	m_global_lighting_ubo.Init();
	m_global_lighting_ubo.Resize(72 * sizeof(float));
	glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBAL_LIGHTING, m_global_lighting_ubo.GetHandle());

	m_common_ubo.Init();
	m_common_ubo.Resize(m_common_ubo_size);
	glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBALS, m_common_ubo.GetHandle());
};

void SceneUBOSystem::OnUpdate() {
	UpdateCommonUBO();
	UpdateMatrixUBO();
	UpdateGlobalLightingUBO();
};

void SceneUBOSystem::OnUnload() {

};

void SceneUBOSystem::UpdateCommonUBO() {
	std::array<std::byte, m_common_ubo_size> data;
	std::byte* p_byte = data.data();

	auto* p_cam = mp_scene->GetSystem<CameraSystem>().GetActiveCamera();
	auto* p_cam_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
	glm::vec3 cam_pos = p_cam_transform->GetAbsPosition();
	glm::mat4 view_mat = glm::lookAt(cam_pos, cam_pos + p_cam_transform->forward, p_cam_transform->up);
	glm::mat4 proj_mat = p_cam->GetProjectionMatrix();

	// Any 0's are padding, all vec3 types are defined as vec4's in the shader for easier alignment.
	ConvertToBytes(p_byte,
		cam_pos, 0,
		p_cam_transform->forward, 0,
		p_cam_transform->right, 0,
		p_cam_transform->up, 0,
		cam_pos, 0,
		cam_pos, 0,
		cam_pos, 0,
		cam_pos, 0,
		//voxel_aligned_cam_pos_c0, 0,
		//voxel_aligned_cam_pos_c1, 0,
		//voxel_aligned_cam_pos_c1, 0,
		//voxel_aligned_cam_pos_c1, 0,
		static_cast<float>(FrameTiming::GetTotalElapsedTime()),
		p_cam->zFar,
		p_cam->zNear,
		static_cast<float>(FrameTiming::GetTimeStep()),
		mp_scene->GetTimeElapsed()
	);

	m_common_ubo.BufferSubData(0, m_common_ubo_size, data.data());
	glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBALS, m_common_ubo.GetHandle());

}

void SceneUBOSystem::UpdateVoxelAlignedPositions(const std::array<glm::vec3, 2>& positions) {
	m_common_ubo.BufferSubData(4 * sizeof(glm::vec4), sizeof(positions), reinterpret_cast<const std::byte*>(positions.data()));
}

void SceneUBOSystem::UpdateMatrixUBO() {
	auto* p_cam = mp_scene->GetSystem<CameraSystem>().GetActiveCamera();
	auto* p_cam_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();

	glm::mat4 proj = p_cam->GetProjectionMatrix();
	glm::mat4 view = glm::lookAt(p_cam_transform->GetPosition(), p_cam_transform->GetPosition() + p_cam_transform->forward, glm::vec3{ 0, 1, 0 });

	glm::mat4 proj_view = proj * view;
	static std::vector<std::byte> matrices;
	matrices.resize(m_matrix_ubo_size);
	std::byte* p_byte = matrices.data();

	ConvertToBytes(p_byte,
		proj,
		view,
		proj_view,
		glm::inverse(proj),
		glm::inverse(view),
		glm::inverse(proj_view)
	);

	m_matrix_ubo.BufferSubData(0, m_matrix_ubo_size, matrices.data());
	glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo.GetHandle());
}

void SceneUBOSystem::UpdateGlobalLightingUBO() {
	DirectionalLight& light = mp_scene->directional_light;

	// Update directional light matrices on CPU first
	auto* p_cam = mp_scene->HasSystem<CameraSystem>() ? mp_scene->GetSystem<CameraSystem>().GetActiveCamera() : nullptr;
	if (p_cam) {
		auto* p_cam_transform = p_cam->GetEntity()->GetComponent<TransformComponent>();
		glm::vec3 pos = p_cam_transform->GetAbsPosition();
		glm::mat4 cam_view_matrix = glm::lookAt(pos, pos + p_cam_transform->forward, p_cam_transform->up);
		const float fov = glm::radians(p_cam->fov / 2.f);

		glm::vec3 light_dir = light.GetLightDirection();
		light.m_light_space_matrices[0] = ExtraMath::CalculateLightSpaceMatrix(
			glm::perspective(fov, p_cam->aspect_ratio, 0.1f, light.cascade_ranges[0]),
			cam_view_matrix, light_dir, light.z_mults[0], (float)DirectionalLight::SHADOW_RESOLUTION);
		light.m_light_space_matrices[1] = ExtraMath::CalculateLightSpaceMatrix(
			glm::perspective(fov, p_cam->aspect_ratio, light.cascade_ranges[0] - 2.f, light.cascade_ranges[1]),
			cam_view_matrix, light_dir, light.z_mults[1], (float)DirectionalLight::SHADOW_RESOLUTION);
		light.m_light_space_matrices[2] = ExtraMath::CalculateLightSpaceMatrix(
			glm::perspective(fov, p_cam->aspect_ratio, light.cascade_ranges[1] - 2.f, light.cascade_ranges[2]),
			cam_view_matrix, light_dir, light.z_mults[2], (float)DirectionalLight::SHADOW_RESOLUTION);

	}

	auto& dir_light = mp_scene->directional_light;

	constexpr int buffer_size = 72 * sizeof(float);
	std::array<std::byte, buffer_size> light_data;

	std::byte* p_byte = light_data.data();
	ConvertToBytes(p_byte,
		dir_light.GetLightDirection(),
		0,
		dir_light.colour,
		0,
		dir_light.cascade_ranges,
		0,
		dir_light.light_size,
		dir_light.blocker_search_size,
		0,
		0,
		dir_light.GetLightSpaceMatrix(0),
		dir_light.GetLightSpaceMatrix(1),
		dir_light.GetLightSpaceMatrix(2),
		dir_light.shadows_enabled,
		0,
		0,
		0
	);

	m_global_lighting_ubo.BufferSubData(0, buffer_size, light_data.data());
	glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBAL_LIGHTING, m_global_lighting_ubo.GetHandle());
}
