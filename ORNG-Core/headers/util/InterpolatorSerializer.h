#pragma once

#include "yaml/include/yaml-cpp/yaml.h"
#include "Interpolators.h"
#include "scene/SerializationUtil.h"

namespace ORNG {
	template<typename T>
	concept IsInterpolator = std::is_same_v<T, InterpolatorV3> || std::is_same_v<T, InterpolatorV1>;

	struct InterpolatorSerializer {
		template <IsInterpolator T>
		static void SerializeInterpolator(const std::string& name, YAML::Emitter& out, const T& interpolator) {
			out << YAML::Key << name << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Points" << YAML::Value << YAML::BeginSeq;

			for (auto point : interpolator.points) {
				out << point;
			}

			out << YAML::EndSeq;

			out << YAML::Key << "Scale" << YAML::Value << interpolator.scale;
			out << YAML::EndMap;
		}

		template <IsInterpolator T>
		static void DeserializeInterpolator(YAML::Node interpolator_node, T& interpolator) {
			auto point_node = interpolator_node["Points"];
			interpolator.points.clear();

			for (auto point : point_node) {
				if constexpr (std::is_same_v<T, InterpolatorV3>)
					interpolator.points.push_back(point.as<glm::vec4>());
				else
					interpolator.points.push_back(point.as<glm::vec2>());
			}

			interpolator.scale = interpolator_node["Scale"].as<float>();
		}
	};

}