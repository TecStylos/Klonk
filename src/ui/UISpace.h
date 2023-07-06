#pragma once

#include "UserInterface.h"

class UISpace : public UIElement
{
public:
	UISpace(int x, int y, int w, int h);
public:
	template <class ElemType, typename... Args>
	ElemType* addElement(int x, int y, Args... args);
	void remElement(UIElement* pElem);
private:
	std::set<UIElement*> m_elements;
};

template <class ElemType, typename... Args>
ElemType* UISpace::addElement(int x, int y, Args... args)
{
	static_assert(std::is_base_of<UIElement, ElemType>::value, "ElemType not derived from BaseClass");
	ElemType* pElem = new ElemType(m_x + x, m_y + y, args...);
	m_elements.insert(pElem);
	return pElem;
}