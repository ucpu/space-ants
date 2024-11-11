#include <cage-core/assetsManager.h>
#include <cage-core/hashString.h>
#include <cage-core/logger.h>
#include <cage-engine/highPerformanceGpuHint.h>
#include <cage-engine/window.h>
#include <cage-simple/engine.h>
#include <cage-simple/fullscreenSwitcher.h>
#include <cage-simple/statisticsGui.h>

using namespace cage;

int main(int argc, const char *args[])
{
	try
	{
		initializeConsoleLogger();
		engineInitialize(EngineCreateConfig());
		controlThread().updatePeriod(1'000'000 / 30);
		engineAssets()->load(HashString("ants/ants.pack"));

		const auto closeListener = engineWindow()->events.listen(inputFilter([](input::WindowClose) { engineStop(); }));
		engineWindow()->title("space-ants");

		{
			Holder<FullscreenSwitcher> fullscreen = newFullscreenSwitcher({});
			Holder<StatisticsGui> engineStatistics = newStatisticsGui();

			engineRun();
		}

		engineAssets()->unload(HashString("ants/ants.pack"));
		engineFinalize();
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
