#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "Shader.h"
#include "Material.h"

class PointLightComponent;
class SpotLightComponent;
class DirectionalLightComponent;
class BaseLight;

class LightingShader : public Shader {
public:
	LightingShader() { paths.emplace_back("res/shaders/LightingVS.shader"); paths.emplace_back("res/shaders/LightingFS.shader"); };
	void Init() final;
	void SetViewPos(const glm::vec3& pos);
	void SetAmbientLight(const BaseLight& light);
	void SetBaseColor(const glm::vec3& color);
	void SetPointLights(std::vector< PointLightComponent*>& p_lights);
	void SetSpotLights(std::vector<SpotLightComponent*>& s_lights);
	void SetDiffuseTextureUnit(unsigned int unit);
	void SetSpecularTextureUnit(unsigned int unit);
	void SetDirDepthMapTexUnit(unsigned int unit);
	void SetSpotDepthMapTexUnit(unsigned int unit);
	void SetPointDepthMapTexUnit(unsigned int unit);
	void SetNormalMapActive(bool active);
	void SetTerrainMode(bool mode);
	void SetMaterial(const Material& material) override;
	void SetDirectionLight(const DirectionalLightComponent& light);
	void SetLightSpaceMatrix(const glm::mat4& mat);
	void GenUBOs();

private:
	static const unsigned int point_light_fs_num_float = 16;
	static const unsigned int spot_light_fs_num_float = 36;
	void InitUniforms() override;

	unsigned int m_point_light_UBO;
	unsigned int m_spot_light_UBO;

	int m_terrain_mode_loc;

	int m_ambient_light_color_loc;
	int m_light_ambient_intensity_loc;

	int m_camera_view_pos_loc;

	int m_material_ambient_color_loc;
	int m_material_specular_color_loc;
	int m_material_diffuse_color_loc;

	int m_sampler_dir_depth_loc;
	int m_sampler_spot_depth_loc;
	int m_sampler_point_depth_loc;
	int m_sampler_specular_loc;
	int m_specular_sampler_active_loc;
	int m_sampler_texture_col_location;
	int m_sampler_terrain_diffuse_loc;
	int m_sampler_normals_loc;
	int m_normal_sampler_active_loc;

	int m_num_point_light_loc;
	int m_num_spot_light_loc;

	int m_dir_light_diffuse_intensity_loc;
	int m_dir_light_ambient_intensity_loc;
	int m_dir_light_color_loc;
	int m_dir_light_dir_loc;
	int m_light_space_mat_loc;

};