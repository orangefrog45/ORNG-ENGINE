#pragma once
#include "Shader.h"
#include "Material.h"
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
	void Init();
	void ActivateProgram();
	const GLint& GetProjectionLocation();
	const GLint& GetCameraLocation();
	const GLint& GetSamplerLocation();
	void SetProjection(const glm::fmat4& proj);
	void SetCamera(const glm::fmat4& cam);
	void SetLight(const BaseLight& light);
	void SetTextureUnit(unsigned int unit);
	void SetMaterial(const Material& material);


private:
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	void InitUniforms();
	GLint light_color_loc;
	GLint light_ambient_intensity_loc;
	GLint material_ambient_intensity_loc;
	GLint m_projection_location;
	GLint m_camera_location;
	GLint m_sampler_location;

};