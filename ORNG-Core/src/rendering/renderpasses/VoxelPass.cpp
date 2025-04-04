#include "pch/pch.h"
#include "core/FrameTiming.h"
#include "core/GLStateManager.h"
#include "components/systems/CameraSystem.h"
#include "components/systems/MeshInstancingSystem.h"
#include "rendering/renderpasses/VoxelPass.h"
#include "rendering/Renderer.h"
#include "shaders/Shader.h"
#include "util/Timers.h"
#include "scene/Scene.h"
#include "scene/SceneEntity.h"
#include "rendering/SceneRenderer.h"
#include "rendering/RenderGraph.h"
#include "components/systems/SceneUBOSystem.h"

#include <glm/glm/gtc/round.hpp>

using namespace ORNG;

enum class MipMap3D_ShaderVariants {
	DEFAULT_MIP,
	ANISOTROPIC,
	ANISOTROPIC_CHAIN,
};

enum class VoxelizationSV {
	MAIN
};

enum class VoxelCS_SV {
	DECREMENT_LUMINANCE,
	ON_CAM_POS_UPDATE,
	BLIT,
};

void VoxelPass::Init() {
	std::vector<std::string> gbuffer_uniforms{
	"u_roughness_sampler_active",
		"u_metallic_sampler_active",
		"u_emissive_sampler_active",
		"u_normal_sampler_active",
		"u_ao_sampler_active",
		"u_displacement_sampler_active",
		"u_num_parallax_layers",
		"u_material.base_colour",
		"u_material.metallic",
		"u_material.roughness",
		"u_material.ao",
		"u_material.tile_scale",
		"u_material.emissive_strength",
		"u_material.flags",
		"u_material.displacement_scale",
		"u_shader_id",
		"u_material.sprite_data.num_rows",
		"u_material.sprite_data.num_cols",
		"u_material.sprite_data.fps"
	};

	auto voxel_uniforms = gbuffer_uniforms;
	PushBackMultiple(voxel_uniforms, "u_orth_proj_view_matrix", "u_voxel_size", "u_cascade_idx");
	m_scene_voxelization_shader.SetPath(GL_VERTEX_SHADER, "res/core-res/shaders/GBufferVS.glsl");
	m_scene_voxelization_shader.SetPath(GL_FRAGMENT_SHADER, "res/core-res/shaders/SceneVoxelizationFS.glsl");
	m_scene_voxelization_shader.AddVariant((unsigned)VoxelizationSV::MAIN, { "VOXELIZE" }, voxel_uniforms);

	m_voxel_compute_sv.SetPath(GL_COMPUTE_SHADER, "res/core-res/shaders/VoxelDecrementCS.glsl");
	m_voxel_compute_sv.AddVariant((unsigned)VoxelCS_SV::DECREMENT_LUMINANCE, { "DECREMENT_LUMINANCE" }, {});
	m_voxel_compute_sv.AddVariant((unsigned)VoxelCS_SV::ON_CAM_POS_UPDATE, { "ON_CAM_POS_UPDATE" }, { "u_delta_tex_coords" });
	m_voxel_compute_sv.AddVariant((unsigned)VoxelCS_SV::BLIT, { "BLIT" }, {});

	m_3d_mipmap_shader.SetPath(GL_COMPUTE_SHADER, "res/core-res/shaders/MipMap3D.glsl");
	m_3d_mipmap_shader.AddVariant((unsigned)MipMap3D_ShaderVariants::DEFAULT_MIP, {}, {});
	m_3d_mipmap_shader.AddVariant((unsigned)MipMap3D_ShaderVariants::ANISOTROPIC, { "ANISOTROPIC_MIPMAP" }, {});
	m_3d_mipmap_shader.AddVariant((unsigned)MipMap3D_ShaderVariants::ANISOTROPIC_CHAIN, { "ANISOTROPIC_MIPMAP_CHAIN" }, { "u_mip_level" });

	mp_scene = mp_graph->GetData<Scene>("Scene");

	const auto& out_spec = mp_graph->GetData<Texture2D>("OutCol")->GetSpec();
	m_render_dimensions = { out_spec.width, out_spec.height };

	Texture3DSpec voxel_spec_rgbaf;
	voxel_spec_rgbaf.format = GL_RGBA;
	voxel_spec_rgbaf.internal_format = GL_RGBA16F;
	voxel_spec_rgbaf.width = 256;
	voxel_spec_rgbaf.height = 256;
	voxel_spec_rgbaf.layer_count = 256;
	voxel_spec_rgbaf.storage_type = GL_FLOAT;
	voxel_spec_rgbaf.mag_filter = GL_LINEAR;
	voxel_spec_rgbaf.min_filter = GL_LINEAR;

	Texture3DSpec voxel_spec_r32ui = voxel_spec_rgbaf;
	voxel_spec_r32ui.format = GL_RED_INTEGER;
	voxel_spec_r32ui.internal_format = GL_R32UI;
	voxel_spec_r32ui.storage_type = GL_UNSIGNED_INT;
	voxel_spec_r32ui.min_filter = GL_NEAREST;
	voxel_spec_r32ui.mag_filter = GL_NEAREST;

	m_scene_voxel_tex_c0_normals.SetSpec(voxel_spec_r32ui);
	m_scene_voxel_tex_c0.SetSpec(voxel_spec_r32ui);
	m_scene_voxel_tex_c1.SetSpec(voxel_spec_r32ui);
	m_scene_voxel_tex_c1_normals.SetSpec(voxel_spec_r32ui);

	Texture3DSpec voxel_mip_spec = voxel_spec_rgbaf;
	voxel_mip_spec.width = 256;
	voxel_mip_spec.height = 256;
	voxel_mip_spec.layer_count = 256 * 6; // Each face starts at z = face_index * VOXEL_MIP_RES
	voxel_mip_spec.generate_mipmaps = true;
	voxel_mip_spec.wrap_params = GL_CLAMP_TO_EDGE;
	voxel_mip_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;

	m_voxel_mip_faces_c0.SetSpec(voxel_mip_spec);
	m_voxel_mip_faces_c1.SetSpec(voxel_mip_spec);

	m_scene_voxelization_fb.Init();
}

void VoxelPass::DoPass() {
	const unsigned active_cascade_idx = FrameTiming::GetFrameCount() % 2;
	const glm::vec3 cam_pos = mp_scene->GetSystem<CameraSystem>().GetActiveCamera()->GetEntity()->GetComponent<TransformComponent>()->GetAbsPosition();

	switch (active_cascade_idx) {
	case 0:
		if (auto [aligned_cam_pos_moved, new_aligned_cam_pos, delta_tex_coords] = UpdateVoxelAlignedCameraPos(6.4f * 0.5f, cam_pos, m_voxel_aligned_cam_positions[0]); aligned_cam_pos_moved) {
			m_voxel_aligned_cam_positions[0] = new_aligned_cam_pos;
			AdjustVoxelGridForCameraMovement(m_scene_voxel_tex_c0, m_scene_voxel_tex_c0_normals, delta_tex_coords, 256);
		}
		break;
	case 1:
		if (auto [aligned_cam_pos_moved, new_aligned_cam_pos, delta_tex_coords] = UpdateVoxelAlignedCameraPos(12.8f * 0.5f, cam_pos, m_voxel_aligned_cam_positions[1]); aligned_cam_pos_moved) {
			m_voxel_aligned_cam_positions[1] = new_aligned_cam_pos;
			AdjustVoxelGridForCameraMovement(m_scene_voxel_tex_c1, m_scene_voxel_tex_c1_normals, delta_tex_coords, 256);
		}
		break;
	}

	mp_scene->GetSystem<SceneUBOSystem>().UpdateVoxelAlignedPositions(m_voxel_aligned_cam_positions);

	constexpr float voxel_size = 0.2f;
	const unsigned cascade_width = m_scene_voxel_tex_c0.GetSpec().width;
	float half_cascade_width = cascade_width * 0.5f;

	Texture3D& main_cascade_tex = active_cascade_idx == 0 ? m_scene_voxel_tex_c0 : m_scene_voxel_tex_c1;
	Texture3D& normals_main_cascade_tex = active_cascade_idx == 0 ? m_scene_voxel_tex_c0_normals : m_scene_voxel_tex_c1_normals;
	Texture3D& cascade_mips = active_cascade_idx == 0 ? m_voxel_mip_faces_c0 : m_voxel_mip_faces_c1;

	ORNG_PROFILE_FUNC_GPU();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_ALWAYS);

	GLuint clear_val = 0;
	// Clear normals, luminance texture not cleared as it accumulates over frames
	glClearTexImage(normals_main_cascade_tex.GetTextureHandle(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_val);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glBindImageTexture(0, main_cascade_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	glBindImageTexture(1, normals_main_cascade_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	// Decrement accumulated values from previous frames as new luminance is about to be added
	m_voxel_compute_sv.Activate((unsigned)VoxelCS_SV::DECREMENT_LUMINANCE);
	GL_StateManager::DispatchCompute(cascade_width / 4, cascade_width / 4, cascade_width / 4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glViewport(0, 0, 256, 256);
	m_scene_voxelization_shader.Activate((unsigned)VoxelizationSV::MAIN);
	m_scene_voxelization_shader.SetUniform("u_voxel_size", voxel_size);
	m_scene_voxelization_shader.SetUniform("u_cascade_idx", active_cascade_idx);
	auto proj = glm::ortho(-half_cascade_width * voxel_size, half_cascade_width * voxel_size, -half_cascade_width * voxel_size, half_cascade_width * voxel_size, voxel_size, cascade_width * voxel_size);

	// Draw scene into voxel textures
	std::array<glm::mat4, 3> matrices = { glm::lookAt(cam_pos + glm::vec3(half_cascade_width * voxel_size, 0, 0), cam_pos, {0, 1, 0}),
		glm::lookAt(cam_pos + glm::vec3(0, half_cascade_width * voxel_size, 0), cam_pos, {0, 0, 1}),
		glm::lookAt(cam_pos + glm::vec3(0, 0, half_cascade_width * voxel_size), cam_pos, {0, 1, 0}) };
	for (int i = 0; i < 3; i++) {
		m_scene_voxelization_shader.SetUniform("u_orth_proj_view_matrix", proj * matrices[i]);
		for (auto* p_group : mp_scene->GetSystem<MeshInstancingSystem>().GetInstanceGroups()) {
			SceneRenderer::DrawInstanceGroupGBuffer(&m_scene_voxelization_shader, p_group, SOLID, MaterialFlags::ORNG_MatFlags_ALL, ORNG_MatFlags_INVALID,
				true, GL_TRIANGLES);
		}
	}
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	glViewport(0, 0, m_render_dimensions.x, m_render_dimensions.y);

	// Clear old mip data
	constexpr unsigned num_mip_levels = 6;
	for (int i = 0; i < num_mip_levels; i++) {
		glClearTexImage(cascade_mips.GetTextureHandle(), i, GL_RGBA, GL_FLOAT, nullptr);
	}
	glBindImageTexture(1, cascade_mips.GetTextureHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	const int voxel_mip_ratio = m_scene_voxel_tex_c0.GetSpec().width / m_voxel_mip_faces_c0.GetSpec().width;
	// Create first anisotropic mip
	glBindImageTexture(2, normals_main_cascade_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	m_3d_mipmap_shader.Activate((unsigned)MipMap3D_ShaderVariants::ANISOTROPIC);
	GL_StateManager::DispatchCompute((int)cascade_width / voxel_mip_ratio / 4, (int)cascade_width / voxel_mip_ratio / 4, (int)cascade_width / voxel_mip_ratio / 4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	// Create mip chain from anisotropic mip
	m_3d_mipmap_shader.Activate((unsigned)MipMap3D_ShaderVariants::ANISOTROPIC_CHAIN);
	GL_StateManager::BindTexture(GL_TEXTURE_3D, cascade_mips.GetTextureHandle(), GL_TEXTURE0);
	for (int i = 1; i <= 6; i++) {
		m_3d_mipmap_shader.SetUniform("u_mip_level", i);
		glBindImageTexture(0, cascade_mips.GetTextureHandle(), i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

		int group_dim = (int)glm::ceil((cascade_width / voxel_mip_ratio / (i + 1)) / 4.f);
		GL_StateManager::DispatchCompute(group_dim, group_dim, group_dim);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(1, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

}

void VoxelPass::Destroy() {

}

void VoxelPass::AdjustVoxelGridForCameraMovement(Texture3D& voxel_luminance_tex, Texture3D& intermediate_copy_tex,
	glm::ivec3 delta_tex_coords, unsigned tex_size) {
	glClearTexImage(intermediate_copy_tex.GetTextureHandle(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindImageTexture(0, voxel_luminance_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	glBindImageTexture(1, intermediate_copy_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	m_voxel_compute_sv.Activate((unsigned)VoxelCS_SV::ON_CAM_POS_UPDATE);
	m_voxel_compute_sv.SetUniform("u_delta_tex_coords", delta_tex_coords);
	GL_StateManager::DispatchCompute(tex_size / 4, tex_size / 4, tex_size / 4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindImageTexture(0, intermediate_copy_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	glBindImageTexture(1, voxel_luminance_tex.GetTextureHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	m_voxel_compute_sv.Activate((unsigned)VoxelCS_SV::BLIT);
	GL_StateManager::DispatchCompute(tex_size / 4, tex_size / 4, tex_size / 4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

std::tuple<bool, glm::vec3, glm::vec3> VoxelPass::UpdateVoxelAlignedCameraPos(float alignment, glm::vec3 unaligned_cam_pos,
	glm::vec3 voxel_aligned_cam_pos) {
	auto new_pos = glm::roundMultiple(unaligned_cam_pos, glm::vec3(alignment));
	glm::bvec3 res = glm::epsilonEqual(new_pos, voxel_aligned_cam_pos, 0.015f);
	bool aligned_pos_moved = !glm::all(res);

	// Camera update will have shifted texture by 64 voxels as the camera position is rounded to 64 voxels (this aligns it with the highest mip to remove artifacts)
	if (aligned_pos_moved) {
		glm::ivec3 delta_tex_coords = glm::ivec3((unsigned)(!res.x) * 64, (unsigned)(!res.y) * 64, (unsigned)(!res.z) * 64)
			* glm::ivec3(new_pos.x < voxel_aligned_cam_pos.x ? -1 : 1, new_pos.y < voxel_aligned_cam_pos.y ? -1 : 1, new_pos.z < voxel_aligned_cam_pos.z ? -1 : 1);


		return std::make_tuple(aligned_pos_moved, new_pos, delta_tex_coords);
	}

	return std::make_tuple(aligned_pos_moved, new_pos, glm::vec3{ 0, 0, 0 });
}

