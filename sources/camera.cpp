#include "common.h"

#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-client/cameraController.h>
#include <cage-client/window.h>

namespace
{
	class autoCameraClass
	{

		variableSmoothingBuffer<vec3, 30> a;
		variableSmoothingBuffer<vec3, 30> b;

		void updatePositions()
		{
			if (entities()->has(shipName))
			{
				entityClass *ship = entities()->get(shipName);
				ENGINE_GET_COMPONENT(transform, t, ship);
				GAME_GET_COMPONENT(physics, p, ship);
				a.add(t.position);
				if (p.velocity.squaredLength() > 1e-7)
					b.add(t.position - p.velocity.normalize() * 5);
			}
			else
			{
				uint32 cnt = shipComponent::component->group()->count();
				uint32 i = randomRange(0u, cnt);
				shipName = shipComponent::component->group()->array()[i]->name();
			}
		}

	public:
		entityClass *cam;
		uint32 shipName;

		autoCameraClass() : cam(0), shipName(0)
		{}

		void update()
		{
			updatePositions();
			if (cam)
			{
				ENGINE_GET_COMPONENT(transform, t, cam);
				transform t2 = t;
				t.position = b.smooth();
				t.orientation = quat(a.smooth() - t.position, t.orientation * vec3(0, 1, 0));
				t = interpolate(t2, t, 0.1);
			}
		}
	};

	entityClass *camera;
	entityClass *cameraSkybox;
	entityClass *objectSkybox;
	holder<cameraControllerClass> manualCamera;
	autoCameraClass autoCamera;
	eventListener<void(uint32, uint32, modifiersFlags)> keyPressListener;

	void keyPress(uint32 a, uint32 b, modifiersFlags)
	{
		if (a == 32)
		{
			if (autoCamera.cam)
			{
				// switch to manual
				autoCamera.cam = nullptr;
				manualCamera->setEntity(camera);
				autoCamera.shipName = 0;
			}
			else
			{
				// switch to auto
				autoCamera.cam = camera;
				manualCamera->setEntity(nullptr);
			}
		}
	}

	void engineInitialize()
	{
		camera = entities()->createUnique();
		{
			ENGINE_GET_COMPONENT(transform, t, camera);
			t.position = vec3(0, 0, 200);
			ENGINE_GET_COMPONENT(camera, c, camera);
			c.cameraOrder = 2;
			c.renderMask = 1;
			c.near = 1;
			c.far = 500;
			c.ambientLight = vec3(0.8);
			c.clear = (cameraClearFlags)0;
			c.effects = cameraEffectsFlags::CombinedPass & ~cameraEffectsFlags::AmbientOcclusion;
			ENGINE_GET_COMPONENT(listener, l, camera);
		}
		cameraSkybox = entities()->createUnique();
		{
			ENGINE_GET_COMPONENT(transform, t, cameraSkybox);
			ENGINE_GET_COMPONENT(camera, c, cameraSkybox);
			c.cameraOrder = 1;
			c.renderMask = 2;
			c.near = 0.1;
			c.far = 100;
			c.ambientLight = vec3(1);
		}
		objectSkybox = entities()->createUnique();
		{
			ENGINE_GET_COMPONENT(transform, t, objectSkybox);
			ENGINE_GET_COMPONENT(render, r, objectSkybox);
			r.object = hashString("ants/skybox/skybox.object");
			r.renderMask = 2;
		}
		manualCamera = newCameraController(camera);
		manualCamera->freeMove = true;
		manualCamera->mouseButton = mouseButtonsFlags::Left;
		manualCamera->movementSpeed = 3;
		keyPressListener.attach(window()->events.keyPress);
		keyPressListener.bind<&keyPress>();
	}

	void engineUpdate()
	{
		ENGINE_GET_COMPONENT(transform, tc, camera);
		ENGINE_GET_COMPONENT(transform, ts, cameraSkybox);
		ts.orientation = tc.orientation;
		autoCamera.update();
	}

	class callbacksInitClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
	public:
		callbacksInitClass()
		{
			engineInitListener.attach(controlThread().initialize, -200);
			engineInitListener.bind<&engineInitialize>();
			engineUpdateListener.attach(controlThread().update, 200);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
