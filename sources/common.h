#ifndef ants_common_h_sdg456ds4hg6
#define ants_common_h_sdg456ds4hg6

#include <cage-core/math.h>
#include <cage-core/entities.h>
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
	real maxSpeed = 0.1;
};

struct OwnerComponent
{
	static EntityComponent *component;

	uint32 owner = 0;
};

struct LifeComponent
{
	static EntityComponent *component;

	sint32 life = 0; // this used to be atomic - todo fix
};

struct ShipComponent
{
	static EntityComponent *component;

	uint32 currentTarget = 0;
	uint32 longtermTarget = 0;
};

struct PlanetComponent
{
	static EntityComponent *component;

	uint32 batch = 0; // number of ships to spawn
};

struct TimeoutComponent
{
	static EntityComponent *component;

	sint32 ttl = 1;
};

extern EntityGroup *entitiesToDestroy;

#define ANTS_COMPONENT(T,C,E) ::T##Component &C = (E)->value<::T##Component>(::T##Component::component);

uint32 pickTargetPlanet(uint32 shipOwner);

extern real shipSeparation;
extern real shipTargetShips;
extern real shipTargetPlanets;
extern real shipCohesion;
extern real shipAlignment;
extern real shipDetectRadius;
extern real shipLaserRadius;

#endif
