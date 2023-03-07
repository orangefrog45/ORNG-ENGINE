#pragma once
#include "Shader.h"
#include "Material.h"
#include "WorldTransform.h"
#include "Light.h"

static const unsigned int max_point_lights = 112;
static const unsigned int max_spot_lights = 1;

class LightingShader : public Shader {
public:
	LightingShader() = default;
	void Init() override;
	void ActivateProgram() override;
	const GLint& GetProjectionLocation();
	const GLint& GetCameraLocation();
	const GLint& GetDiffuseSamplerLocation();
	const GLint& GetSpecularSamplerLocation();
	void SetProjection(const glm::fmat4& proj);
	void SetCamera(const glm::fmat4& cam);
	void SetViewPos(const glm::fvec3& pos);
	void SetAmbientLight(const BaseLight& light);
	void SetBaseColor(const glm::fvec3& color);
	void SetPointLights(std::vector< PointLight*>& p_lights);
	void SetSpotLights(std::vector<SpotLight*>& s_lights);
	void SetDiffuseTextureUnit(unsigned int unit);
	void SetSpecularTextureUnit(unsigned int unit);
	void SetMaterial(const Material& material) override;

private:
	void InitUniforms() override;

	struct PointLightLocationStruct {
		GLuint color;
		GLuint ambient_intensity;
		GLuint diffuse_intensity;
		GLuint position;
		GLuint max_distance;

		struct {
			GLuint constant;
			GLuint linear;
			GLuint exp;
		} Atten;
	} m_point_light_locations[max_point_lights];

	struct SpotLightLocationStruct {
		PointLightLocationStruct base;
		GLuint aperture;
		GLuint direction;
	} m_spot_light_locations[max_spot_lights];

	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	GLint m_ambient_light_color_loc;
	GLint m_light_ambient_intensity_loc;
	GLint m_camera_view_pos_loc;
	GLint m_material_ambient_color_loc;
	GLint m_material_specular_color_loc;
	GLint m_material_diffuse_color_loc;
	GLint m_sampler_specular_loc;
	GLint m_specular_sampler_active_loc;
	GLint m_num_point_light_loc;
	GLint m_num_spot_light_loc;
	GLint m_projection_location;
	GLint m_camera_location;
	GLint m_sampler_location;
};