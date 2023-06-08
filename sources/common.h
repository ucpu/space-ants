#ifndef ants_common_h_sdg456ds4hg6
#define ants_common_h_sdg456ds4hg6

#include <cage-core/entities.h>
#include <cage-core/math.h>
#include <cage-core/profiling.h>
#include <cage-engine/scene.h>
#include <cage-simple/engine.h>

#include <atomic>

using namespace cage;

struct PhysicsComponent
{
	Quat rotation;
	Vec3 velocity;
	Vec3 acceleration;
	Real maxSpeed = 0.1;
};

struct OwnerComponent
{
	uint32 owner = 0;
};

struct LifeComponent
{
	sint32 life = 0; // this used to be atomic - todo fix
};

struct ShipComponent
{
	uint32 currentTarget = 0;
	uint32 longtermTarget = 0;
};

struct PlanetComponent
{
	uint32 batch = 0; // number of ships to spawn
};

struct TimeoutComponent
{
	sint32 ttl = 1;
};

extern EntityGroup *entitiesToDestroy;

uint32 pickTargetPlanet(uint32 shipOwner);

extern Real shipSeparation;
extern Real shipTargetShips;
extern Real shipTargetPlanets;
extern Real shipCohesion;
extern Real shipAlignment;
extern Real shipDetectRadius;
extern Real shipLaserRadius;

#endif
