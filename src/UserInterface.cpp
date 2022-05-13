#include "UserInterface.h"

UIElement::UIElement(int x, int y, int w, int h)
	: m_x(x), m_y(y), m_w(w), m_h(h)
{}

bool UIElement::isHit(int x, int y) const
{
	return
		0 <= x && x < m_w &&
		0 <= y && y < m_h;
}

UISpace::UISpace(int x, int y, int w, int h)
	: UIElement(x, y, w, h)
{}

bool UISpace::onDown(int x, int y)
{
	for (auto& elem : m_elements)
		if (elem->onDown(x - m_x, y - m_y))
			return true;
	return false;
}

bool UISpace::onUp(int x, int y)
{
	for (auto& elem : m_elements)
		if (elem->onUp(x - m_x, y - m_y))
			return true;
	return false;
}

bool UISpace::onMove(int xOld, int yOld, int xNew, int yNew)
{
	for (auto& elem : m_elements)
		if (elem->onMove(xOld - m_x, yOld - m_y, xNew - m_x, yNew - m_y))
			return true;
	return false;
}

void UISpace::onUpdate()
{
	for (auto& elem : m_elements)
		elem->onUpdate();
}

void UISpace::onRender(Framebuffer& fb) const
{
	for (auto& elem : m_elements)
		elem->onRender(fb);
}

void UISpace::remElement(UIElement* pElem)
{
	m_elements.erase(pElem);
}

UIImage::UIImage(int x, int y, int w, int h)
	: UIElement(x, y, w, h), m_img(w, h)
{}

UIImage::UIImage(int x, int y, Image& img)
	: UIElement(x, y, img.width(), img.height()), m_img(img)
{}

void UIImage::onRender(Framebuffer& fb) const
{
	fb.drawImage(m_x, m_y, m_img);
}

Image& UIImage::getImage()
{
	return m_img;
}
