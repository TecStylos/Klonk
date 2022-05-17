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

UISpace::UISpace(int x, int y, int w, int h)
	: UIElement(x, y, w, h)
{}

bool UISpace::onDown(int x, int y, void* pData)
{
	for (auto& elem : m_elements)
		if (elem->onDown(x, y, pData))
			return true;
	return UIElement::onDown(x, y, pData);
}

bool UISpace::onUp(int x, int y, void* pData)
{
	for (auto& elem : m_elements)
		if (elem->onUp(x, y, pData))
			return true;
	return UIElement::onUp(x, y, pData);
}

bool UISpace::onMove(int xOld, int yOld, int xNew, int yNew, void* pData)
{
	for (auto& elem : m_elements)
		if (elem->onMove(xOld, yOld, xNew, yNew, pData))
			return true;
	return UIElement::onMove(xOld, yOld, xNew, yNew, pData);
}

void UISpace::onUpdate(void* pData)
{
	for (auto& elem : m_elements)
		elem->onUpdate(pData);
	UIElement::onUpdate(pData);
}

void UISpace::onRender(Framebuffer& fb, const void* pData) const
{
	for (auto& elem : m_elements)
		elem->onRender(fb, pData);
	UIElement::onRender(fb, pData);
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

void UIImage::onRender(Framebuffer& fb, const void* pData) const
{
	fb.drawImage(m_x, m_y, m_img);
	UIElement::onRender(fb, pData);
}

Image& UIImage::getImage()
{
	return m_img;
}
