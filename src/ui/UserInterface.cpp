#include "UserInterface.h"

UIElement::UIElement(int x, int y, int w, int h)
	: m_x(x), m_y(y), m_w(w), m_h(h)
{}

bool UIElement::isHit(int x, int y) const
{
	return
		m_x <= x && x < m_x + m_w &&
		m_y <= y && y < m_y + m_h;
}

bool UIElement::isHidden() const
{
	return m_isHidden;
}

bool UIElement::isVisible() const
{
	return !m_isHidden;
}

void UIElement::hide()
{
	m_isHidden = true;
}

void UIElement::show()
{
	m_isHidden = false;
}