#include <ShaderLibrary.h>
#include <string>
#include <sstream>
#include <glew.h>
#include <iostream>
#include <freeglut.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include "GLErrorHandling.h"
#include "util/util.h"

void ShaderLibrary::Init() {
	grid_shader.Init();
	lighting_shader.Init();

	lighting_shader.SetDiffuseTextureUnit(TextureUnits::COLOR_TEXTURE_UNIT);
	lighting_shader.SetSpecularTextureUnit(TextureUnits::SPECULAR_TEXTURE_UNIT);
}

