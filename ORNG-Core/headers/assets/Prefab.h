#pragma once
#include "Asset.h"
#include <yaml-cpp/node/node.h>

namespace ORNG {
	struct Prefab : public Asset {
		Prefab(const std::string& filepath) : Asset(filepath) {};
		// Yaml string that can be deserialized into entity
		std::string serialized_content;

		// Parsed version of "serialized_content"
		YAML::Node node;

		template<typename S>
		void serialize(S& s) {
			s.text1b(serialized_content, 10000);
			s.object(uuid);
			s.text1b(filepath, ORNG_MAX_FILEPATH_SIZE);
		}
	};
}
