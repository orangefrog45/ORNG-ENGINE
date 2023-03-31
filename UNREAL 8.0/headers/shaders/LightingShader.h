#pragma once
#include "Shader.h"
#include "Material.h"
#include "WorldTransform.h"
#include "LightComponent.h"
#include "RendererResources.h"

class LightingShader : public Shader {
public:
	LightingShader() { paths.emplace_back("res/shaders/LightingVS.shader"); paths.emplace_back("res/shaders/LightingFS.shader"); };
	void Init() final;
	void SetViewPos(const glm::fvec3& pos);
	void SetAmbientLight(const BaseLight& light);
	void SetBaseColor(const glm::fvec3& color);
	void SetPointLights(std::vector< PointLightComponent*>& p_lights);
	void SetSpotLights(std::vector<SpotLightComponent*>& s_lights);
	void SetDiffuseTextureUnit(unsigned int unit);
	void SetSpecularTextureUnit(unsigned int unit);
	void SetDirDepthMapTexUnit(unsigned int unit);
	void SetSpotDepthMapTexUnit(unsigned int unit);
	void SetPointDepthMapTexUnit(unsigned int unit);
	void SetMaterial(const Material& material) override;
	void SetDirectionLight(const DirectionalLightComponent& light);
	void SetLightSpaceMatrix(const glm::fmat4& mat) { glUniformMatrix4fv(m_light_space_mat_loc, 1, GL_FALSE, &mat[0][0]); }
	void GenUBOs();

private:
	static const unsigned int point_light_fs_num_float = 16;
	static const unsigned int spot_light_fs_num_float = 36;
	void InitUniforms() override;

	GLuint m_point_light_UBO;
	GLuint m_spot_light_UBO;

	GLint m_ambient_light_color_loc;
	GLint m_light_ambient_intensity_loc;

	GLint m_camera_view_pos_loc;

	GLint m_material_ambient_color_loc;
	GLint m_material_specular_color_loc;
	GLint m_material_diffuse_color_loc;

	GLint m_sampler_dir_depth_loc;
	GLint m_sampler_spot_depth_loc;
	GLint m_sampler_point_depth_loc;
	GLint m_sampler_specular_loc;
	GLint m_specular_sampler_active_loc;
	GLint m_sampler_texture_col_location;
	GLint m_sampler_normals_loc;
	GLint m_normal_sampler_active_loc;

	GLint m_num_point_light_loc;
	GLint m_num_spot_light_loc;

	GLint m_dir_light_diffuse_intensity_loc;
	GLint m_dir_light_ambient_intensity_loc;
	GLint m_dir_light_color_loc;
	GLint m_dir_light_dir_loc;
	GLint m_light_space_mat_loc;

};