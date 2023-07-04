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

UISpace::UISpace(int x, int y, int w, int h)
	: UIElement(x, y, w, h)
{
	setCbOnDown(
		[](UIElement* pThis, int x, int y, void* pData) {
			for (auto& elem : ((UISpace*)pThis)->m_elements)
				if (elem->onDown(x, y, pData))
					return true;
			return false;
		}
	);

	setCbOnUp(
		[](UIElement* pThis, int x, int y, void* pData) {
			for (auto& elem : ((UISpace*)pThis)->m_elements)
				if (elem->onUp(x, y, pData))
					return true;
			return false;
		}
	);

	setCbOnMove(
		[](UIElement* pThis, int xOld, int yOld, int xNew, int yNew, void* pData) {
			for (auto& elem : ((UISpace*)pThis)->m_elements)
				if (elem->onMove(xOld, yOld, xNew, yNew, pData))
					return true;
			return false;
		}
	);

	setCbOnUpdate(
		[](UIElement* pThis, void* pData) {
			for (auto& elem : ((UISpace*)pThis)->m_elements)
				elem->onUpdate(pData);
		}
	);

	setCbOnRender(
		[](const UIElement* pThis, Framebuffer& fb, const void* pData) {
			for (auto& elem : ((const UISpace*)pThis)->m_elements)
				elem->onRender(fb, pData);
		}
	);
}

void UISpace::remElement(UIElement* pElem)
{
	m_elements.erase(pElem);
}

UIImage::UIImage(int x, int y, int w, int h)
	: UIElement(x, y, w, h), m_img(w, h)
{
	initCallbacks();
}

UIImage::UIImage(int x, int y, Image& img)
	: UIElement(x, y, img.width(), img.height()), m_img(img)
{
	initCallbacks();
}

void UIImage::initCallbacks()
{
	setCbOnRender(
		[](const UIElement* pThis, Framebuffer& fb, const void* pData) {
			fb.drawImage(((const UIImage*)pThis)->m_x, ((const UIImage*)pThis)->m_y, ((const UIImage*)pThis)->m_img);
		}
	);
}

Image& UIImage::getImage()
{
	return m_img;
}
