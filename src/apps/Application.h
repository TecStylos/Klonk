#pragma once

#include <memory>

#include "ui/framebuffer.h"
#include "ui/touch.h"
#include "ui/UserInterface.h"

class Application
{
public:
	Application() = delete;
	Application(Framebuffer& fb)
		: m_fb(fb), m_uiRoot(0, 0, fb.width(), fb.height())
	{}
	virtual ~Application() = default;
public:
	virtual void onUp(TouchPos pos) { m_uiRoot.onUp(pos.x, pos.y, this); }
	virtual void onDown(TouchPos pos) { m_uiRoot.onDown(pos.x, pos.y, this); }
	virtual void onMove(TouchPos posOld, TouchPos posNew) { m_uiRoot.onMove(posOld.x, posOld.y, posNew.x, posNew.y, this); }
	virtual const char* onUpdate() { m_uiRoot.onUpdate(this); return appToSwitchTo; }
	virtual void onRender() const { m_uiRoot.onRender(m_fb, this); }
	virtual const char* getName() const { return "<APPLICATION>"; }
protected:
	Framebuffer& m_fb;
	UISpace m_uiRoot;
public:
	const char* appToSwitchTo = nullptr;
};

typedef std::shared_ptr<Application> ApplicationRef;
