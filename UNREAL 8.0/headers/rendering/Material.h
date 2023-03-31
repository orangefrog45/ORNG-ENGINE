#pragma once
#include <glm/glm.hpp>
#include "Texture2D.h"
#include <memory>


class Material {
public:
	glm::fvec3 ambient_color = glm::fvec3(1.0f, 1.0f, 1.0f);
	glm::fvec3 diffuse_color = glm::fvec3(1.0f, 1.0f, 1.0f);
	glm::fvec3 specular_color = glm::fvec3(1.0f, 1.0f, 1.0f);
	float roughness = 0.5f;

	std::unique_ptr<Texture2D> diffuse_texture;
	std::unique_ptr<Texture2D> specular_texture;
	std::unique_ptr<Texture2D> normal_map_texture;;
};