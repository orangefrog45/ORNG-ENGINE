#pragma once
#include "Shader.h"
#include "Material.h"

class PointLight {
public:
	PointLight() = default;
	PointLight(const glm::fvec3& position, const glm::fvec3& color) : position(position), color(color) {};
	glm::fvec3 position;
	glm::fvec3 color;
};


class BaseLight {
public:
	glm::fvec3 color;
	float ambient_intensity;

	BaseLight() {
		color = glm::fvec3(1.0f, 1.0f, 1.0f);
		ambient_intensity = 0.0f;
	}
};

class LightingShader : public Shader {
public:
	LightingShader() {};
	PointLight point_light;
	void Init() override;
	void ActivateProgram() override;
	const GLint& GetProjectionLocation();
	const GLint& GetCameraLocation();
	const GLint& GetSamplerLocation();
	void SetProjection(const glm::fmat4& proj);
	void SetCamera(const glm::fmat4& cam);
	void SetAmbientLight(const BaseLight& light);
	void SetPointLight(const PointLight& light);
	void SetTextureUnit(unsigned int unit);
	void SetMaterial(const Material& material);


private:
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	void InitUniforms() override;
	GLint m_ambient_light_color_loc;
	GLint m_point_light_position_loc;
	GLint m_point_light_color_loc;
	GLint m_light_ambient_intensity_loc;
	GLint m_material_ambient_intensity_loc;
	GLint m_projection_location;
	GLint m_camera_location;
	GLint m_sampler_location;

};