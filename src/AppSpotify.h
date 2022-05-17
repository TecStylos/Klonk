#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>

#include "Application.h"
#include "spotify.h"

class AppSpotify : public Application
{
public:
	AppSpotify() = delete;
	AppSpotify(Framebuffer& fb);
	virtual ~AppSpotify() = default;
public:
	virtual const char* getName() const override { return "spotify"; }
private:
	void threadFunc();
	void initInstantUpdate();
private:
	Spotify m_spotify;
	std::thread m_thread;
	std::string m_trackID = std::string();
	std::atomic_bool m_loadNewCoverFromDisk = false;
	std::atomic_int m_trackPos = 0;
	std::atomic_int m_trackLen = 1;
	std::string m_trackName = "<UNKNOWN>";
	std::atomic_bool m_isPlaying = false;
	std::atomic_int m_volume = 100;
	mutable std::mutex m_mtx;
	std::condition_variable m_condVar;
	bool m_updateNow = false;
	Pixel m_accentColor = { 0.3f };
	std::map<int, int> m_trackSections;
};
