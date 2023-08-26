#pragma once
#include "terrain/TerrainChunk.h"

namespace ORNG {

	class ChunkLoader {
	public:
		static void RequestChunkLoad(TerrainChunk& chunk);
		static void CancelChunkLoad(int key);
		static void ProcessChunkLoads();
		static void LoadChunk(TerrainChunk* chunk);
	private:
		static [[nodiscard]] int CreateKey();
		inline static int m_last_id = 0;
		inline static std::map<int, TerrainChunk*> m_chunks_to_load;
		inline static std::map<int, std::future<void>> m_futures;
		inline static unsigned int m_active_processes = 0;
	};

}