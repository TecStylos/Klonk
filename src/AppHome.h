#pragma once

#include <map>

#include "Application.h"

class AppHome : public Application
{
public:
	AppHome() = delete;
	AppHome(Framebuffer& fb, std::map<std::string, ApplicationRef>& apps);
	virtual ~AppHome() = default;
public:
	virtual const char* onUpdate() override;
	virtual const char* getName() const override { return "Home"; }
private:
	bool m_initialized = false;
	std::map<std::string, ApplicationRef>& m_apps;
	std::map<UIElement*, const char*> m_tiles;
};
