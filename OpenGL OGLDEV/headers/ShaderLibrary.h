#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <glew.h>
#include <sstream>
#include "util/util.h"
#include "Shader.h"
#include "shaders/LightingShader.h"
#include "shaders/GridShader.h"

class ShaderLibrary {
public:
	ShaderLibrary() {};
	void Init();

	BaseLight base_light;
	LightingShader lighting_shader;
	GridShader grid_shader;

private:

};