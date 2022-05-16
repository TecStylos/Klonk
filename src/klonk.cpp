#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <filesystem>

#include "spotify.h"
#include "UserInterface.h"
#include "GenTextImage.h"

#define WIDTH 320
#define HEIGHT 240

#define SEEK_PREV_DELAY 3000

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
	std::string trackID = "";
	bool isPlaying = false;
	int volume = 100;
	Pixel accentColor = { 0.5f };
	Spotify spotify;
	std::queue<TouchEvent> touchEvents;
	std::mutex mtxInfo;
	std::mutex mtxSpotify;
	bool shouldExit = false;
	bool spotifyShouldUpdateNow;
	std::condition_variable condVarSpotify;
	std::map<int, int> trackSections; // Key: start, Val: duration
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

std::string msTimeToString(uint64_t ms)
{
	auto seconds = ms / 1000;
	auto minutes = seconds / 60;
	seconds -=  minutes * 60;
	return std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
}

void spotifyThreadFunc(UIInfo* pUIInfo)
{
	auto& uiInfo = *pUIInfo;

	std::string cacheDir = "./data/cache/";

	if (!std::filesystem::is_directory(cacheDir))
		std::filesystem::create_directory(cacheDir);

	while (!uiInfo.shouldExit)
	{
		Response response;
		EXEC_SPOTIFY_LOCKED(response = uiInfo.spotify.exec("spotify.current_playback()"));

		if (
			response.has("item.id") &&
			response.has("progress_ms") &&
			response.has("item.duration_ms") &&
			response.has("item.name") &&
			response.has("device.volume_percent") &&
			response.has("is_playing"))
		{
			auto& newTrackID = response["item.id"].getString();
			bool isNewTrackID;
			EXEC_UIINFO_LOCKED(isNewTrackID = (uiInfo.trackID != newTrackID));
			if (isNewTrackID)
			{
				EXEC_UIINFO_LOCKED(uiInfo.trackID = newTrackID);

				std::string trackDir = cacheDir + newTrackID + "/";
				std::string imgPath = trackDir + "cover.jpg";
				std::string secPath = trackDir + "sections.txt";

				if (!std::filesystem::is_directory(trackDir))
					std::filesystem::create_directory(trackDir);

				if (std::filesystem::is_regular_file(imgPath))
				{
					EXEC_UIINFO_LOCKED(uiInfo.coverIsOutdated = true);
				}
				else
				{
					auto& imgURL = getImageURL(response, "item.album.images");
					if (!imgURL.empty())
					{
						bool imgDownloadSuccess;
						EXEC_SPOTIFY_LOCKED(imgDownloadSuccess = uiInfo.spotify.exec("urllib.request.urlretrieve('" + imgURL + "', '" + imgPath + "')").toString().find(imgPath) != std::string::npos);
						EXEC_UIINFO_LOCKED(uiInfo.coverIsOutdated = imgDownloadSuccess);
					}
				}

				Response sections;
				if (std::filesystem::is_regular_file(secPath))
				{
					std::ifstream file(secPath);
					std::stringstream buff;
					buff << file.rdbuf();
					sections = Response(buff.str());
				}
				else
				{
					EXEC_SPOTIFY_LOCKED(sections = uiInfo.spotify.exec("spotify.audio_analysis('" + newTrackID + "')"));
					if (sections.has("sections"))
					{
						sections = sections["sections"];
						std::ofstream file(secPath);
						auto secStr = sections.toString();
						file.write(secStr.c_str(), secStr.size());
						file.close();
					}
				}

				if (sections.isList())
				{
					EXEC_UIINFO_LOCKED(
						uiInfo.trackSections.clear();
						for (auto& section : sections.getList())
							uiInfo.trackSections.insert({ section["start"].getFloat() * 1000.0f, section["duration"].getFloat() * 1000.0f });
					);
				}
			}

			EXEC_UIINFO_LOCKED(
				uiInfo.trackPos = response["progress_ms"].getInteger();
				uiInfo.trackLen = response["item.duration_ms"].getInteger();
				uiInfo.trackName = response["item.name"].getString();
				uiInfo.isPlaying = response["is_playing"].getBoolean();
				uiInfo.volume = response["device.volume_percent"].getInteger();
			);
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
			static int scrollDirection;

			std::string newName;
			EXEC_UIINFO_LOCKED(newName = uiInfo.trackName);
			if (oldName != newName)
			{
				auto& img = ((UIImage*)pElem)->getImage();
				img = genTextImage(newName, 18);
				oldName = newName;

				scrollDirection = -1;

				pElem->posX() = (img.width() > 320) ? 16 : (160 - img.width() / 2);
				pElem->posY() = 22 - img.height() / 2;
				pElem->width() = img.width();
				pElem->height() = img.height();

			}

			if (pElem->width() > 320)
			{
				if (pElem->posX() >= 16)
					scrollDirection = -1;
				else if (pElem->posX() + pElem->width() < 320 - 16)
					scrollDirection = 1;
				pElem->posX() += scrollDirection * 3;
			}
		}
	);

	auto uiTrackView = uiRoot.addElement<UISpace>(84, 44, 152, 152);
	auto uiTrackViewAccent = uiTrackView->addElement<UIElement>(0, 0, 152, 152);
	uiTrackViewAccent->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, void* pData)
		{
			MAKE_UIINFO();

			static int smoothVolume = 0;

			Pixel color;
			int infoVolume;
			EXEC_UIINFO_LOCKED(
				color = uiInfo.accentColor;
				infoVolume = uiInfo.volume;
			);

			int diff = infoVolume - smoothVolume;
			smoothVolume += (std::abs(diff) < 5) ? diff : diff / 5;

			int h = pElem->height();
			int hFilled = h * smoothVolume / 100;
			int hDarkened = h - hFilled;

			fb.drawRect(
				pElem->posX(),
				pElem->posY(),
				pElem->width(),
				hDarkened,
				color * Pixel(smoothVolume / 100.0f)
			);
			fb.drawRect(
				pElem->posX(),
				pElem->posY() + hDarkened,
				pElem->width(),
				hFilled,
				color
			);
		}
	);
	auto uiTrackViewCover = uiTrackView->addElement<UIImage>(12, 12, 128, 128);
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

			std::string trackID;
			EXEC_UIINFO_LOCKED(trackID = uiInfo.trackID);

			img.downscaleFrom(Image("./data/cache/" + trackID + "/cover.jpg"));
			auto avgColor = getAvgColor(img);

			float brightness = float(avgColor);
			if (brightness < 0.7f)
				avgColor = avgColor * Color(0.7f / brightness);

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

			EXEC_UIINFO_LOCKED(
				for (auto& section : uiInfo.trackSections)
				{
					int lx = x + pElem->width() * section.first / uiInfo.trackLen;
					fb.drawLine(lx, y, lx, y + pElem->height(), Color(0.0f));
				}
			);
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
			EXEC_UIINFO_LOCKED(seekToBeginning = uiInfo.trackPos > SEEK_PREV_DELAY;);
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

	auto uiTrackTimeProg = uiRoot.addElement<UIImage>(0, 0, 1, 1);
	uiTrackTimeProg->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			MAKE_UIINFO();
			EXEC_UIINFO_LOCKED(if (!uiInfo.isPlaying) return);

			auto& img = ((UIImage*)pElem)->getImage();
			std::string timeStr;
			EXEC_UIINFO_LOCKED(timeStr = msTimeToString(uiInfo.trackPos));
			if (timeStr.size() > 9)
				return;

			img = genTextImage(timeStr, 14);

			pElem->posX() = 42 - img.width() / 2;
			pElem->posY() = 196 - img.height() / 2;
			pElem->width() = img.width();
			pElem->height() = img.height();
		}
	);
	uiTrackTimeProg->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			MAKE_UIINFO();

			if (!pElem->isHit(x, y))
				return false;

			std::map<int, int>::iterator it;
			bool gotoPrevTrack = false;

			EXEC_UIINFO_LOCKED(
				it = uiInfo.trackSections.upper_bound(uiInfo.trackPos);
				--it;

				if (uiInfo.trackPos - it->first < SEEK_PREV_DELAY)
				{
					if (it == uiInfo.trackSections.begin())
						gotoPrevTrack = true;
					else
						--it;
				}
			);

			EXEC_SPOTIFY_LOCKED(
				uiInfo.spotify.exec(gotoPrevTrack ? "spotify.previous_track()" : "spotify.seek_track(" + std::to_string(it->first) + ")");
			);

			notifySpotifyUpdate(uiInfo);

			return true;
		}
	);

	auto uiTrackTimeLeft = uiRoot.addElement<UIImage>(0, 0, 1, 1);
	uiTrackTimeLeft->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			MAKE_UIINFO();
			EXEC_UIINFO_LOCKED(if (!uiInfo.isPlaying) return);

			auto& img = ((UIImage*)pElem)->getImage();
			std::string timeStr;
			EXEC_UIINFO_LOCKED(timeStr = "-" + msTimeToString(uiInfo.trackLen - uiInfo.trackPos));
			if (timeStr.size() > 10)
				return;

			img = genTextImage(timeStr, 14);

			pElem->posX() = 278 - img.width() / 2;
			pElem->posY() = 196 - img.height() / 2;
			pElem->width() = img.width();
			pElem->height() = img.height();
		}
	);
	uiTrackTimeLeft->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			MAKE_UIINFO();

			if (!pElem->isHit(x, y))
				return false;

			std::map<int, int>::iterator it;
			bool gotoNextTrack = false;

			EXEC_UIINFO_LOCKED(
				it = uiInfo.trackSections.upper_bound(uiInfo.trackPos);
				gotoNextTrack = (it == uiInfo.trackSections.end());
			);

			EXEC_SPOTIFY_LOCKED(
				uiInfo.spotify.exec(gotoNextTrack ? "spotify.next_track()" : "spotify.seek_track(" + std::to_string(it->first) + ")");
			);

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
