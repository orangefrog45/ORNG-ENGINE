#pragma once
#include "Asset.h"
#include <yaml-cpp/node/node.h>
#include <bitsery/traits/string.h>

namespace ORNG {
	struct PrefabAsset final : public Asset {
		explicit PrefabAsset(const std::string& _filepath) : Asset(_filepath) {}
		~PrefabAsset() override = default;

		// Yaml string that can be deserialized into entity
		std::string serialized_content;

		// Parsed version of "serialized_content"
		YAML::Node node;

		template<typename S>
		void serialize(S& s) {
			s.text1b(serialized_content, 10000);
			s.object(uuid);
		}
	};
}
