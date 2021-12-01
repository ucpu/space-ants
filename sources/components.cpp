#include "common.h"

namespace
{
	void engineInitialize()
	{
		engineEntities()->defineComponent(PhysicsComponent());
		engineEntities()->defineComponent(OwnerComponent());
		engineEntities()->defineComponent(LifeComponent());
		engineEntities()->defineComponent(ShipComponent());
		engineEntities()->defineComponent(PlanetComponent());
		engineEntities()->defineComponent(TimeoutComponent());
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
