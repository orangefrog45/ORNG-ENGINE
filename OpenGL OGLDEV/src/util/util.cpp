#include <glew.h>
#include <glut.h>
#include "util/util.h"

void PrintUtils::PrintDebug(const std::string& text) {
	std::cout << GetFormattedTime() + " " + text << std::endl;
}

void PrintUtils::PrintWarning(const std::string& text) {
	std::cout << "\033[33m";
	PrintDebug("WRN: " + text);
	std::cout << "\033[37m";
}

void PrintUtils::PrintSuccess(const std::string& text) {
	std::cout << "\033[32m";
	PrintDebug("OK: " + text);
	std::cout << "\033[37m";
}

void PrintUtils::PrintError(const std::string& text) {
	std::cout << "\033[31m";
	PrintDebug("ERR: " + text);
	std::cout << "\033[37m";
}

std::string PrintUtils::GetFormattedTime() {
	int time_in_seconds = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	int hours = time_in_seconds / 3600;
	int minutes = (time_in_seconds / 60) - hours * 60;
	int seconds = time_in_seconds - (hours * 3600 + minutes * 60);
	std::string formatted_hours = hours < 10 ? "0" + std::to_string(hours) : std::to_string(hours);
	std::string formatted_minutes = minutes < 10 ? "0" + std::to_string(minutes) : std::to_string(minutes);
	std::string formatted_seconds = seconds < 10 ? "0" + std::to_string(seconds) : std::to_string(seconds);
	return "[" + formatted_hours + ":" + formatted_minutes + ":" + formatted_seconds + "]";
}