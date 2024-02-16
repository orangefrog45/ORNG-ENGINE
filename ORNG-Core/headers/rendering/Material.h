#pragma once
#include "assets/Asset.h"
#include "Textures.h"

namespace ORNG {
	class Texture2D;

	enum RenderGroup : uint8_t {
		SOLID,
		ALPHA_TESTED
	};

	struct SpriteSheetData {
		unsigned num_rows = 5;
		unsigned num_cols = 5;

		unsigned fps = 24;

		template<typename S>
		void serialize(S& s) {
			s.value4b(num_rows);
			s.value4b(num_cols);
			s.value4b(fps);
		}
	};

	enum MaterialFlags : uint32_t {
		ORNG_MatFlags_INVALID = 0,

		ORNG_MatFlags_NONE = 1 << 0,
		ORNG_MatFlags_TESSELLATED = 1 << 1,
		ORNG_MatFlags_PARALLAX_MAP = 1 << 2,
		ORNG_MatFlags_EMISSIVE = 1 << 3,
		ORNG_MatFlags_IS_SPRITESHEET = 1 << 4,

		ORNG_MatFlags_ALL = ~(0),
	};

	class Material : public Asset {
		friend class SceneRenderer;
		friend class AssetManager;
	public:
		explicit Material(Texture2D* p_base_color_tex) : Asset(""), base_color_texture(p_base_color_tex) {};

		// New material wont have a path until saved
		Material() : Asset("") {};

		// If being deserialized, provide filepath
		explicit Material(const std::string& filepath) : Asset(filepath) {};

		// Set ID explicitly, this is only to be used in rare scenarios (e.g the base replacement material in-engine)
		explicit Material(uint64_t id) : Asset("") { uuid = UUID(0); };


		Material(const Material& other) = default;

		template<typename S>
		void serialize(S& s) {
			s.object(base_color);
			s.value1b((uint8_t)render_group);
			s.value4b(roughness);
			s.value4b(metallic);
			s.value4b(ao);
			s.value4b(emissive_strength);
			s.value8b(base_color_texture ? base_color_texture->uuid() : 0);
			s.value8b(normal_map_texture ? normal_map_texture->uuid() : 0);
			s.value8b(metallic_texture ? metallic_texture->uuid() : 0);
			s.value8b(roughness_texture ? roughness_texture->uuid() : 0);
			s.value8b(ao_texture ? ao_texture->uuid() : 0);
			s.value8b(displacement_texture ? displacement_texture->uuid() : 0);
			s.value8b(emissive_texture ? emissive_texture->uuid() : 0);
			s.value4b(parallax_layers);
			s.object(tile_scale);
			s.text1b(name, ORNG_MAX_NAME_SIZE);
			s.object(uuid);
			s.object(spritesheet_data);

			s.value4b((uint32_t)flags);
			s.value4b(displacement_scale);
		}

		inline void FlipFlags(MaterialFlags _flags) {
			flags = (MaterialFlags)(flags ^ _flags);

			if (flags != 0)
				flags = (MaterialFlags)(flags & ~(ORNG_MatFlags_NONE));
			else
				flags = (MaterialFlags)(flags | ORNG_MatFlags_NONE);
		}

		inline void RaiseFlags(MaterialFlags _flags) {
			flags = (MaterialFlags)(flags | _flags);

			if (flags != 0)
				flags = (MaterialFlags)(flags & ~(ORNG_MatFlags_NONE));
		};

		inline void RemoveFlags(MaterialFlags _flags) {
			flags = (MaterialFlags)(flags & ~_flags);

			if (flags == 0)
				flags = (MaterialFlags)(flags | ORNG_MatFlags_NONE);
		}

		inline MaterialFlags GetFlags() {
			return flags;
		}

		RenderGroup render_group = SOLID;

		glm::vec4 base_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		float roughness = 0.2f;
		float metallic = 0.0f;
		float ao = 0.1f;
		float emissive_strength = 1.f;
		float opacity = 1.0;

		Texture2D* base_color_texture = nullptr;
		Texture2D* normal_map_texture = nullptr;
		Texture2D* metallic_texture = nullptr;
		Texture2D* roughness_texture = nullptr;
		Texture2D* ao_texture = nullptr;
		Texture2D* displacement_texture = nullptr;
		Texture2D* emissive_texture = nullptr;

		SpriteSheetData spritesheet_data;

		uint32_t parallax_layers = 24;
		float displacement_scale = 0.05f;

		glm::vec2 tile_scale{ 1.f, 1.f };

		std::string name = "Unnamed material";

		uint64_t shader_id = 1; // 1 = default shader (pbr lighting)

	private:
		MaterialFlags flags = MaterialFlags::ORNG_MatFlags_NONE;
	};
}