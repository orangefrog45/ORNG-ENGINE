#pragma once
#include "Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"

namespace ORNG {
	class VoxelPass : public Renderpass {
		friend class LightingPass;
	public:
		explicit VoxelPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Voxel") {}

		void Init() override;

		void DoPass() override;

		std::tuple<bool, glm::vec3, glm::vec3> UpdateVoxelAlignedCameraPos(float alignment, glm::vec3 unaligned_cam_pos, glm::vec3 voxel_aligned_cam_pos);
	private:
		void AdjustVoxelGridForCameraMovement(Texture3D& voxel_luminance_tex, Texture3D& intermediate_copy_tex, glm::ivec3 delta_tex_coords, unsigned tex_size);

		std::array<glm::vec3, 2> m_voxel_aligned_cam_positions;

		glm::ivec2 m_render_dimensions{ 0, 0 };

		Texture3D m_scene_voxel_tex_c0{ "scene voxel tex cascade 0" };
		Texture3D m_scene_voxel_tex_c0_normals{ "scene voxel tex normals cascade 0" };

		Texture3D m_scene_voxel_tex_c1{ "scene voxel tex cascade 1" };
		Texture3D m_scene_voxel_tex_c1_normals{ "scene voxel tex normals cascade 1" };

		Texture3D m_voxel_mip_faces_c0{ "voxel mips cascade 0" };
		Texture3D m_voxel_mip_faces_c1{ "voxel mips cascade 1" };

		ShaderVariants m_scene_voxelization_shader;
		ShaderVariants m_3d_mipmap_shader;
		ShaderVariants m_voxel_compute_sv;
		Framebuffer m_scene_voxelization_fb;

		ShaderVariants mp_sv;

		class Scene* mp_scene = nullptr;
		class PointlightSystem* mp_pointlight_system = nullptr;
	};
}
