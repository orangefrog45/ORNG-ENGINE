#pragma once
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include <memory>
#include "Texture2D.h"

struct Material {
	glm::vec3 ambient_color = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 specular_color = glm::vec3(1.0f, 1.0f, 1.0f);

	std::unique_ptr<Texture2D> diffuse_texture;
	std::unique_ptr<Texture2D> specular_texture;
	std::unique_ptr<Texture2D> normal_map_texture;
};