#include "AppSpotify.h"

#include <fstream>
#include <sstream>
#include <thread>
#include <filesystem>
#include <chrono>

#include "KlonkError.h"
#include "uibackend/GenTextImage.h"
#include "ui/MakeHomeButton.h"
#include "util/TimeInMs.h"

#define SEEK_PREV_DELAY 3000

#define DECL_APP_SPOTIFY(ptr) \
	auto& app = *(AppSpotify*)(ptr); \
	auto& m_mtx = app.m_mtx;

#define DECL_CONST_APP_SPOTIFY(ptr) \
	auto& app = *(const AppSpotify*)(ptr); \
	auto& m_mtx = app.m_mtx;

#define APP_SPOTIFY_LOCKED(code) \
{ \
	std::lock_guard<std::mutex> lock(m_mtx); \
	code; \
}

Pixel getAvgColor(const Image& img)
{
	Pixel c = { 0.0f };
	for (int y = 0; y < img.height(); ++y)
		for (int x = 0; x < img.width(); ++x)
			c += img.getNC(x, y);
	return c / Pixel(float(img.width() * img.height()));
}

std::string msTimeToString(uint64_t ms)
{
	auto sec = ms / 1000;
	auto min = sec / 60;
	sec -= min * 60;
	return std::to_string(min) + ":" + (sec < 10 ? "0" : "") + std::to_string(sec);
}

AppSpotify::AppSpotify(Framebuffer& fb)
	: Application(fb)
{
	m_thread = std::thread(&AppSpotify::threadFunc, this);

	auto uiTrackName = m_uiRoot.addElement<UIImage>(0, 0, 1, 1);
	uiTrackName->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			DECL_APP_SPOTIFY(pData);
			static std::string oldName, newName;
			static int scrollDirection;

			APP_SPOTIFY_LOCKED(newName = app.m_trackName);
			if (oldName != newName)
			{
				oldName = newName;

				Image img = genTextImage(newName, 18);
				((UIImage*)pElem)->setImage(img);

				scrollDirection = -1;

				pElem->posX() = (pElem->width() > app.m_uiRoot.width()) ? 16 : ((app.m_uiRoot.width() - pElem->width()) / 2);
				pElem->posY() = 22 - pElem->height() / 2;
			}

			if (pElem->width() > app.m_uiRoot.width())
			{
				if (pElem->posX() >= 16)
					scrollDirection = -1;
				else if (pElem->posX() + pElem->width() < app.m_uiRoot.width() - 16)
					scrollDirection = 1;
					
				pElem->posX() += scrollDirection * 3;
			}
		}
	);

	auto uiTrackView = m_uiRoot.addElement<UISpace>(84, 44, 152, 152);
	auto uiTrackViewAccent = uiTrackView->addElement<UIElement>(0, 0, 152, 152);
	uiTrackViewAccent->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, const void* pData)
		{
			DECL_CONST_APP_SPOTIFY(pData);

			static int smoothVolume = 0;

			Pixel color;
			APP_SPOTIFY_LOCKED(color = app.m_accentColor);

			int volDiff = app.m_volume - smoothVolume;
			smoothVolume += (std::abs(volDiff) < 5) ? volDiff : volDiff / 5;

			int hFilled = pElem->height() * smoothVolume / 100;
			int hDarkened = pElem->height() - hFilled;

			fb.drawRect(
				pElem->posX(), pElem->posY(),
				pElem->width(), hDarkened,
				color * Pixel(smoothVolume / 100.0f)
			);
			fb.drawRect(
				pElem->posX(), pElem->posY() + hDarkened,
				pElem->width(), hFilled,
				color
			);
		}
	);
	auto uiTrackViewCover = uiTrackView->addElement<UIImage>(12, 12, 128, 128);
	uiTrackViewCover->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!pElem->isHit(x, y))
				return false;

			app.m_spotify.exec(app.m_isPlaying ? "spotify.pause_playback()" : "spotify.start_playback()");
			app.m_isPlaying = !app.m_isPlaying;

			app.initInstantUpdate();

			return true;
		}
	);
	uiTrackViewCover->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!app.m_loadNewCoverFromDisk)
				return;
			app.m_loadNewCoverFromDisk = false;

			auto& img = ((UIImage*)pElem)->getImage();

			std::string trackID;
			APP_SPOTIFY_LOCKED(trackID = app.m_trackID);

			img.downscaleFrom(Image("./data/spotify/cache/" + trackID + "/cover.jpg"));
			auto color = getAvgColor(img);

			float brightness = float(color);
			if (brightness < 0.7f)
				color *= Color(0.7f / brightness);

			APP_SPOTIFY_LOCKED(app.m_accentColor = color);
		}
	);

	auto uiSeekbar = m_uiRoot.addElement<UIElement>(20, 215, 280, 12);
	uiSeekbar->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, const void* pData)
		{
			DECL_CONST_APP_SPOTIFY(pData);

			int wFilled = pElem->width() * app.m_trackPos / app.m_trackLen;
			int wEmpty = pElem->width() - wFilled;
			fb.drawRect(
				pElem->posX(), pElem->posY(),
				wFilled, pElem->height(),
				{ 0.1f, 0.7f, 0.2f }
			);
			fb.drawRect(
				pElem->posX() + wFilled, pElem->posY(),
				wEmpty, pElem->height(),
				{ 0.2f }
			);

			APP_SPOTIFY_LOCKED(
				for (auto& section : app.m_trackSections)
				{
					int lx = pElem->posX() + pElem->width() * section.first / app.m_trackLen;
					fb.drawLine(
						lx, pElem->posY(),
						lx, pElem->posY() + pElem->height(),
						{ 0.0f }
					);
				}
			);
		}
	);
	uiSeekbar->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!pElem->isHit(x, y))
				return false;

			app.m_trackPos = (x - pElem->posX()) * app.m_trackLen / pElem->width();

			app.m_spotify.exec("spotify.seek_track(" + std::to_string(app.m_trackPos) + ")");

			app.initInstantUpdate();

			return true;
		}
	);
	uiSeekbar->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			static uint64_t lastTime = 0;
			uint64_t currTime = timeInMs();

			if (app.m_isPlaying && lastTime)
			{
				uint64_t diff = currTime - lastTime;
				app.m_trackPos += diff;
			}

			lastTime = currTime;
		}
	);

	auto uiBtnPrevTrack = m_uiRoot.addElement<UIImage>(11, 88, 64, 64);
	uiBtnPrevTrack->getImage().downscaleFrom(Image("resource/btn-prev-track.jpg"));
	uiBtnPrevTrack->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!pElem->isHit(x, y))
				return false;

			app.m_spotify.exec((app.m_trackPos > SEEK_PREV_DELAY) ? "spotify.seek_track(0)" : "spotify.previous_track()");

			app.initInstantUpdate();

			return true;
		}
	);

	auto uiBtnNextTrack = m_uiRoot.addElement<UIImage>(245, 88, 64, 64);
	uiBtnNextTrack->getImage().downscaleFrom(Image("resource/btn-next-track.jpg"));
	uiBtnNextTrack->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!pElem->isHit(x, y))
				return false;

			app.m_spotify.exec("spotify.next_track()");

			app.initInstantUpdate();

			return true;
		}
	);

	auto uiTrackTimeProg = m_uiRoot.addElement<UIImage>(0, 0, 1, 1);
	uiTrackTimeProg->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!pElem->isHit(x, y))
				return false;

			std::map<int, int>::iterator it;
			bool gotoPrevTrack = false;

			APP_SPOTIFY_LOCKED(
				it = app.m_trackSections.upper_bound(app.m_trackPos);
				--it;

				if (app.m_trackPos - it->first < SEEK_PREV_DELAY)
				{
					if (it == app.m_trackSections.begin())
						gotoPrevTrack = true;
					else
						--it;
				}
			);

			app.m_spotify.exec(gotoPrevTrack ? "spotify.previous_track()" : "spotify.seek_track(" + std::to_string(it->first) + ")");

			app.initInstantUpdate();

			return true;
		}
	);
	uiTrackTimeProg->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!app.m_isPlaying)
				return;

			auto& img = ((UIImage*)pElem)->getImage();
			std::string timeStr = msTimeToString(app.m_trackPos);
			if (timeStr.size() > 9)
				return;

			img = genTextImage(timeStr, 14);

			pElem->posX() = 42 - img.width() / 2;
			pElem->posY() = 196 - img.height() / 2;
			pElem->width() = img.width();
			pElem->height() = img.height();
		}
	);

	auto uiTrackTimeLeft = m_uiRoot.addElement<UIImage>(0, 0, 1, 1);
	uiTrackTimeLeft->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!pElem->isHit(x, y))
				return false;

			std::map<int, int>::iterator it;
			bool gotoNextTrack = false;
			APP_SPOTIFY_LOCKED(
				it = app.m_trackSections.upper_bound(app.m_trackPos);
				gotoNextTrack = (it == app.m_trackSections.end());
			);

			app.m_spotify.exec(gotoNextTrack ? "spotify.next_track()" : "spotify.seek_track(" + std::to_string(it->first) + ")");

			app.initInstantUpdate();

			return true;
		}
	);

	uiTrackTimeLeft->setCbOnUpdate(
		[](UIElement* pElem, void* pData)
		{
			DECL_APP_SPOTIFY(pData);

			if (!app.m_isPlaying)
				return;

			auto& img = ((UIImage*)pElem)->getImage();
			std::string timeStr = "-" + msTimeToString(app.m_trackLen - app.m_trackPos);
			if (timeStr.size() > 10)
				return;

			img = genTextImage(timeStr, 14);

			pElem->posX() = 278 - img.width() / 2;
			pElem->posY() = 196 - img.height() / 2;
			pElem->width() = img.width();
			pElem->height() = img.height();
		}
	);
}

void AppSpotify::threadFunc()
{
	std::string spotifyDir = "./data/spotify/";
	std::string cacheDir = spotifyDir + "cache/";

	if (!std::filesystem::is_directory(spotifyDir))
		std::filesystem::create_directory(spotifyDir);

	if (!std::filesystem::is_directory(cacheDir))
		std::filesystem::create_directory(cacheDir);

	while (true)
	{
		Response response = m_spotify.exec("spotify.current_playback()");

		if (
			response.has("item.id") &&
			response.has("progress_ms") &&
			response.has("item.duration_ms") &&
			response.has("item.name") &&
			response.has("is_playing") &&
			response.has("device.volume_percent"))
		{
			auto& newTrackID = response["item.id"].getString();
			bool trackIDChanged;
			APP_SPOTIFY_LOCKED(trackIDChanged = (m_trackID != newTrackID));
			if (trackIDChanged)
			{
				APP_SPOTIFY_LOCKED(m_trackID = newTrackID);

				std::string trackDir = cacheDir + newTrackID + "/";
				std::string imgPath = trackDir + "cover.jpg";
				std::string secPath = trackDir + "sections.txt";

				if (!std::filesystem::is_directory(trackDir))
					std::filesystem::create_directory(trackDir);

				if (std::filesystem::is_regular_file(imgPath))
				{
					m_loadNewCoverFromDisk = true;
				}
				else if (response.has("item.album.images.0.url"))
				{
					auto& imgURL = response["item.album.images.0.url"].getString();
					m_loadNewCoverFromDisk = m_spotify.exec("urllib.request.urlretrieve('" + imgURL + "', '" + imgPath + "')").toString().find(imgPath) != std::string::npos;
				}

				Response sections;
				if (std::filesystem::is_regular_file(secPath))
				{
					std::ifstream file(secPath);
					std::stringstream buff;
					buff << file.rdbuf();
					sections = Response(buff.str());
				}
				else if ((sections = m_spotify.exec("spotify.audio_analysis('" + newTrackID + "')")).has("sections"))
				{
					sections = sections["sections"];
					std::ofstream file (secPath);
					auto secStr = sections.toString();
					file.write(secStr.c_str(), secStr.size());
					file.close();
				}

				if (sections.isList())
				{
					APP_SPOTIFY_LOCKED(
						m_trackSections.clear();
						for (auto& section : sections.getList())
							m_trackSections.insert({ section["start"].getFloat() * 1000.0f, sections["duration"].getFloat() * 1000.0f });
					);
				}
			}

			m_trackPos = response["progress_ms"].getInteger();
			m_trackLen = response["item.duration_ms"].getInteger();
			APP_SPOTIFY_LOCKED(m_trackName = response["item.name"].getString());
			m_isPlaying = response["is_playing"].getBoolean();
			m_volume = response["device.volume_percent"].getInteger();
		}

		{
			std::unique_lock lock(m_mtx);
			m_condVar.wait_for(
				lock,
				std::chrono::seconds(2),
				[&]()
				{
					if (!m_updateNow)
						return false;
					m_updateNow = false;
					return true;
				}
			);
		}
	}
}

void AppSpotify::initInstantUpdate()
{
	m_updateNow = true;
	m_condVar.notify_one();
}
