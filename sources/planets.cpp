#include "common.h"

#include <cage-core/color.h>
#include <cage-core/hashString.h>

#include <vector>

namespace
{
	constexpr const uint32 modelNames[] = {
		HashString("ants/planets/farcodev/0.object"),
		HashString("ants/planets/farcodev/1.object"),
		HashString("ants/planets/farcodev/2.object"),
		HashString("ants/planets/farcodev/3.object"),
		HashString("ants/planets/farcodev/4.object"),
		HashString("ants/planets/farcodev/5.object"),
		HashString("ants/planets/farcodev/6.object"),
		HashString("ants/planets/farcodev/7.object"),
	};

#ifdef CAGE_DEBUG
	constexpr uint32 shipsLimit = 200;
	constexpr uint32 batchScale = 1;
#else
	constexpr uint32 shipsLimit = 5000;
	constexpr uint32 batchScale = 10;
#endif // CAGE_DEBUG

	const auto engineInitListener = controlThread().initialize.listen(
		[]()
		{
			uint32 playersCount = randomRange(2u, 5u);
			std::vector<Vec3> playerColors;
			playerColors.resize(playersCount);
			Real hueOff = randomChance();
			for (uint32 p = 0; p < playersCount; p++)
				playerColors[p] = colorHsvToRgb(Vec3((Real(p) / playersCount + hueOff) % 1, randomRange(0.5, 1.0), randomRange(0.5, 1.0)));
			uint32 planetsCount = randomRange(3u, 12u);
			for (uint32 p = 0; p < planetsCount; p++)
			{
				Entity *e = engineEntities()->createUnique();
				TransformComponent &t = e->value<TransformComponent>();
				t.position = randomDirection3() * randomRange(80, 150);
				t.orientation = randomDirectionQuat();
				t.scale = 5;
				OwnerComponent &owner = e->value<OwnerComponent>();
				owner.owner = p % playersCount;
				RenderComponent &r = e->value<RenderComponent>();
				r.object = modelNames[randomRange(0u, (uint32)(sizeof(modelNames) / sizeof(modelNames[0])))];
				r.color = playerColors[owner.owner];
				PhysicsComponent &physics = e->value<PhysicsComponent>();
				physics.rotation = interpolate(Quat(), randomDirectionQuat(), 0.0003);
				LifeComponent &life = e->value<LifeComponent>();
				life.life = randomRange(1000000, 2000000);
				PlanetComponent &planet = e->value<PlanetComponent>();
				planet.batch = randomRange(3 * batchScale, 5 * batchScale);
			}
		},
		-50);

	void createShip(Entity *planet, uint32 target)
	{
		Entity *e = engineEntities()->createUnique();
		TransformComponent &planetTransform = planet->value<TransformComponent>();
		TransformComponent &t = e->value<TransformComponent>();
		t.scale = 0.3;
		t.position = planetTransform.position + randomDirection3() * (t.scale + planetTransform.scale + 1e-5);
		t.orientation = randomDirectionQuat();
		OwnerComponent &planetOwner = (planet)->value<OwnerComponent>();
		OwnerComponent &owner = e->value<OwnerComponent>();
		owner.owner = planetOwner.owner;
		RenderComponent &planetRender = planet->value<RenderComponent>();
		RenderComponent &r = e->value<RenderComponent>();
		r.color = planetRender.color;
		r.object = HashString("ants/ships/1/1.object");
		PhysicsComponent &physics = e->value<PhysicsComponent>();
		LifeComponent &life = e->value<LifeComponent>();
		life.life = randomRange(200, 300);
		ShipComponent &ship = e->value<ShipComponent>();
		ship.longtermTarget = target;
	}

	uint32 planetIndex = 0;

	const auto engineUpdateListener = controlThread().update.listen(
		[]()
		{
			ProfilingScope profiling("planets");
			uint32 shipsCount = engineEntities()->component<ShipComponent>()->count();
			uint32 planetsCount = engineEntities()->component<PlanetComponent>()->count();
			uint32 currentIndex = 0;
			for (Entity *e : engineEntities()->component<PlanetComponent>()->entities())
			{
				PlanetComponent &p = e->value<PlanetComponent>();
				if (currentIndex++ != (planetIndex % planetsCount))
					continue;
				if (shipsCount + p.batch > shipsLimit)
					continue;
				OwnerComponent &owner = e->value<OwnerComponent>();
				uint32 target = pickTargetPlanet(owner.owner);
				for (uint32 s = 0; s < p.batch; s++)
					createShip(e, target);
				p.batch = randomRange(3 * batchScale, 5 * batchScale);
				planetIndex++;
			}
		},
		60);
}
