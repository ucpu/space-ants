#include <exception>

#include <cage-core/core.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/config.h>
#include <cage-core/assetManager.h>
#include <cage-core/configIni.h>
#include <cage-core/hashString.h>

#include <cage-engine/core.h>
#include <cage-engine/window.h>
#include <cage-engine/engine.h>
#include <cage-engine/engineProfiling.h>
#include <cage-engine/fullscreenSwitcher.h>
#include <cage-engine/highPerformanceGpuHint.h>

using namespace cage;

namespace
{
	bool windowClose()
	{
		engineStop();
		return true;
	}
}

int main(int argc, const char *args[])
{
	try
	{
		holder<logger> log1 = newLogger();
		log1->format.bind<logFormatConsole>();
		log1->output.bind<logOutputStdOut>();

		configSetBool("cage.config.autoSave", true);
		controlThread().timePerTick = 1000000 / 30;
		engineInitialize(engineCreateConfig());
		assets()->add(hashString("ants/ants.pack"));

		eventListener<bool()> windowCloseListener;
		windowCloseListener.bind<&windowClose>();
		window()->events.windowClose.attach(windowCloseListener);
		window()->title("space-ants");

		{
			holder<fullscreenSwitcher> fullscreen = newFullscreenSwitcher({});
			holder<engineProfiling> engineProfiling = newEngineProfiling();
			engineProfiling->profilingScope = engineProfilingScopeEnum::None;

			engineStart();
		}

		assets()->remove(hashString("ants/ants.pack"));
		engineFinalize();
		return 0;
	}
	catch (const cage::exception &e)
	{
		CAGE_LOG(severityEnum::Note, "exception", e.message);
		CAGE_LOG(severityEnum::Error, "exception", "caught cage exception in main");
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(severityEnum::Note, "exception", e.what());
		CAGE_LOG(severityEnum::Error, "exception", "caught std exception in main");
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "exception", "caught unknown exception in main");
	}
	return 1;
}
