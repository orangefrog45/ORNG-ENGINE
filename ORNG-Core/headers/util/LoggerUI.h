#pragma once
#include "util/ExtraUI.h"
#include "util/Log.h"
#include "events/Events.h"

namespace ORNG {
	class LoggerUI {
	public:
		void Init();

		// This should be called inside an ImGui window/child, it will draw formatted strings there
		// Returns the total number of logs drawn (NOT just those that are visible)
		unsigned RenderLogContentsWithImGui();

		inline void ClearLogs() {
			m_logs.clear();
		}

		void AddLog(const std::string& content, LogEvent::Type type);

		void Shutdown();
	private:
		void OnLogEvent(const LogEvent& _event);

		struct LogEntry {
			std::string content;
			LogEvent::Type type;
		};

		std::vector<LogEntry> m_logs;
		Events::EventListener<LogEvent> m_log_listener;
	};
}