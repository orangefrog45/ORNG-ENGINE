#pragma once
#include <glm/glm.hpp>
#include "Texture.h"
#include <memory>


class Material {
public:
	glm::fvec3 ambient_color = glm::fvec3(1.0f, 1.0f, 1.0f);
	glm::fvec3 diffuse_color = glm::fvec3(1.0f, 1.0f, 1.0f);
	glm::fvec3 specular_color = glm::fvec3(1.0f, 1.0f, 1.0f);

	std::shared_ptr<Texture> diffuse_texture;
	std::shared_ptr<Texture> specular_texture;
};