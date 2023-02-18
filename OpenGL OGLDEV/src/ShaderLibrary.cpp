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
#include "util.h"

void ShaderLibrary::Init() {
	basic_shader.Init();
	lighting_shader.Init();
}

