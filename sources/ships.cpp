#include "common.h"

#include <cage-core/random.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/geometry.h>
#include <cage-core/threadPool.h>
#include <cage-core/concurrent.h>
#include <cage-core/hashString.h>
#include <cage-core/timer.h>
#include <cage-core/variableSmoothingBuffer.h>

#include <vector>
#include <algorithm>
#include <random>

uint32 pickTargetPlanet(uint32 shipOwner)
{
	auto range = PlanetComponent::component->entities();
	std::vector<Entity*> planets(range.begin(), range.end());
	std::shuffle(planets.begin(), planets.end(), std::default_random_engine((unsigned)detail::getApplicationRandomGenerator().next()));
	for (Entity *e : planets)
	{
		ANTS_COMPONENT(Owner, owner, e);
		if (owner.owner != shipOwner)
			return e->name();
	}
	return 0;
}

real shipSeparation = 0.002;
real shipTargetShips = 0.005;
real shipTargetPlanets = 0.001;
real shipCohesion = 0.003;
real shipAlignment = 0.001;
real shipDetectRadius = 3;
real shipLaserRadius = 2;

namespace
{
	Holder<SpatialStructure> initSpatialData()
	{
		SpatialStructureCreateConfig cfg;
		return newSpatialStructure(cfg);
	}

	Holder<ThreadPool> initThreadPool();

	Holder<SpatialStructure> spatialSearchData = initSpatialData();
	Holder<ThreadPool> threads = initThreadPool();
	Holder<Mutex> mutex = newMutex();

	uint32 tickIndex = 1;
	Holder<Timer> clock = newTimer();
	VariableSmoothingBuffer<uint64, 512> smoothTimeSpatialBuild;
	VariableSmoothingBuffer<uint64, 512> smoothTimeShipsUpdate;
	VariableSmoothingBuffer<uint32, 2048> shipsInteractionRatio;

	vec3 front(const transform &t)
	{
		return t.position + t.orientation * vec3(0, 0, -1) * t.scale;
	}

	struct Laser
	{
		transform tr;
		transform trh;
		vec3 color;
	};

	void shipsUpdateEntry(uint32 thrIndex, uint32 thrCount)
	{
		OPTICK_EVENT("ships");

		Holder<SpatialQuery> SpatialQuery = newSpatialQuery(spatialSearchData.get());
		EntityManager *ents = engineEntities();

		auto entsArr = ShipComponent::component->entities();
		const uint32 entsTotal = ShipComponent::component->count();
		const uint32 entsPerThr = entsTotal / thrCount;
		const uint32 myStart = entsPerThr * thrIndex;
		const uint32 myEnd = thrIndex + 1 == thrCount ? entsTotal : myStart + entsPerThr;

		std::vector<Laser> shots;
		shots.reserve(myEnd - myStart);

		uint32 shipsInteracted = 0;

		for (uint32 entIndex = myStart; entIndex != myEnd; entIndex++)
		{
			Entity *e = entsArr[entIndex];

			CAGE_COMPONENT_ENGINE(Transform, t, e);
			ANTS_COMPONENT(Ship, s, e);
			ANTS_COMPONENT(Owner, owner, e);
			ANTS_COMPONENT(Physics, phys, e);

			// destroy ships that wandered too far away
			if (lengthSquared(t.position) > sqr(200))
			{
				ANTS_COMPONENT(Life, life, e);
				life.life = 0;
				continue;
			}

			// find all nearby objects
			SpatialQuery->intersection(Sphere(t.position, t.scale + shipDetectRadius));
			uint32 myName = e->name();
			vec3 avgPos;
			vec3 avgDir;
			uint32 avgCnt = 0;
			uint32 closestTargetName = 0;
			real closestTargetDistance = real::Infinity();
			shipsInteracted += numeric_cast<uint32>(SpatialQuery->result().size());
			for (uint32 nearbyName : SpatialQuery->result())
			{
				if (nearbyName == myName)
					continue;
				Entity *n = ents->get(nearbyName);
				CAGE_COMPONENT_ENGINE(Transform, nt, n);
				vec3 d = nt.position - t.position;
				real l = length(d);
				if (l > 1e-7)
				{
					vec3 dn = d / l;
					phys.acceleration -= dn / sqr(max(l - t.scale - nt.scale, 1e-7)) * shipSeparation; // separation
				}
				if (n->has(OwnerComponent::component))
				{
					ANTS_COMPONENT(Owner, no, n);
					if (no.owner == owner.owner)
					{
						// friend
						if (n->has(ShipComponent::component))
						{
							ANTS_COMPONENT(Physics, np, n);
							avgPos += nt.position;
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
			if (ents->has(s.currentTarget))
				targetName = s.currentTarget;
			else if (closestTargetName)
				targetName = s.currentTarget = closestTargetName;
			else
			{
				// use long-term goal
				if (!ents->has(s.longtermTarget))
					s.longtermTarget = pickTargetPlanet(owner.owner);
				targetName = s.longtermTarget;
			}

			// accelerate towards target
			if (ents->has(targetName))
			{
				Entity *target = ents->get(targetName);
				CAGE_COMPONENT_ENGINE(Transform, targetTransform, target);
				vec3 f = targetTransform.position - t.position;
				real l = length(f) - t.scale - targetTransform.scale;
				if (l > 1e-7)
					phys.acceleration += normalize(f) * shipTargetShips;
			}

			// fire at closest enemy
			if (ents->has(closestTargetName))
			{
				Entity *target = ents->get(closestTargetName);
				CAGE_COMPONENT_ENGINE(Transform, tt, target);
				vec3 o = front(t);
				vec3 d = tt.position - o;
				real l = length(d);
				if (l < shipLaserRadius + tt.scale)
				{
					// fire at the target
					CAGE_ASSERT(target->has(LifeComponent::component));
					ANTS_COMPONENT(Life, targetLife, target);
					targetLife.life--;
					const transform &th = e->value<TransformComponent>(TransformComponent::componentHistory);
					const transform &tth = target->value<TransformComponent>(TransformComponent::componentHistory);
					Laser laser;
					laser.tr.position = o;
					laser.tr.orientation = quat(d, vec3(0, 1, 0));
					laser.tr.scale = l;
					laser.trh.position = front(th);
					vec3 dh = tth.position - laser.trh.position;
					real lh = length(dh);
					laser.trh.orientation = quat(dh, vec3(0, 1, 0));
					laser.trh.scale = lh;
					CAGE_COMPONENT_ENGINE(Render, render, e);
					laser.color = render.color;
					shots.push_back(laser);
				}
			}

			CAGE_ASSERT(phys.acceleration.valid() && phys.velocity.valid());

			{
				// update ship orientation
				if (lengthSquared(phys.acceleration) > 1e-10)
					t.orientation = quat(normalize(phys.acceleration), t.orientation * vec3(0, 1, 0));
			}
		}

		{
			OPTICK_EVENT("generate lasers");
			ScopeLock<Mutex> lock(mutex);

			// generate lasers
			for (auto &it : shots)
			{
				Entity *e = ents->createAnonymous();
				CAGE_COMPONENT_ENGINE(Transform, t, e);
				t = it.tr;
				transform &th = e->value<TransformComponent>(TransformComponent::componentHistory);
				th = it.trh;
				CAGE_COMPONENT_ENGINE(Render, r, e);
				r.object = HashString("ants/laser/laser.obj");
				r.color = it.color;
				ANTS_COMPONENT(Timeout, ttl, e);
				ttl.ttl = 1;
			}

			// stats
			{
				uint32 cnt = myEnd - myStart;
				if (cnt > 0)
					shipsInteractionRatio.add(1000 * shipsInteracted / cnt);
			}
		}
	}

	Holder<ThreadPool> initThreadPool()
	{
		auto thrs = newThreadPool("ships ", processorsCount() - 1);
		thrs->function.bind<&shipsUpdateEntry>();
		return thrs;
	}

	void engineUpdate()
	{
		OPTICK_EVENT("ships");

		// add all physics objects into spatial data
		clock->reset();
		{
			OPTICK_EVENT("spatial update");
			spatialSearchData->clear();
			for (Entity *e : PhysicsComponent::component->entities())
			{
				if (e->name() == 0)
					continue;
				CAGE_COMPONENT_ENGINE(Transform, t, e);
				spatialSearchData->update(e->name(), Sphere(t.position, t.scale));
			}
		}
		{
			OPTICK_EVENT("spatial rebuild");
			spatialSearchData->rebuild();
		}
		smoothTimeSpatialBuild.add(clock->microsSinceLast());

		// update ships
		clock->reset();
		{
			OPTICK_EVENT("ships update");
			threads->run();
		}
		smoothTimeShipsUpdate.add(clock->microsSinceLast());

		// log timing
		if (tickIndex++ % 120 == 0)
		{
			CAGE_LOG(SeverityEnum::Info, "performance", stringizer() + "Spatial prepare time: " + smoothTimeSpatialBuild.smooth() + " us");
			CAGE_LOG(SeverityEnum::Info, "performance", stringizer() + "Ships update time: " + smoothTimeShipsUpdate.smooth() + " us");
			CAGE_LOG(SeverityEnum::Info, "performance", stringizer() + "Ships interaction ratio: " + real(shipsInteractionRatio.smooth()) * 0.001);
		}
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineUpdateListener.attach(controlThread().update, 30);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}
