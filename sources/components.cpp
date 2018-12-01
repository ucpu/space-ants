#include "common.h"

#define COMPONENTS_LIST physicsComponent, ownerComponent, lifeComponent, shipComponent, planetComponent, timeoutComponent

#define GCHL_GENERATE(N) componentClass *N::component;
CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, COMPONENTS_LIST))
#undef GCHL_GENERATE

physicsComponent::physicsComponent()
{}

ownerComponent::ownerComponent() : owner(0)
{}

lifeComponent::lifeComponent() : life(0)
{}

shipComponent::shipComponent() : currentTarget(0), longtermTarget(0)
{}

planetComponent::planetComponent() : production(0), resources(0), batch(0)
{}

timeoutComponent::timeoutComponent() : ttl(1)
{}

namespace
{
	void engineInitialize()
	{
#define GCHL_GENERATE(N) N::component = entities()->defineComponent(N(), true);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, COMPONENTS_LIST))
#undef GCHL_GENERATE
	}

	class callbacksInitClass
	{
		eventListener<void()> engineInitListener;
	public:
		callbacksInitClass()
		{
			engineInitListener.attach(controlThread().initialize, -100);
			engineInitListener.bind<&engineInitialize>();
		}
	} callbacksInitInstance;
}
