#ifndef ants_common_h_sdg456ds4hg6
#define ants_common_h_sdg456ds4hg6

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>

#include <cage-engine/core.h>
#include <cage-engine/engine.h>

#include <optick.h>

#include <atomic>

using namespace cage;

struct PhysicsComponent
{
	static EntityComponent *component;

	quat rotation;
	vec3 velocity;
	vec3 acceleration;
	real maxSpeed;

	PhysicsComponent();
};

struct OwnerComponent
{
	static EntityComponent *component;

	uint32 owner;

	OwnerComponent();
};

struct LifeComponent
{
	static EntityComponent *component;

	std::atomic<sint32> life;

	LifeComponent();
};

struct ShipComponent
{
	static EntityComponent *component;

	uint32 currentTarget;
	uint32 longtermTarget;

	ShipComponent();
};

struct PlanetComponent
{
	static EntityComponent *component;

	uint32 batch; // number of ships to spawn

	PlanetComponent();
};

struct TimeoutComponent
{
	static EntityComponent *component;

	sint32 ttl;

	TimeoutComponent();
};

extern EntityGroup *entitiesToDestroy;

#define ANTS_COMPONENT(T,C,E) ::CAGE_JOIN(T, Component) &C = (E)->value<::CAGE_JOIN(T, Component)>(::CAGE_JOIN(T, Component)::component);

uint32 pickTargetPlanet(uint32 shipOwner);

extern real shipSeparation;
extern real shipTargetShips;
extern real shipTargetPlanets;
extern real shipCohesion;
extern real shipAlignment;
extern real shipDetectRadius;
extern real shipLaserRadius;

#endif
