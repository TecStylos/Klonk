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
	virtual bool onDown(int x, int y) { return false; }
	virtual bool onUp(int x, int y) { return false; }
	virtual bool onMove(int xOld, int yOld, int xNew, int yNew) { return false; }
	virtual void onUpdate() {}
	virtual void onRender(Framebuffer& fb) const {}
protected:
	bool isHit(int x, int y) const;
protected:
	int m_x;
	int m_y;
	int m_w;
	int m_h;
};

class UISpace : public UIElement
{
public:
	UISpace(int x, int y, int w, int h);
public:
	virtual bool onDown(int x, int y) override;
	virtual bool onUp(int x, int y) override;
	virtual bool onMove(int xOld, int yOld, int xNew, int yNew) override;
	virtual void onUpdate() override;
	virtual void onRender(Framebuffer& fb) const override;
public:
	template <class ElemType, typename... Args>
	ElemType* addElement(Args... args);
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
	virtual void onRender(Framebuffer& fb) const override;
public:
	Image& getImage();
protected:
	Image m_img;
};

class UIText : public UIElement
{
};


template <class ElemType, typename... Args>
ElemType* UISpace::addElement(Args... args)
{
	static_assert(std::is_base_of<UIElement, ElemType>::value, "ElemType not derived from BaseClass");
	ElemType* pElem = new ElemType(args...);
	m_elements.insert(pElem);
	return pElem;
}
