#include "Application.h"
#include "Log.h"

int main() {
	Log::Init();
	Application application;
	application.Init();
	return 0;
}
