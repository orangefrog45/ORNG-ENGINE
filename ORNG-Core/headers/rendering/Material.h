#pragma once
#include "rendering/Textures.h"
#include "util/UUID.h"

namespace ORNG {

	struct Material {
		friend class Scene;
		explicit Material(Texture2D* p_base_color_tex) : base_color_texture(p_base_color_tex) {};
		Material() = default;
		explicit Material(uint64_t uuid) : uuid(uuid) {};
		Material(const Material& other) = default;

		glm::vec3 base_color = glm::vec3(1.0f, 1.0f, 1.0f);

		float roughness = 0.2f;
		float metallic = 0.0f;
		float ao = 0.1f;
		bool emissive = false;
		float emissive_strength = 1.f;

		Texture2D* base_color_texture = nullptr;
		Texture2D* normal_map_texture = nullptr;
		Texture2D* metallic_texture = nullptr;
		Texture2D* roughness_texture = nullptr;
		Texture2D* ao_texture = nullptr;
		Texture2D* displacement_texture = nullptr;
		Texture2D* emissive_texture = nullptr;

		unsigned int parallax_layers = 24;
		float parallax_height_scale = 0.05f;

		glm::vec2 tile_scale{ 1.f, 1.f };

		std::string name = "Unnamed material";

		uint64_t shader_id = 1; // 1 = default shader (pbr lighting)
		UUID uuid;
	};

}