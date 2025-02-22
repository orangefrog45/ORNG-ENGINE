#include "util/LoggerUI.h"
#include "events/EventManager.h"

using namespace ORNG;

void LoggerUI::Init() {
	m_log_listener.OnEvent = [this](const auto& _event) {
		OnLogEvent(_event);
	};
	Events::EventManager::RegisterListener(m_log_listener);
}

unsigned LoggerUI::RenderLogContentsWithImGui() {
	for (const auto& log : m_logs) {
		ImVec4 col;
		switch (log.type) {
		case LogEvent::Type::L_TRACE:
			col = ImVec4{ 1.f, 1.f, 1.f, 1.f };
			break;
		case LogEvent::Type::L_INFO:
			col = ImVec4{ 0.2f, 1.f, 0.2f, 1.f };
			break;
		case LogEvent::Type::L_WARN:
			col = ImVec4{ 1.f, 1.f, 0.2f, 1.f };
			break;
		case LogEvent::Type::L_ERROR:
			col = ImVec4{ 1.f, 0.2f, 0.2f, 1.f };
			break;
		case LogEvent::Type::L_CRITICAL:
			col = ImVec4{ 1.f, 0.2f, 0.2f, 1.f };
			break;
		}

		ImGui::TextColored(col, log.content.c_str());
	}

	return m_logs.size();
}

void LoggerUI::Shutdown() {
	Events::EventManager::DeregisterListener(m_log_listener.GetRegisterID());
}

void LoggerUI::AddLog(const std::string& content, LogEvent::Type type) {
	m_logs.push_back(LoggerUI::LogEntry{ .content = content, .type = type });
}

void LoggerUI::OnLogEvent(const LogEvent& _event) {
	m_logs.push_back(LoggerUI::LogEntry{ .content = _event.content, .type =_event.type });
}