#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "util/util.h"

namespace ORNG {
	// Sequences renderpasses and provides interface to easily pass resources/data between them
	// Does NOT synchronize resources between passes, just runs them in the set order
	class RenderGraph {
	public:
		~RenderGraph() {
			for (auto* p_pass : m_renderpasses) {
				delete p_pass;
			}
		}

		// Only call AFTER adding every pass and required data to this graph
		void Init() {
			for (auto* p_pass : m_renderpasses) {
				p_pass->Init();
			}
		}

		void Execute() {
			for (auto* p_pass : m_renderpasses) {
				p_pass->DoPass();
			}
		}

		// Clears all renderpasses and data
		void Reset() {
			for (auto* p_pass : m_renderpasses) {
				delete p_pass;
			}

			m_renderpasses.clear();
			m_data.clear();
		}

		// Will overwrite any existing data with name='name'
		void SetData(const std::string& name, void* data) {
			m_data[name] = data;
		}

		template<typename T>
		T* GetData(const std::string& name) {
			return reinterpret_cast<T*>(m_data[name]);
		}

		template<typename T, typename... Args>
		void AddRenderpass(Args&&... args) {
			m_renderpasses.push_back(new T(this, std::forward<Args>(args)...));
		}

		// Returns pointer to renderpass or nullptr if not found
		template<std::derived_from<Renderpass> T>
		T* GetRenderpass() {
			for (auto* p_pass : m_renderpasses) {
				if (auto* p_casted = dynamic_cast<T*>(p_pass)) return p_casted;
			}

			return nullptr;
		}
	private:
		std::vector<Renderpass*> m_renderpasses{};

		std::unordered_map<std::string, void*> m_data;
	};
}
