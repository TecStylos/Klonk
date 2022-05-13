#pragma once

#include <memory>

#include "framebuffer.h"
#include "touch.h"

class UIElement
{
public:
	UIElement(int x, int y, int w, int h);
	virtual ~UIElement() = default;
public:
	virtual bool onDown(int x, int y);
	virtual bool onUp(int x, int y);
	virtual void onUpdate();
	virtual void onRender();
protected:
	int m_x;
	int m_y;
	int m_width;
	int m_height;
}
typedef std::shared_ptr<UIElement> UIElementRef;

class UISpace : public UIElement
{
public:
	UserInterface();
public:
	int addElement(UIElementRef elem);
private:
	int m_nextElemID = 1;
	std::map<int, UIElementRef> m_elements;
}

class UIImage : public UIElement
{
}

class UIText : public UIElement
{
}
