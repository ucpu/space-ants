#include "common.h"

namespace
{
	void engineInitialize()
	{
		// todo generate some planets
	}

	void engineUpdate()
	{
		for (entityClass *e : planetComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			GAME_GET_COMPONENT(planet, p, e);
			// todo process planets
		}
	}

	class callbacksInitClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
	public:
		callbacksInitClass()
		{
			engineInitListener.attach(controlThread().initialize, -50);
			engineInitListener.bind<&engineInitialize>();
			engineUpdateListener.attach(controlThread().update, 60);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
