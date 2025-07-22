#include <cage-core/color.h>
#include <cage-core/hashString.h>

#include "common.h"

namespace
{
	Vec3 colorVariation(const Vec3 &c)
	{
		Vec3 hsv = colorRgbToHsv(c);
		hsv[0] += randomRange(0.8, 1.2);
		hsv[0] = hsv[0] % 1;
		hsv[1] += randomRange(-0.2, 0.2);
		hsv[2] += randomRange(-0.2, 0.2);
		hsv = clamp(hsv, 0, 1);
		return colorHsvToRgb(hsv);
	}

	void shipExplode(Entity *ship)
	{
		const TransformComponent &st = ship->value<TransformComponent>();
		const ModelComponent &sr = ship->value<ModelComponent>();
		const ColorComponent &sc = ship->value<ColorComponent>();
		const PhysicsComponent &sp = ship->value<PhysicsComponent>();
		uint32 cnt = randomRange(4, 7);
		for (uint32 i = 0; i < cnt; i++)
		{
			Entity *e = engineEntities()->createAnonymous();
			TransformComponent &t = e->value<TransformComponent>();
			t.scale = st.scale;
			t.position = st.position + randomDirection3() * st.scale;
			t.orientation = randomDirectionQuat();
			ModelComponent &r = e->value<ModelComponent>();
			r.model = HashString("ants/explosion/particle.blend");
			ColorComponent &c = e->value<ColorComponent>();
			c.color = colorVariation(sc.color);
			c.intensity = 2;
			AnimationSpeedComponent &at = e->value<AnimationSpeedComponent>();
			at.speed = randomRange(0.7, 1.5);
			e->value<PhysicsComponent>().velocity = randomDirection3() * t.scale * 0.07 + sp.velocity;
			e->value<TimeoutComponent>().ttl = numeric_cast<sint32>(Real(30) / at.speed);
		}
	}

	const auto engineUpdateListener = controlThread().update.listen(
		[]()
		{
			{
				const ProfilingScope profiling("timeout");
				for (Entity *e : engineEntities()->component<TimeoutComponent>()->entities())
				{
					TimeoutComponent &t = e->value<TimeoutComponent>();
					if (t.ttl-- <= 0)
						e->value<DestroyingComponent>();
				}
			}
			{
				const ProfilingScope profiling("explosions");
				for (Entity *e : engineEntities()->component<LifeComponent>()->entities())
				{
					LifeComponent &l = e->value<LifeComponent>();
					if (l.life <= 0)
					{
						e->value<DestroyingComponent>();
						if (e->has<ShipComponent>())
							shipExplode(e);
					}
				}
			}
			engineEntities()->component<DestroyingComponent>()->destroy();
		},
		90);
}
