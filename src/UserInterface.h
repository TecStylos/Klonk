#pragma once

#include <set>

#include "framebuffer.h"
#include "touch.h"

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
public:
	virtual bool onDown(int x, int y, void* pData) { return m_cbOnDown ? m_cbOnDown(this, x, y, pData) : false; }
	virtual bool onUp(int x, int y, void* pData) { return m_cbOnUp ? m_cbOnUp(this, x, y, pData) : false; }
	virtual bool onMove(int xOld, int yOld, int xNew, int yNew, void* pData) { return m_cbOnMove ? m_cbOnMove(this, xOld, yOld, xNew, yNew, pData) : false; }
	virtual void onUpdate(void* pData) { if (m_cbOnUpdate) m_cbOnUpdate(this, pData); }
	virtual void onRender(Framebuffer& fb, const void* pData) const { if (m_cbOnRender) m_cbOnRender(this, fb, pData); }
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
};

class UISpace : public UIElement
{
public:
	UISpace(int x, int y, int w, int h);
public:
	virtual bool onDown(int x, int y, void* pData) override;
	virtual bool onUp(int x, int y, void* pData) override;
	virtual bool onMove(int xOld, int yOld, int xNew, int yNew, void* pData) override;
	virtual void onUpdate(void* pData) override;
	virtual void onRender(Framebuffer& fb, const void* pData) const override;
public:
	template <class ElemType, typename... Args>
	ElemType* addElement(int x, int y, Args... args);
	void remElement(UIElement* pElem);
private:
	std::set<UIElement*> m_elements;
};

class UIImage : public UIElement
{
public:
	UIImage(int x, int y, int w, int h);
	UIImage(int x, int y, Image& img);
public:
	virtual void onRender(Framebuffer& fb, const void* pData) const override;
public:
	Image& getImage();
protected:
	Image m_img;
};

template <class ElemType, typename... Args>
ElemType* UISpace::addElement(int x, int y, Args... args)
{
	static_assert(std::is_base_of<UIElement, ElemType>::value, "ElemType not derived from BaseClass");
	ElemType* pElem = new ElemType(m_x + x, m_y + y, args...);
	m_elements.insert(pElem);
	return pElem;
}
