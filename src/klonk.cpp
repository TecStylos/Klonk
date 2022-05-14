#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#include "spotify.h"
#include "UserInterface.h"

#define WIDTH 320
#define HEIGHT 240

int modeQuery(int argc, char** argv)
{
	Spotify spotify;
        while (true)
        {
                std::string command;
                std::cout << " >> ";
                std::getline(std::cin, command);
		if (command == "q")
			break;

                auto response = spotify.exec(command);
                std::cout << "RESPONSE: " << response.toString() << std::endl;

                std::string query = "";
                while (true)
                {
                        std::cout << " QUERY > ";
                        std::getline(std::cin, query);
                        if (query == "q")
                                break;
                        bool exists = response.has(query);
                        std::cout << (exists ? response[query].toString() : " QUERY DOES NOT EXIST!") << std::endl;
                }
        }

	return 0;
}

std::string getImageURL(const Response& response, const std::string& imagesPath)
{
	if (!response.has(imagesPath + ".0.url"))
		return "";

	return response[imagesPath + ".0.url"].getString();
}

Pixel getAvgColor(const Image& img)
{
	Pixel avgColor = { 0.0f, 0.0f, 0.0f };
	for (int y = 0; y < img.height(); ++y)
		for (int x = 0; x < img.width(); ++x)
			avgColor += img.getNC(x, y);
	return avgColor / Pixel(float(img.width() * img.height()));
}

struct TouchEvent
{
	enum class Type { None, Up, Down, Move } type = Type::None;
	TouchPos posOld, posNew;
};

struct UIInfo
{
	std::string coverURL = "";
	int trackPos = 0;
	int trackLen = 1;
	Pixel accentColor = { 0.5f };
	Spotify spotify;
	std::queue<TouchEvent> touchEvents;
	std::mutex mtxInfo;
	std::mutex mtxSpotify;
	bool shouldExit = false;
};

#define MAKE_UIINFO() UIInfo& uiInfo = *(UIInfo*)pData
#define EXEC_UIINFO_LOCKED(code) \
{ \
	std::lock_guard lock(uiInfo.mtxInfo); \
	code; \
}
#define EXEC_SPOTIFY_LOCKED(code) \
{ \
	std::lock_guard lock(uiInfo.mtxSpotify); \
	code; \
}

void spotifyThreadFunc(UIInfo* pUIInfo)
{
	auto& uiInfo = *pUIInfo;

	while (!uiInfo.shouldExit)
	{
		sleep(1);
	}
}

void touchThreadFunc(UIInfo* pUIInfo)
{
	auto& uiInfo = *pUIInfo;

	Touch<WIDTH, HEIGHT, 200, 3800> touch;

	TouchPos oldPos;
	bool oldIsDown = false;

	while (!uiInfo.shouldExit)
	{
		touch.fetchToSync();

		TouchEvent event;

		if (oldIsDown)
		{
			if (touch.down())
			{
				event.type = TouchEvent::Type::Move;
			}
			else
			{
				event.type = TouchEvent::Type::Up;
			}
		}
		else
		{
			if (touch.down())
			{
				event.type = TouchEvent::Type::Down;
			}
			else
			{
				event.type = TouchEvent::Type::None;
			}
		}

		if (event.type != TouchEvent::Type::None)
		{
			event.posOld = oldPos;
			event.posNew = touch.pos();

			EXEC_UIINFO_LOCKED(uiInfo.touchEvents.push(event));
		}

		oldPos = touch.pos();
		oldIsDown = touch.down();
	}
}

int modePlayback(int argc, char** argv)
{
	Framebuffer fb(WIDTH, HEIGHT);
	fb.flush();

	Response response;

	UIInfo uiInfo;

	UISpace uiRoot(0, 0, WIDTH, HEIGHT);

	auto uiTrackView = uiRoot.addElement<UISpace>(85, 45, 150, 150);
	auto uiTrackViewAccent = uiTrackView->addElement<UIElement>(0, 0, 150, 150);
	uiTrackViewAccent->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, void* pData)
		{
			MAKE_UIINFO();
			EXEC_UIINFO_LOCKED(
				fb.drawRect(
					pElem->posX(),
					pElem->posY(),
					pElem->width(),
					pElem->height(),
					uiInfo.accentColor
				);
			);
		}
	);
	auto uiCover = uiTrackView->addElement<UIImage>(11, 11, 128, 128);
	uiCover->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			MAKE_UIINFO();
			if (!pElem->isHit(x, y))
				return false;

			EXEC_SPOTIFY_LOCKED(uiInfo.spotify.exec("spotify.pause_playback()"));

			return true;
		}
	);

	auto uiSeekbar = uiRoot.addElement<UIElement>(20, 218, 280, 7);
	uiSeekbar->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, void* pData)
		{
			MAKE_UIINFO();
			int x = pElem->posX();
			int y = pElem->posY();
			int wFilled;
			EXEC_UIINFO_LOCKED(wFilled = pElem->width() * uiInfo.trackPos / uiInfo.trackLen);
			int wEmpty = pElem->width() - wFilled;
			fb.drawRect(x, y, wFilled, pElem->height(), { 0.1f, 0.7f, 0.2f });
			fb.drawRect(x + wFilled, y, wEmpty, pElem->height(), { 0.2f });
		}
	);

	auto uiExitBtn = uiRoot.addElement<UIElement>(0, 0, 10, 10);
	uiExitBtn->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			if (!pElem->isHit(x, y))
				return false;

			MAKE_UIINFO();
			EXEC_UIINFO_LOCKED(uiInfo.shouldExit = true);

			return true;
		}
	);
	uiExitBtn->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, void* pData)
		{
			fb.drawRect(pElem->posX(), pElem->posY(), pElem->width(), pElem->height(), { 1.0f, 0.0f, 0.0f });
		}
	);

	std::thread spotifyThread(spotifyThreadFunc, &uiInfo);
	std::thread touchThread(touchThreadFunc, &uiInfo);

	while (!uiInfo.shouldExit)
	{
		EXEC_SPOTIFY_LOCKED(response = uiInfo.spotify.exec("spotify.currently_playing()"));

		auto newImgURL = getImageURL(response, "item.album.images");

		EXEC_UIINFO_LOCKED(
			if (!newImgURL.empty() && uiInfo.coverURL != newImgURL)
			{
				uiInfo.coverURL = newImgURL;
				if (uiInfo.spotify.exec("urllib.request.urlretrieve('" + uiInfo.coverURL + "', 'data/cover.jpg')").toString().find("data/cover.jpg") != std::string::npos)
				{
					uiCover->getImage().downscaleFrom(Image("data/cover.jpg"));
					uiInfo.accentColor = getAvgColor(uiCover->getImage());
				}
				else
					std::cout << "[ ERR ]: Could not download " << uiInfo.coverURL << std::endl;
			}

			if (response.has("progress_ms") && response.has("item.duration_ms"))
			{
				uiInfo.trackPos = response["progress_ms"].getInteger();
				uiInfo.trackLen = response["item.duration_ms"].getInteger();
			}
			else
			{
				std::cout << "[ ERR ]: Invalid response:\n    " << response.toString() << std::endl;
			}
		);

		bool touchEventAvail;
		EXEC_UIINFO_LOCKED(touchEventAvail = !uiInfo.touchEvents.empty());
		while (touchEventAvail)
		{
			TouchEvent event;
			EXEC_UIINFO_LOCKED(
				event = uiInfo.touchEvents.front();
				uiInfo.touchEvents.pop();
			);

			switch (event.type)
			{
			case TouchEvent::Type::Up:
				uiRoot.onUp(event.posNew.x, event.posNew.y, &uiInfo);
				break;
			case TouchEvent::Type::Down:
				uiRoot.onDown(event.posNew.x, event.posNew.y, &uiInfo);
				break;
			case TouchEvent::Type::Move:
				uiRoot.onMove(event.posOld.x, event.posOld.y, event.posNew.x, event.posNew.y, &uiInfo);
				break;
			}

			EXEC_UIINFO_LOCKED(touchEventAvail = !uiInfo.touchEvents.empty());
		}

		uiRoot.onUpdate(&uiInfo);

		EXEC_UIINFO_LOCKED(fb.clear(uiInfo.accentColor * Pixel(0.2f)));
		uiRoot.onRender(fb, &uiInfo);
		fb.flush();

		sleep(2);
	}

	std::cout << "Exiting...";

	spotifyThread.join();
	touchThread.join();

	fb.clear({});
	fb.flush();

	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "Usage: " << argv[0] << " [mode] [args*]" << std::endl;
		return 1;
	}

	std::string mode = argv[1];

	argc -= 2;
	argv += 2;

	if (mode == "query")
		return modeQuery(argc, argv);

	if (mode == "playback")
		return modePlayback(argc, argv);

	std::cout << "Unknown mode '" << mode << "'!" << std::endl;

	return 1;
}
