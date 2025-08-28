#pragma once

namespace ORNG {
	class Renderpass {
	public:
		Renderpass(class RenderGraph* p_graph, const std::string& name) : mp_graph(p_graph), m_name(name) {}
		virtual ~Renderpass() = default;

		virtual void Init() = 0;

		virtual void DoPass() = 0;

		const std::string& GetName() const noexcept {
			return m_name;
		}

	protected:
		RenderGraph* mp_graph = nullptr;
		std::string m_name;
	};
}
