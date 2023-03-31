#pragma once
#include "shaders/LightingShader.h"
#include "shaders/GridShader.h"
#include "shaders/FlatColourShader.h"
#include "DepthShader.h"
#include "BasicSampler.h"
#include "ReflectionShader.h"
#include "CubemapShadowShader.h"
#include "GBufferShader.h"
#include "SkyboxShader.h"

struct ShaderLibrary {
public:
	ShaderLibrary() {};
	void Init();
	void SetMatrixUBOs(glm::fmat4& proj, glm::fmat4& view);

	FlatColorShader flat_color_shader;
	LightingShader lighting_shader;
	GridShader grid_shader;
	DepthShader depth_shader;
	BasicSampler basic_sampler_shader;
	ReflectionShader reflection_shader;
	CubemapShadowShader cube_map_shadow_shader;
	GBufferShader g_buffer_shader;
	SkyboxShader skybox_shader;
private:
	unsigned int m_matrix_UBO;
};