#include <cage-core/core.h>
#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/config.h>
#include <cage-core/assetManager.h>
#include <cage-core/ini.h>
#include <cage-core/hashString.h>

#include <cage-engine/core.h>
#include <cage-engine/window.h>
#include <cage-engine/engine.h>
#include <cage-engine/engineProfiling.h>
#include <cage-engine/fullscreenSwitcher.h>
#include <cage-engine/highPerformanceGpuHint.h>

#include <exception>

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
		Holder<Logger> log1 = newLogger();
		log1->format.bind<logFormatConsole>();
		log1->output.bind<logOutputStdOut>();

		configSetBool("cage/config/autoSave", true);
		engineInitialize(EngineCreateConfig());
		controlThread().updatePeriod(1000000 / 30);
		engineAssets()->add(HashString("ants/ants.pack"));

		EventListener<bool()> windowCloseListener;
		windowCloseListener.bind<&windowClose>();
		engineWindow()->events.windowClose.attach(windowCloseListener);
		engineWindow()->title("space-ants");

		{
			Holder<FullscreenSwitcher> fullscreen = newFullscreenSwitcher({});
			Holder<EngineProfiling> EngineProfiling = newEngineProfiling();
			EngineProfiling->profilingScope = EngineProfilingScopeEnum::None;

			engineStart();
		}

		engineAssets()->remove(HashString("ants/ants.pack"));
		engineFinalize();
		return 0;
	}
	catch (const cage::Exception &e)
	{
		CAGE_LOG(SeverityEnum::Note, "exception", e.message);
		CAGE_LOG(SeverityEnum::Error, "exception", "caught cage exception in main");
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(SeverityEnum::Note, "exception", e.what());
		CAGE_LOG(SeverityEnum::Error, "exception", "caught std exception in main");
	}
	catch (...)
	{
		CAGE_LOG(SeverityEnum::Error, "exception", "caught unknown exception in main");
	}
	return 1;
}
