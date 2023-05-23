#include <cage-core/logger.h>
#include <cage-core/math.h>
#include <cage-core/config.h>
#include <cage-core/assetManager.h>
#include <cage-core/ini.h>
#include <cage-core/hashString.h>

#include <cage-engine/window.h>
#include <cage-engine/highPerformanceGpuHint.h>
#include <cage-simple/engine.h>
#include <cage-simple/statisticsGui.h>
#include <cage-simple/fullscreenSwitcher.h>

using namespace cage;

namespace
{
	void windowClose(InputWindow)
	{
		engineStop();
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

		const auto closeListener = engineWindow()->events.listen(inputListener<InputClassEnum::WindowClose, InputWindow>([](auto) { engineStop(); }));
		engineWindow()->title("space-ants");

		{
			Holder<FullscreenSwitcher> fullscreen = newFullscreenSwitcher({});
			Holder<StatisticsGui> engineStatistics = newStatisticsGui();
			engineStatistics->statisticsScope = StatisticsGuiScopeEnum::None;

			engineRun();
		}

		engineAssets()->remove(HashString("ants/ants.pack"));
		engineFinalize();
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
