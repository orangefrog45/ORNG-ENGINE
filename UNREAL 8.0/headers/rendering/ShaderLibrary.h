#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include "util/util.h"
#include "Shader.h"
#include "shaders/LightingShader.h"
#include "shaders/GridShader.h"
#include "shaders/FlatColourShader.h"
#include "DepthShader.h"
#include "BasicSampler.h"
#include "ReflectionShader.h"
#include "CubemapShadowShader.h"

class ShaderLibrary {
public:
	ShaderLibrary() {};
	void Init();

	FlatColorShader flat_color_shader;
	LightingShader lighting_shader;
	GridShader grid_shader;
	DepthShader depth_shader;
	BasicSampler basic_sampler_shader;
	ReflectionShader reflection_shader;
	CubemapShadowShader cube_map_shadow_shader;
private:

};