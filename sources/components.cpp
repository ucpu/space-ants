#include "common.h"

namespace
{
	const auto engineInitListener = controlThread().initialize.listen([]() {
		engineEntities()->defineComponent(PhysicsComponent());
		engineEntities()->defineComponent(OwnerComponent());
		engineEntities()->defineComponent(LifeComponent());
		engineEntities()->defineComponent(ShipComponent());
		engineEntities()->defineComponent(PlanetComponent());
		engineEntities()->defineComponent(TimeoutComponent());
	}, -100);
}
