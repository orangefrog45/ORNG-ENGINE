#pragma once
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <yaml-cpp/yaml.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "util/Log.h"

namespace YAML {
    template<>
    struct convert<lml::vec3> {
        static Node encode(const lml::vec3& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, lml::vec3& rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<lml::ivec3> {
        static Node encode(const lml::ivec3& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, lml::ivec3& rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<int>();
            rhs.y = node[1].as<int>();
            rhs.z = node[2].as<int>();
            return true;
        }
    };

    template<>
    struct convert<lml::vec4> {
        static Node encode(const lml::vec4& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            return node;
        }

        static bool decode(const Node& node, lml::vec4& rhs) {
            if (!node.IsSequence() || node.size() != 4) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };

    template<>
    struct convert<lml::vec2> {
        static Node encode(const lml::vec2& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            return node;
        }

        static bool decode(const Node& node, lml::vec2& rhs) {
            if (!node.IsSequence() || node.size() != 2) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            return true;
        }
    };
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const lml::vec3& v) {
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const lml::ivec3& v) {
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const lml::vec4 v) {
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const lml::vec2 v) {
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}

template<typename T>
inline void Out(YAML::Emitter& out, const std::string& key, const T& v) {
    out << YAML::Key << key << YAML::Value << v;
}

template<typename T>
static T SafeRead(const YAML::Node& node, const std::string& key, T default_value) {
	try {
	    return node[key].as<T>();
	} catch (std::exception& e) {
	    ORNG_CORE_WARN("YAML error reading key '{}':", key, e.what());
	    return default_value;
	}
}
