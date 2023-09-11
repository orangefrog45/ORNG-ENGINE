#pragma once
#include "../util/UUID.h"
#include "../util/Log.h"


namespace ORNG {
	struct Asset {
		Asset(const std::string& t_filepath) : filepath(t_filepath) {};
		virtual ~Asset() = default;

		bool operator == (const std::string& t_filepath) {
			return PathEqualTo(t_filepath);
		}

		inline bool PathEqualTo(const std::string& path) {
			if (filepath.empty())
				return false;

			bool ret = false;
			try {
				ret = std::filesystem::equivalent(path, filepath);
			}
			catch (std::exception e) {
				ORNG_CORE_ERROR("PathEqualTo err: '{0}'", e.what());
			}
			return ret;
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