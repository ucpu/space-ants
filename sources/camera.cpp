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
				entity *ship = entities()->get(shipName);
				CAGE_COMPONENT_ENGINE(transform, t, ship);
				ANTS_COMPONENT(physics, p, ship);
				a.add(t.position);
				if (squaredLength(p.velocity) > 1e-7)
					b.add(t.position - normalize(p.velocity) * 5);
			}
			else
			{
				uint32 cnt = shipComponent::component->group()->count();
				uint32 i = randomRange(0u, cnt);
				shipName = shipComponent::component->group()->array()[i]->name();
			}
		}

	public:
		entity *cam;
		uint32 shipName;

		autoCameraClass() : cam(0), shipName(0)
		{}

		void update()
		{
			updatePositions();
			if (cam)
			{
				CAGE_COMPONENT_ENGINE(transform, t, cam);
				transform t2 = t;
				t.position = b.smooth();
				t.orientation = quat(a.smooth() - t.position, t.orientation * vec3(0, 1, 0));
				t = interpolate(t2, t, 0.1);
			}
		}
	};

	entity *camera;
	entity *cameraSkybox;
	entity *objectSkybox;
	holder<cameraController> manualCamera;
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
			CAGE_COMPONENT_ENGINE(transform, t, camera);
			t.position = vec3(0, 0, 200);
			CAGE_COMPONENT_ENGINE(camera, c, camera);
			c.cameraOrder = 2;
			c.renderMask = 1;
			c.near = 1;
			c.far = 500;
			c.ambientLight = vec3(0.1);
			c.clear = (cameraClearFlags)0;
			c.effects = cameraEffectsFlags::CombinedPass & ~cameraEffectsFlags::AmbientOcclusion;
			CAGE_COMPONENT_ENGINE(listener, ls, camera);
			CAGE_COMPONENT_ENGINE(light, l, camera);
			l.lightType = lightTypeEnum::Directional;
			l.color = vec3(1);
		}
		cameraSkybox = entities()->createUnique();
		{
			CAGE_COMPONENT_ENGINE(transform, t, cameraSkybox);
			CAGE_COMPONENT_ENGINE(camera, c, cameraSkybox);
			c.cameraOrder = 1;
			c.renderMask = 2;
			c.near = 0.1;
			c.far = 100;
			c.ambientLight = vec3(1);
		}
		objectSkybox = entities()->createUnique();
		{
			CAGE_COMPONENT_ENGINE(transform, t, objectSkybox);
			CAGE_COMPONENT_ENGINE(render, r, objectSkybox);
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
		OPTICK_EVENT("camera");
		CAGE_COMPONENT_ENGINE(transform, tc, camera);
		CAGE_COMPONENT_ENGINE(transform, ts, cameraSkybox);
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
