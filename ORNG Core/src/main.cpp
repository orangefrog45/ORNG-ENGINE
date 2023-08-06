#include "pch/pch.h"

#include "core/Application.h"
#include "util/Log.h"

int main() {
	ORNG::Log::Init();
	ORNG::Application application;
	application.Init();
	return 0;
}
