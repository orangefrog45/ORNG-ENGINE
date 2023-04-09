#pragma once
#include "Shader.h"
class GBufferShader : public Shader {
public:
	GBufferShader() { paths = { "res/shaders/GBufferVS.shader", "res/shaders/GBufferFS.shader" }; }
	void InitUniforms() {};
};