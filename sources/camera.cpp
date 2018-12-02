#include "common.h"

#include <cage-core/hashString.h>
#include <cage-client/cameraController.h>

namespace
{
	entityClass *camera;
	entityClass *cameraSkybox;
	entityClass *objectSkybox;
	holder<cameraControllerClass> cameraController;

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
			c.ambientLight = vec3(1, 1, 1) * 0.8;
			c.clear = (cameraClearFlags)0;
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
			c.ambientLight = vec3(1, 1, 1);
		}
		objectSkybox = entities()->createUnique();
		{
			ENGINE_GET_COMPONENT(transform, t, objectSkybox);
			ENGINE_GET_COMPONENT(render, r, objectSkybox);
			r.object = hashString("ants/skybox/skybox.object");
			r.renderMask = 2;
		}
		cameraController = newCameraController(camera);
		//cameraController->freeMove = true;
		cameraController->mouseButton = mouseButtonsFlags::Left;
		cameraController->movementSpeed = 3;
	}

	void engineUpdate()
	{
		ENGINE_GET_COMPONENT(transform, tc, camera);
		ENGINE_GET_COMPONENT(transform, ts, cameraSkybox);
		ts.orientation = tc.orientation;
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
