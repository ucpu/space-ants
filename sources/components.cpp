#include "common.h"

#include <cage-core/macros.h>

#define COMPONENTS_LIST PhysicsComponent, OwnerComponent, LifeComponent, ShipComponent, PlanetComponent, TimeoutComponent

#define GCHL_GENERATE(N) EntityComponent *N::component;
CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, COMPONENTS_LIST))
#undef GCHL_GENERATE

namespace
{
	void engineInitialize()
	{
#define GCHL_GENERATE(N) N::component = engineEntities()->defineComponent(N());
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, COMPONENTS_LIST))
#undef GCHL_GENERATE
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize, -100);
			engineInitListener.bind<&engineInitialize>();
		}
	} callbacksInitInstance;
}
