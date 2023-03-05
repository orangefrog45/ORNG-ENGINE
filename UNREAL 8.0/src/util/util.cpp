#include <glew.h>
#include <glfw/glfw3.h>
#include <windows.h>
#include "util/util.h"

void PrintUtils::PrintDebug(const std::string& text) {
	std::cout << GetFormattedTime() + " " + text << std::endl;
}

void PrintUtils::PrintWarning(const std::string& text) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x06);
	PrintDebug("WRN: " + text);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
}

void PrintUtils::PrintSuccess(const std::string& text) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0A);
	PrintDebug("OK: " + text);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
}

void PrintUtils::PrintError(const std::string& text) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x04);
	PrintDebug("ERR: " + text);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
}

std::string PrintUtils::RoundDouble(double value) {
	std::string value1 = std::to_string(value);
	return value1.substr(0, value1.find(".") + 2);
}

std::string PrintUtils::GetFormattedTime() {
	int time_in_seconds = glfwGetTime() / 1000.0f;
	int hours = time_in_seconds / 3600;
	int minutes = (time_in_seconds / 60) - hours * 60;
	int seconds = time_in_seconds - (hours * 3600 + minutes * 60);
	std::string formatted_hours = hours < 10 ? "0" + std::to_string(hours) : std::to_string(hours);
	std::string formatted_minutes = minutes < 10 ? "0" + std::to_string(minutes) : std::to_string(minutes);
	std::string formatted_seconds = seconds < 10 ? "0" + std::to_string(seconds) : std::to_string(seconds);
	return "[" + formatted_hours + ":" + formatted_minutes + ":" + formatted_seconds + "]";
}