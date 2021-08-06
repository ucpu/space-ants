#include "common.h"

#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-engine/fpsCamera.h>
#include <cage-engine/window.h>

namespace
{
	class AutoCamera
	{
		VariableSmoothingBuffer<vec3, 30> a;
		VariableSmoothingBuffer<vec3, 30> b;

		void updatePositions()
		{
			if (engineEntities()->has(shipName))
			{
				Entity *ship = engineEntities()->get(shipName);
				TransformComponent &t = ship->value<TransformComponent>();
				ANTS_COMPONENT(Physics, p, ship);
				a.add(t.position);
				if (lengthSquared(p.velocity) > 1e-7)
					b.add(t.position - normalize(p.velocity) * 5);
			}
			else
			{
				uint32 cnt = ShipComponent::component->count();
				uint32 i = randomRange(0u, cnt);
				shipName = ShipComponent::component->entities()[i]->name();
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
				transform t2 = t;
				t.position = b.smooth();
				t.orientation = quat(a.smooth() - t.position, t.orientation * vec3(0, 1, 0));
				t = interpolate(t2, t, 0.1);
				CameraComponent &c = cam->value<CameraComponent>();
				c.depthOfField.focusDistance = distance(t.position, a.smooth()) * 2;
				c.depthOfField.blendRadius = c.depthOfField.focusDistance * 2;
				c.depthOfField.focusRadius = 0;
			}
		}
	};

	Entity *camera;
	Entity *cameraSkybox;
	Entity *objectSkybox;
	Holder<FpsCamera> manualCamera;
	AutoCamera autoCamera;
	EventListener<void(uint32, ModifiersFlags)> keyPressListener;

	void keyPress(uint32 key, ModifiersFlags)
	{
		if (key == 32)
		{
			CameraComponent &c = camera->value<CameraComponent>();
			if (autoCamera.cam)
			{
				// switch to manual
				autoCamera.cam = nullptr;
				manualCamera->setEntity(camera);
				autoCamera.shipName = 0;
				c.effects &= ~CameraEffectsFlags::DepthOfField;
			}
			else
			{
				// switch to auto
				autoCamera.cam = camera;
				manualCamera->setEntity(nullptr);
				c.effects |= CameraEffectsFlags::DepthOfField;
			}
		}
	}

	void engineInitialize()
	{
		camera = engineEntities()->createUnique();
		{
			TransformComponent &t = camera->value<TransformComponent>();
			t.position = vec3(0, 0, 200);
			CameraComponent &c = camera->value<CameraComponent>();
			c.cameraOrder = 2;
			c.sceneMask = 1;
			c.near = 1;
			c.far = 500;
			c.ambientColor = vec3(1);
			c.ambientIntensity = 0.1;
			c.ambientDirectionalColor = vec3(1);
			c.ambientDirectionalIntensity = 3;
			c.clear = CameraClearFlags::None;
			c.effects = CameraEffectsFlags::Default & ~CameraEffectsFlags::AmbientOcclusion;
			ListenerComponent &ls = camera->value<ListenerComponent>();
		}
		cameraSkybox = engineEntities()->createUnique();
		{
			TransformComponent &t = cameraSkybox->value<TransformComponent>();
			CameraComponent &c = cameraSkybox->value<CameraComponent>();
			c.cameraOrder = 1;
			c.sceneMask = 2;
			c.near = 0.1;
			c.far = 100;
			c.effects |= CameraEffectsFlags::MotionBlur;
		}
		objectSkybox = engineEntities()->createUnique();
		{
			TransformComponent &t = objectSkybox->value<TransformComponent>();
			RenderComponent &r = objectSkybox->value<RenderComponent>();
			r.object = HashString("ants/skybox/skybox.object");
			r.sceneMask = 2;
		}
		manualCamera = newFpsCamera(camera);
		manualCamera->freeMove = true;
		manualCamera->mouseButton = MouseButtonsFlags::Left;
		manualCamera->movementSpeed = 3;
		keyPressListener.attach(engineWindow()->events.keyPress);
		keyPressListener.bind<&keyPress>();
	}

	void engineUpdate()
	{
		TransformComponent &tc = camera->value<TransformComponent>();
		TransformComponent &ts = cameraSkybox->value<TransformComponent>();
		ts.orientation = tc.orientation;
		autoCamera.update();
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize, -200);
			engineInitListener.bind<&engineInitialize>();
			engineUpdateListener.attach(controlThread().update, 200);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}
