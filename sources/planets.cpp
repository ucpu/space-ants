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

#ifdef CAGE_DEBUG
	const uint32 shipsLimit = 300;
#else
	const uint32 shipsLimit = 3000;
#endif // CAGE_DEBUG

	const uint32 shipCost = 100;

	void engineInitialize()
	{
		uint32 playersCount = randomRange(2u, 5u);
		std::vector<vec3> playerColors;
		playerColors.resize(playersCount);
		real hueOff = randomChance();
		for (uint32 p = 0; p < playersCount; p++)
			playerColors[p] = convertHsvToRgb(vec3((real(p) / playersCount + hueOff) % 1, randomRange(0.5, 1.0), randomRange(0.5, 1.0)));
		uint32 planetsCount = randomRange(3u, 12u);
		for (uint32 p = 0; p < planetsCount; p++)
		{
			entityClass *e = entities()->createUnique();
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = randomDirection3() * randomRange(80, 150);
			t.orientation = randomDirectionQuat();
			t.scale = 5;
			GAME_GET_COMPONENT(owner, owner, e);
			owner.owner = p % playersCount;
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = modelNames[randomRange(0u, (uint32)(sizeof(modelNames) / sizeof(modelNames[0])))];
			r.color = playerColors[owner.owner];
			GAME_GET_COMPONENT(physics, physics, e);
			physics.rotation = interpolate(quat(), randomDirectionQuat(), 0.0003);
			GAME_GET_COMPONENT(life, life, e);
			life.life = randomRange(1000000, 2000000);
			GAME_GET_COMPONENT(planet, planet, e);
			planet.production = randomRange(40, 50);
			planet.resources = randomRange(40, 100) * shipCost;
			planet.batch = 50 * shipCost;
		}
	}

	void createShip(entityClass *planet, uint32 target)
	{
		entityClass *e = entities()->createUnique();
		ENGINE_GET_COMPONENT(transform, planetTransform, planet);
		ENGINE_GET_COMPONENT(transform, t, e);
		t.scale = 0.3;
		t.position = planetTransform.position + randomDirection3() * (t.scale + planetTransform.scale + 1e-5);
		t.orientation = randomDirectionQuat();
		GAME_GET_COMPONENT(owner, planetOwner, planet);
		GAME_GET_COMPONENT(owner, owner, e);
		owner.owner = planetOwner.owner;
		ENGINE_GET_COMPONENT(render, planetRender, planet);
		ENGINE_GET_COMPONENT(render, r, e);
		r.color = planetRender.color;
		r.object = hashString("ants/ships/1/1.object");
		GAME_GET_COMPONENT(physics, physics, e);
		GAME_GET_COMPONENT(life, life, e);
		life.life = 50;
		GAME_GET_COMPONENT(ship, ship, e);
		ship.longtermTarget = target;
	}

	uint32 planetIndex = 0;

	void engineUpdate()
	{
		uint32 shipsCount = shipComponent::component->group()->count();
		uint32 planetsCount = planetComponent::component->group()->count();
		for (entityClass *e : planetComponent::component->entities())
		{
			GAME_GET_COMPONENT(planet, p, e);
			p.resources += p.production;
			p.resources = min(p.resources, p.batch);
			if ((e->name() % planetsCount) != (planetIndex % planetsCount))
				continue;
			if (p.resources == p.batch)
			{
				uint32 create = p.resources / shipCost;
				if (shipsCount + create > shipsLimit)
					continue;
				p.resources -= create * shipCost;
				p.batch = randomRange(10, 30) * shipCost;
				GAME_GET_COMPONENT(owner, owner, e);
				uint32 target = pickTargetPlanet(owner.owner);
				for (uint32 s = 0; s < create; s++)
					createShip(e, target);
			}
			planetIndex++;
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
