#include <thread>
#include <iostream>

#include "KlonkError.h"
#include "SharedQueue.h"
#include "AppSpotify.h"

#define WIDTH 320
#define HEIGHT 240

#define TOUCH_DELAY_MS 100
#define FRAME_DELAY_MS 175

void touchThreadFunc(SharedQueue<TouchEvent>* pTouchEvents)
{
	Touch<WIDTH, HEIGHT, 200, 3800> touch;

	TouchPos oldPos;
	bool oldIsDown = false;

	while (true)
	{
		if (!touch.fetchToSync())
		{
			usleep(TOUCH_DELAY_MS * 1000);
			continue;
		}

		TouchEvent event;
		if (oldIsDown)
			event.type = touch.down() ? TouchEvent::Type::Move : TouchEvent::Type::Up;
		else
			event.type = touch.down() ? TouchEvent::Type::Down : TouchEvent::Type::None;

		if (event.type != TouchEvent::Type::None)
		{
			event.posOld = oldPos;
			event.posNew = touch.pos();

			pTouchEvents->push(event);
		}

		oldPos = touch.pos();
		oldIsDown = touch.down();
	}
}

int main(int argc, const char** argv)
{
	Framebuffer fb(WIDTH, HEIGHT);
	fb.flush();

	SharedQueue<TouchEvent> touchEvents;

	std::thread touchThread(touchThreadFunc, &touchEvents);

	std::map<std::string, ApplicationRef> applications;

	//applications.insert({ "home", std::make_shared<Application>(new AppHome(fb, applications)) });
	applications.insert({ "spotify", ApplicationRef(new AppSpotify(fb)) });

	const char* changeAppName = nullptr;
	auto currApp = applications.find("spotify");

	while (true)
	{
		fb.clear({ });
		currApp->second->onRender();
		fb.flush();

		while (!touchEvents.empty())
		{
			TouchEvent event = touchEvents.pop();

			switch (event.type)
			{
			case TouchEvent::Type::Up: currApp->second->onUp(event.posNew); break;
			case TouchEvent::Type::Down: currApp->second->onDown(event.posNew); break;
			case TouchEvent::Type::Move: currApp->second->onMove(event.posOld, event.posNew); break;
			}
		}

		if ((changeAppName = currApp->second->onUpdate()))
		{
			currApp = applications.find(changeAppName);
			if (currApp == applications.end())
				throw KlonkError("Unknown application '" + std::string(changeAppName) + "'!");
		}

		usleep(FRAME_DELAY_MS * 1000);
	}

	touchThread.join();

	fb.clear({ });
	fb.flush();

	return 0;
}
