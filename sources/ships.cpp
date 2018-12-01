#include "common.h"

namespace
{
	void engineUpdate()
	{
		for (entityClass *e : shipComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			GAME_GET_COMPONENT(ship, s, e);
			// todo process ships
		}
	}

	class callbacksInitClass
	{
		eventListener<void()> engineUpdateListener;
	public:
		callbacksInitClass()
		{
			engineUpdateListener.attach(controlThread().update, 30);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
