#include "common.h"

#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-engine/sceneScreenSpaceEffects.h>
#include <cage-engine/window.h>
#include <cage-simple/fpsCamera.h>

namespace
{
	class AutoCamera
	{
		VariableSmoothingBuffer<Vec3, 30> a;
		VariableSmoothingBuffer<Vec3, 30> b;

		void updatePositions()
		{
			if (engineEntities()->has(shipName))
			{
				Entity *ship = engineEntities()->get(shipName);
				TransformComponent &t = ship->value<TransformComponent>();
				PhysicsComponent &p = ship->value<PhysicsComponent>();
				a.add(t.position);
				if (lengthSquared(p.velocity) > 1e-7)
					b.add(t.position - normalize(p.velocity) * 5);
			}
			else
			{
				uint32 cnt = engineEntities()->component<ShipComponent>()->count();
				uint32 i = randomRange(0u, cnt);
				shipName = engineEntities()->component<ShipComponent>()->entities()[i]->name();
			}
		}

	public:
		Entity *cam = nullptr;
		uint32 shipName = 0;

		void update()
		{
			updatePositions();
			if (cam)
			{
				TransformComponent &t = cam->value<TransformComponent>();
				Transform t2 = t;
				t.position = b.smooth();
				t.orientation = Quat(a.smooth() - t.position, t.orientation * Vec3(0, 1, 0));
				t = interpolate(t2, t, 0.1);
				ScreenSpaceEffectsComponent &c = cam->value<ScreenSpaceEffectsComponent>();
				c.depthOfField.focusDistance = distance(t.position, a.smooth()) * 2;
				c.depthOfField.blendRadius = c.depthOfField.focusDistance * 2;
				c.depthOfField.focusRadius = 0;
			}
		}
	};

	Entity *camera;
	Holder<FpsCamera> manualCamera;
	AutoCamera autoCamera;

	void keyPress(input::KeyPress in)
	{
		if (in.key == 32)
		{
			ScreenSpaceEffectsComponent &c = camera->value<ScreenSpaceEffectsComponent>();
			if (autoCamera.cam)
			{
				// switch to manual
				autoCamera.cam = nullptr;
				manualCamera->setEntity(camera);
				autoCamera.shipName = 0;
				c.effects &= ~ScreenSpaceEffectsFlags::DepthOfField;
			}
			else
			{
				// switch to auto
				autoCamera.cam = camera;
				manualCamera->setEntity(nullptr);
				c.effects |= ScreenSpaceEffectsFlags::DepthOfField;
			}
		}
	}

	EventListener<bool(const GenericInput &)> keyPressListener = engineEvents().listen(inputFilter(keyPress));

	const auto engineInitListener = controlThread().initialize.listen(
		[]()
		{
			camera = engineEntities()->createUnique();
			{
				camera->value<TransformComponent>().position = Vec3(0, 0, 200);
				CameraComponent &c = camera->value<CameraComponent>();
				c.near = 1;
				c.far = 500;
				c.ambientColor = Vec3(1);
				c.ambientIntensity = 0.1;
				LightComponent &l = camera->value<LightComponent>();
				l.lightType = LightTypeEnum::Directional;
				l.color = Vec3(1);
				l.intensity = 3;
				l.ssaoFactor = 1;
				camera->value<ScreenSpaceEffectsComponent>().effects = ScreenSpaceEffectsFlags::Default & ~ScreenSpaceEffectsFlags::AmbientOcclusion;
				camera->value<ListenerComponent>();
			}
			{
				Entity *skybox = engineEntities()->createUnique();
				skybox->value<TransformComponent>();
				skybox->value<RenderComponent>().object = HashString("ants/skybox/skybox.object");
			}
			manualCamera = newFpsCamera(camera);
			manualCamera->freeMove = true;
			manualCamera->mouseButton = MouseButtonsFlags::Left;
			manualCamera->movementSpeed = 3;
		},
		-200);

	const auto engineUpdateListener = controlThread().update.listen([]() { autoCamera.update(); }, 200);
}
