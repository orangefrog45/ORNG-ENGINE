#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <glew.h>
#include <sstream>
#include "util.h"
#include "Shader.h"
#include "shaders/BasicShader.h"

class ShaderLibrary {
public:

	void Init();

	BasicShader basicShader;

private:

};