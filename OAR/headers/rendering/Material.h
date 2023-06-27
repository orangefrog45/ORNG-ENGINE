#pragma once
#include "rendering/Textures.h"

namespace ORNG {

	struct Material {

		glm::vec3 base_color = glm::vec3(1.0f, 1.0f, 1.0f);

		float roughness = 0.2f;
		float metallic = 0.0f;
		float ao = 0.2f;


		Texture2D* base_color_texture = nullptr;
		Texture2D* normal_map_texture = nullptr;
		Texture2D* metallic_texture = nullptr;
		Texture2D* roughness_texture = nullptr;
		Texture2D* ao_texture = nullptr;
		Texture2D* displacement_texture = nullptr;

		unsigned int parallax_layers = 24;
		float parallax_height_scale = 0.05f;

		glm::vec2 tile_scale{ 1.f, 1.f };

		std::string name = "Unnamed material";

		unsigned int material_id;
	};

}