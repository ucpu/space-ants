#include <vector>

#include "common.h"

#include <cage-core/hashString.h>
#include <cage-core/color.h>

namespace
{
	const uint32 modelNames[] = {
		hashString("ants/planets/farcodev/0.object"),
		hashString("ants/planets/farcodev/1.object"),
		hashString("ants/planets/farcodev/2.object"),
		hashString("ants/planets/farcodev/3.object"),
		hashString("ants/planets/farcodev/4.object"),
		hashString("ants/planets/farcodev/5.object"),
		hashString("ants/planets/farcodev/6.object"),
		hashString("ants/planets/farcodev/7.object"),
	};

	void engineInitialize()
	{
		uint32 playersCount = randomRange(2u, 5u);
		std::vector<vec3> playerColors;
		playerColors.resize(playersCount);
		for (uint32 p = 0; p < playersCount; p++)
			playerColors[p] = convertHsvToRgb(vec3(randomChance(), randomRange(0.5, 1.0), randomRange(0.5, 1.0)));
		uint32 planetsCount = randomRange(3u, 12u);
		for (uint32 p = 0; p < planetsCount; p++)
		{
			entityClass *e = entities()->createUnique();
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = randomDirection3() * randomRange(50, 100);
			t.orientation = randomDirectionQuat();
			t.scale = randomRange(1.0, 5.0);
			GAME_GET_COMPONENT(owner, owner, e);
			owner.owner = p % playersCount;
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = modelNames[randomRange(0u, (uint32)(sizeof(modelNames) / sizeof(modelNames[0])))];
			r.color = playerColors[owner.owner];
			GAME_GET_COMPONENT(physics, physics, e);
			physics.radius = t.scale;
			physics.rotation = interpolate(quat(), randomDirectionQuat(), 0.0003);
			GAME_GET_COMPONENT(life, life, e);
			life.life = randomRange(1000000, 2000000);
			GAME_GET_COMPONENT(planet, planet, e);
			planet.production = randomRange(10, 20);
			planet.resources = randomRange(100, 1000);
		}
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
