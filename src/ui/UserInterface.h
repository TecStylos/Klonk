#pragma once

#include <set>

#include "uibackend/framebuffer.h"
#include "uibackend/touch.h"

class UIElement
{
public:
	UIElement() = delete;
	UIElement(int x, int y, int w, int h);
	virtual ~UIElement() = default;
public:
	int& posX() { return m_x; }
	int posX() const { return m_x; }
	int& posY() { return m_y; }
	int posY() const { return m_y; }
	int& width() { return m_w; }
	int width() const { return m_w; }
	int& height() { return m_h; }
	int height() const { return m_h; }
public:
	typedef bool (*OnDownCb)(UIElement*, int, int, void*);
	typedef bool (*OnUpCb)(UIElement*, int, int, void*);
	typedef bool (*OnMoveCb)(UIElement*, int, int, int, int, void*);
	typedef void (*OnUpdateCb)(UIElement*, void*);
	typedef void (*OnRenderCb)(const UIElement*, Framebuffer&, const void*);
	void setCbOnDown(OnDownCb cb) { m_cbOnDown = cb; }
	void setCbOnUp(OnUpCb cb) { m_cbOnUp = cb; }
	void setCbOnMove(OnMoveCb cb) { m_cbOnMove = cb; }
	void setCbOnUpdate(OnUpdateCb cb) { m_cbOnUpdate = cb; }
	void setCbOnRender(OnRenderCb cb) { m_cbOnRender = cb; }
public:
	bool isHit(int x, int y) const;
	bool isHidden() const;
	bool isVisible() const;
	void hide();
	void show();
public:
	bool onDown(int x, int y, void* pData) { return (m_cbOnDown && isVisible()) ? m_cbOnDown(this, x, y, pData) : false; }
	bool onUp(int x, int y, void* pData) { return (m_cbOnUp && isVisible()) ? m_cbOnUp(this, x, y, pData) : false; }
	bool onMove(int xOld, int yOld, int xNew, int yNew, void* pData) { return (m_cbOnMove) ? m_cbOnMove(this, xOld, yOld, xNew, yNew, pData) : false; }
	void onUpdate(void* pData) { if (m_cbOnUpdate) m_cbOnUpdate(this, pData); }
	void onRender(Framebuffer& fb, const void* pData) const { if (m_cbOnRender && isVisible()) m_cbOnRender(this, fb, pData); }
protected:
	int m_x;
	int m_y;
	int m_w;
	int m_h;
	OnDownCb m_cbOnDown = nullptr;
	OnUpCb m_cbOnUp = nullptr;
	OnMoveCb m_cbOnMove = nullptr;
	OnUpdateCb m_cbOnUpdate = nullptr;
	OnRenderCb m_cbOnRender = nullptr;
private:
	bool m_isHidden = false;
};
