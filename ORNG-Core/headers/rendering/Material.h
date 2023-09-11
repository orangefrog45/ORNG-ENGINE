#pragma once
#include "assets/Asset.h"
#include "Textures.h"

namespace ORNG {
	class Texture2D;
	struct Material : public Asset {
		friend class Scene;
		explicit Material(Texture2D* p_base_color_tex) : Asset(""), base_color_texture(p_base_color_tex) {};
		Material() : Asset("") {};
		explicit Material(uint64_t t_uuid) : Asset("") { uuid = UUID(t_uuid); };
		Material(const Material& other) = default;

		template<typename S>
		void serialize(S& s) {
			s.object(base_color);
			s.value4b(roughness);
			s.value4b(metallic);
			s.value4b(ao);
			s.value1b(emissive);
			s.value4b(emissive_strength);
			s.value8b(base_color_texture ? base_color_texture->uuid() : 0);
			s.value8b(normal_map_texture ? normal_map_texture->uuid() : 0);
			s.value8b(metallic_texture ? metallic_texture->uuid() : 0);
			s.value8b(roughness_texture ? roughness_texture->uuid() : 0);
			s.value8b(ao_texture ? ao_texture->uuid() : 0);
			s.value8b(displacement_texture ? displacement_texture->uuid() : 0);
			s.value8b(emissive_texture ? emissive_texture->uuid() : 0);
			s.value4b(parallax_layers);
			s.value4b(parallax_height_scale);
			s.object(tile_scale);
			s.text1b(name, ORNG_MAX_NAME_SIZE);
			s.object(uuid);
			s.text1b(filepath, ORNG_MAX_NAME_SIZE);
		}


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

		uint32_t parallax_layers = 24;
		float parallax_height_scale = 0.05f;

		glm::vec2 tile_scale{ 1.f, 1.f };

		std::string name = "Unnamed material";

		uint64_t shader_id = 1; // 1 = default shader (pbr lighting)
	};

}