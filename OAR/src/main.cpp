#include "pch/pch.h"

#include "Application.h"
#include "util/Log.h"

int main() {
	Log::Init();
	Application application;
	application.Init();
	return 0;
}
