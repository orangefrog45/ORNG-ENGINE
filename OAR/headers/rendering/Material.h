#pragma once
#include "rendering/Textures.h"

namespace ORNG {

	struct Material {

		glm::vec3 ambient_color = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 diffuse_color = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 specular_color = glm::vec3(1.0f, 1.0f, 1.0f);

		Texture2D* diffuse_texture = nullptr;
		Texture2D* specular_texture = nullptr;
		Texture2D* normal_map_texture = nullptr;

		std::string name = "Unnamed material";

		unsigned int material_id;
	};

}