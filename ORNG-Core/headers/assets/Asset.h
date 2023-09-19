#pragma once
#include "../util/UUID.h"
#include "../util/Log.h"
#include "util/util.h"


namespace ORNG {
	struct Asset {
		Asset(const std::string& t_filepath) : filepath(t_filepath) {};
		virtual ~Asset() = default;

		bool operator == (const std::string& t_filepath) {
			return PathEqualTo(t_filepath);
		}

		bool PathEqualTo(const std::string& path) {
			return ORNG::PathEqualTo(path, filepath);
		}

		std::string filepath = "";
		UUID uuid;



		template<typename S>
		void serialize(S& s) {
			s.object(uuid);
			s.container1b(filepath, filepath.size());
		}
	};
}