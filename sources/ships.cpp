#include <algorithm>
#include <atomic>
#include <random>
#include <vector>

#include <cage-core/concurrent.h>
#include <cage-core/geometry.h>
#include <cage-core/hashString.h>
#include <cage-core/random.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/tasks.h>
#include <cage-core/timer.h>
#include <cage-core/variableSmoothingBuffer.h>

#include "common.h"

uint32 pickTargetPlanet(uint32 shipOwner)
{
	const auto &range = engineEntities()->component<PlanetComponent>()->entities();
	std::vector<Entity *> planets(range.begin(), range.end());
	std::shuffle(planets.begin(), planets.end(), std::default_random_engine((unsigned)detail::randomGenerator().next()));
	for (Entity *e : planets)
	{
		if (e->value<OwnerComponent>().owner != shipOwner)
			return e->id();
	}
	return 0;
}

Real shipSeparation = 0.002;
Real shipTargetShips = 0.005;
Real shipTargetPlanets = 0.001;
Real shipCohesion = 0.003;
Real shipAlignment = 0.001;
Real shipDetectRadius = 3;
Real shipLaserRadius = 2;

namespace
{
	Holder<SpatialStructure> spatialSearchData = newSpatialStructure({});

	uint32 tickIndex = 1;
	Holder<Timer> clock = newTimer();
	VariableSmoothingBuffer<uint64, 512> smoothTimeSpatialBuild;
	VariableSmoothingBuffer<uint64, 512> smoothTimeShipsUpdate;
	VariableSmoothingBuffer<uint64, 512> shipsInteractionRatio;

	struct ShipUpdater
	{
		Entity *e = nullptr;
		TransformComponent &t;
		ShipComponent &s;
		OwnerComponent &owner;
		PhysicsComponent &phys;
		LifeComponent &life;

		static inline Holder<Mutex> mutex = newMutex();
		static inline std::atomic<uint32> shipsInteracted = 0;

		static Vec3 front(const Transform &t) { return t.position + t.orientation * Vec3(0, 0, -1) * t.scale; }

		void operator()()
		{
			thread_local Holder<SpatialQuery> spatialQuery = newSpatialQuery(spatialSearchData.share());

			// destroy ships that wandered too far away
			if (lengthSquared(t.position) > sqr(200))
			{
				life.life = 0;
				return;
			}

			// find all nearby objects
			spatialQuery->intersection(Sphere(t.position, t.scale + shipDetectRadius));
			shipsInteracted += numeric_cast<uint32>(spatialQuery->result().size());
			const uint32 myName = e->id();
			Vec3 avgPos;
			Vec3 avgDir;
			uint32 avgCnt = 0;
			uint32 closestTargetName = 0;
			Real closestTargetDistance = Real::Infinity();
			for (uint32 nearbyName : spatialQuery->result())
			{
				if (nearbyName == myName)
					continue;
				Entity *n = engineEntities()->get(nearbyName);
				const TransformComponent &nt = n->value<TransformComponent>();
				Vec3 d = nt.position - t.position;
				Real l = length(d);
				if (l > 1e-7)
				{
					const Vec3 dn = d / l;
					phys.acceleration -= dn / sqr(max(l - t.scale - nt.scale, 1e-7)) * shipSeparation; // separation
				}
				if (n->has<OwnerComponent>())
				{
					const OwnerComponent &no = n->value<OwnerComponent>();
					if (no.owner == owner.owner)
					{
						// friend
						if (n->has<ShipComponent>())
						{
							avgPos += nt.position;
							const PhysicsComponent &np = n->value<PhysicsComponent>();
							if (lengthSquared(np.velocity) > 1e-10)
								avgDir += normalize(np.velocity);
							avgCnt++;
						}
					}
					else
					{
						// enemy
						if (l < closestTargetDistance)
						{
							closestTargetDistance = l;
							closestTargetName = nearbyName;
						}
					}
				}
			}
			if (avgCnt > 0)
			{
				avgPos = avgPos / avgCnt - t.position;
				if (lengthSquared(avgPos) > 1e-10)
					phys.acceleration += normalize(avgPos) * shipCohesion; // cohesion
				if (lengthSquared(avgDir) > 1e-10)
					phys.acceleration += normalize(avgDir) * shipAlignment; // alignment
			}

			// choose a target to follow
			uint32 targetName = 0;
			if (engineEntities()->exists(s.currentTarget))
				targetName = s.currentTarget;
			else if (closestTargetName)
				targetName = s.currentTarget = closestTargetName;
			else
			{
				// use long-term goal
				if (!engineEntities()->exists(s.longtermTarget))
					s.longtermTarget = pickTargetPlanet(owner.owner);
				targetName = s.longtermTarget;
			}

			// accelerate towards target
			if (engineEntities()->exists(targetName))
			{
				Entity *target = engineEntities()->get(targetName);
				const TransformComponent &targetTransform = target->value<TransformComponent>();
				const Vec3 f = targetTransform.position - t.position;
				const Real l = length(f) - t.scale - targetTransform.scale;
				if (l > 1e-7)
					phys.acceleration += normalize(f) * shipTargetShips;
			}

			// fire at closest enemy
			if (engineEntities()->exists(closestTargetName))
			{
				Entity *target = engineEntities()->get(closestTargetName);
				const TransformComponent &tt = target->value<TransformComponent>();
				const Vec3 o = front(t);
				const Vec3 d = tt.position - o;
				const Real l = length(d);
				if (l < shipLaserRadius + tt.scale)
				{
					// fire at the target
					ScopeLock<Mutex> lock(mutex);
					CAGE_ASSERT(target->has<LifeComponent>());
					target->value<LifeComponent>().life--;
					Entity *laser = engineEntities()->createAnonymous();
					TransformComponent &tr = laser->value<TransformComponent>();
					tr.position = o;
					tr.orientation = Quat(d, Vec3(0, 1, 0));
					tr.scale = l;
					RenderComponent &r = laser->value<RenderComponent>();
					r.object = HashString("ants/laser/laser.obj");
					r.color = e->value<RenderComponent>().color;
					laser->value<TimeoutComponent>().ttl = 1;
				}
			}

			CAGE_ASSERT(phys.acceleration.valid() && phys.velocity.valid());

			{
				// update ship orientation
				if (lengthSquared(phys.acceleration) > 1e-10)
					t.orientation = Quat(normalize(phys.acceleration), t.orientation * Vec3(0, 1, 0));
			}
		}
	};

	const auto engineUpdateListener = controlThread().update.listen(
		[]()
		{
			ProfilingScope profilingTop("ships");
			profilingTop.set(Stringizer() + "count: " + engineEntities()->component<ShipComponent>()->count());

			// add all physics objects into spatial data
			clock->reset();
			{
				const ProfilingScope profiling("spatial data");
				spatialSearchData->clear();
				entitiesVisitor(
					[&](Entity *e, const PhysicsComponent &, const TransformComponent &t)
					{
						if (e->id())
							spatialSearchData->update(e->id(), Sphere(t.position, t.scale));
					},
					engineEntities(), false);
				spatialSearchData->rebuild();
			}
			smoothTimeSpatialBuild.add(clock->elapsed());

			// update ships
			clock->reset();
			{
				const ProfilingScope profiling("update");
				std::vector<ShipUpdater> ships;
				ships.reserve(10000);
				entitiesVisitor([&](Entity *e, TransformComponent &t, ShipComponent &s, OwnerComponent &owner, PhysicsComponent &phys, LifeComponent &life) { ships.push_back({ e, t, s, owner, phys, life }); }, engineEntities(), false);
				tasksRunBlocking<ShipUpdater, 32>("update ships", ships);
				if (!ships.empty())
					shipsInteractionRatio.add(1000 * uint64(ShipUpdater::shipsInteracted) / ships.size());
				ShipUpdater::shipsInteracted = 0;
			}
			smoothTimeShipsUpdate.add(clock->elapsed());

			// log timing
			if (tickIndex++ % 120 == 0)
			{
				CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "Spatial prepare time: " + smoothTimeSpatialBuild.smooth() + " us");
				CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "Ships update time: " + smoothTimeShipsUpdate.smooth() + " us");
				CAGE_LOG(SeverityEnum::Info, "performance", Stringizer() + "Ships interaction ratio: " + Real(shipsInteractionRatio.smooth()) * 0.001);
			}
		},
		30);
}
