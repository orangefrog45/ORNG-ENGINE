#pragma once
#include "rendering/Textures.h"

namespace ORNG {

	struct Material {

		glm::vec3 base_color = glm::vec3(1.0f, 1.0f, 1.0f);

		float roughness = 0.1f;
		float metallic = 0.9f;
		float ao = 0.2f;


		Texture2D* base_color_texture = nullptr;
		Texture2D* normal_map_texture = nullptr;
		Texture2D* metallic_texture = nullptr;
		Texture2D* roughness_texture = nullptr;
		Texture2D* ao_texture = nullptr;
		Texture2D* displacement_texture = nullptr;

		unsigned int parallax_layers = 12;
		float parallax_height_scale = 0.1f;

		std::string name = "Unnamed material";

		unsigned int material_id;
	};

}