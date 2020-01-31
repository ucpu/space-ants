#include "common.h"

#include <cage-core/macros.h>

#define COMPONENTS_LIST PhysicsComponent, OwnerComponent, LifeComponent, ShipComponent, PlanetComponent, TimeoutComponent

#define GCHL_GENERATE(N) EntityComponent *N::component;
CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, COMPONENTS_LIST))
#undef GCHL_GENERATE

PhysicsComponent::PhysicsComponent() : maxSpeed(0.1)
{}

OwnerComponent::OwnerComponent() : owner(0)
{}

LifeComponent::LifeComponent() : life(0)
{}

ShipComponent::ShipComponent() : currentTarget(0), longtermTarget(0)
{}

PlanetComponent::PlanetComponent() : batch(0)
{}

TimeoutComponent::TimeoutComponent() : ttl(1)
{}

namespace
{
	void engineInitialize()
	{
#define GCHL_GENERATE(N) N::component = engineEntities()->defineComponent(N(), true);
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
