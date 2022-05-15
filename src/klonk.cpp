#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

#include "spotify.h"
#include "UserInterface.h"
#include "GenTextImage.h"

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

const std::string& getImageURL(const Response& response, const std::string& imagesPath)
{
	static std::string emptyString;
	if (!response.has(imagesPath + ".0.url"))
		return emptyString;

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
	bool coverIsOutdated = false;
	int trackPos = 0;
	int trackLen = 1;
	std::string trackName = "<UNKNOWN>";
	bool isPlaying = false;
	Pixel accentColor = { 0.5f };
	Spotify spotify;
	std::queue<TouchEvent> touchEvents;
	std::mutex mtxInfo;
	std::mutex mtxSpotify;
	bool shouldExit = false;
	bool spotifyShouldUpdateNow;
	std::condition_variable condVarSpotify;
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

void notifySpotifyUpdate(UIInfo& uiInfo)
{
	EXEC_UIINFO_LOCKED(uiInfo.spotifyShouldUpdateNow = true);
	uiInfo.condVarSpotify.notify_one();
}

uint64_t timeInMs()
{
	timeval tv;
	if (gettimeofday(&tv, 0))
		throw std::runtime_error("Unable to gettimeofday");
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void spotifyThreadFunc(UIInfo* pUIInfo)
{
	auto& uiInfo = *pUIInfo;

	std::string trackID = "";

	while (!uiInfo.shouldExit)
	{
		Response response;
		EXEC_SPOTIFY_LOCKED(response = uiInfo.spotify.exec("spotify.currently_playing()"));

		if (response.has("item.id"))
		{
			auto& newTrackID = response["item.id"].getString();
			if (trackID != newTrackID)
			{
				trackID = newTrackID;
				auto& newImgURL = getImageURL(response, "item.album.images");
				if (!newImgURL.empty())
				{
					bool imgDownloadSuccess;
					EXEC_SPOTIFY_LOCKED(imgDownloadSuccess = uiInfo.spotify.exec("urllib.request.urlretrieve('" + newImgURL + "', 'data/cover.jpg')").toString().find("data/cover.jpg") != std::string::npos);
					EXEC_UIINFO_LOCKED(uiInfo.coverIsOutdated = imgDownloadSuccess);
				}
			}
		}

		if (response.has("progress_ms") && response.has("item.duration_ms") && response.has("item.name"))
		{
			EXEC_UIINFO_LOCKED(
				uiInfo.trackPos = response["progress_ms"].getInteger();
				uiInfo.trackLen = response["item.duration_ms"].getInteger();
				uiInfo.trackName = response["item.name"].getString();
			);
		}

		if (response.has("is_playing"))
		{
			EXEC_UIINFO_LOCKED(uiInfo.isPlaying = response["is_playing"].getBoolean());
		}

		{
			std::unique_lock lock(uiInfo.mtxInfo);
			uiInfo.condVarSpotify.wait_for(
				lock,
				std::chrono::seconds(2),
				[&]()
				{
					if (!uiInfo.spotifyShouldUpdateNow)
						return false;
					uiInfo.spotifyShouldUpdateNow = false;
					return true;
				}
			);
		}
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
		if (!touch.fetchToSync())
		{
			usleep(100 * 1000);
			continue;
		}

		TouchEvent event;

		if (oldIsDown)
		{
			if (touch.down())
				event.type = TouchEvent::Type::Move;
			else
				event.type = TouchEvent::Type::Up;
		}
		else
		{
			if (touch.down())
				event.type = TouchEvent::Type::Down;
			else
				event.type = TouchEvent::Type::None;
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

	UIInfo uiInfo;

	UISpace uiRoot(0, 0, WIDTH, HEIGHT);

	auto uiTrackName = uiRoot.addElement<UIImage>(0, 0, 1, 1);
	uiTrackName->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			MAKE_UIINFO();
			static std::string oldName;

			std::string newName;
			EXEC_UIINFO_LOCKED(newName = uiInfo.trackName);
			if (oldName == newName)
				return;

			auto& img = ((UIImage*)pElem)->getImage();

			img = genTextImage(newName, 14);

			pElem->posX() = 160 - img.width() / 2;
			pElem->posY() = 22 - img.height() / 2;
			pElem->width() = img.width();
			pElem->height() = img.height();

			oldName = newName;
		}
	);

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
	auto uiTrackViewCover = uiTrackView->addElement<UIImage>(11, 11, 128, 128);
	uiTrackViewCover->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			MAKE_UIINFO();
			if (!pElem->isHit(x, y))
				return false;

			bool isPlaying;
			EXEC_UIINFO_LOCKED(isPlaying = uiInfo.isPlaying);

			EXEC_SPOTIFY_LOCKED(uiInfo.spotify.exec(isPlaying ? "spotify.pause_playback()" : "spotify.start_playback()"));
			EXEC_UIINFO_LOCKED(uiInfo.isPlaying = !isPlaying);

			notifySpotifyUpdate(uiInfo);

			return true;
		}
	);
	uiTrackViewCover->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			MAKE_UIINFO();

			bool isOutdated;
			EXEC_UIINFO_LOCKED(isOutdated = uiInfo.coverIsOutdated);
			if (!isOutdated)
				return;

			auto& img = ((UIImage*)pElem)->getImage();

			img.downscaleFrom(Image("data/cover.jpg"));
			auto avgColor = getAvgColor(img);

			EXEC_UIINFO_LOCKED(
				uiInfo.accentColor = avgColor;
				uiInfo.coverIsOutdated = false;
			);
		}
	);

	auto uiSeekbar = uiRoot.addElement<UIElement>(20, 215, 280, 12);
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
	uiSeekbar->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			MAKE_UIINFO();
			if (!pElem->isHit(x, y))
				return false;

			EXEC_UIINFO_LOCKED(x = (x - pElem->posX()) * uiInfo.trackLen / pElem->width(););

			EXEC_SPOTIFY_LOCKED(uiInfo.spotify.exec("spotify.seek_track(" + std::to_string(x) + ")"));

			notifySpotifyUpdate(uiInfo);

			return true;
		}
	);
	uiSeekbar->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			MAKE_UIINFO();
			static uint64_t lastTime = 0;
			uint64_t currTime = timeInMs();
			bool isPlaying;
			EXEC_UIINFO_LOCKED(isPlaying = uiInfo.isPlaying);
			if (isPlaying && lastTime)
			{
				uint64_t diff = currTime - lastTime;
				EXEC_UIINFO_LOCKED(uiInfo.trackPos += diff);
			}
			lastTime = currTime;
		}
	);

	auto uiBtnPrevTrack = uiRoot.addElement<UIImage>(11, 88, 64, 64);
	uiBtnPrevTrack->getImage().downscaleFrom(Image("resource/btn-prev-track.jpg"));
	uiBtnPrevTrack->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			MAKE_UIINFO();
			if (!pElem->isHit(x, y))
				return false;

			bool seekToBeginning;
			EXEC_UIINFO_LOCKED(seekToBeginning = uiInfo.trackPos > 5000;);
			EXEC_SPOTIFY_LOCKED(uiInfo.spotify.exec(seekToBeginning ? "spotify.seek_track(0)" : "spotify.previous_track()"));

			notifySpotifyUpdate(uiInfo);

			return true;
		}
	);

	auto uiBtnNextTrack = uiRoot.addElement<UIImage>(245, 88, 64, 64);
	uiBtnNextTrack->getImage().downscaleFrom(Image("resource/btn-next-track.jpg"));
	uiBtnNextTrack->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			MAKE_UIINFO();
			if (!pElem->isHit(x, y))
				return false;

			EXEC_SPOTIFY_LOCKED(uiInfo.spotify.exec("spotify.next_track()"));

			notifySpotifyUpdate(uiInfo);

			return true;
		}
	);

	std::thread spotifyThread(spotifyThreadFunc, &uiInfo);
	std::thread touchThread(touchThreadFunc, &uiInfo);

	while (!uiInfo.shouldExit)
	{
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

		EXEC_UIINFO_LOCKED(fb.clear(uiInfo.accentColor * Pixel(0.0f)));
		uiRoot.onRender(fb, &uiInfo);
		fb.flush();

		usleep(175 * 1000);
	}

	std::cout << "Exiting...\n";

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
