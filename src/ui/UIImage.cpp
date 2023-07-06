#include "UIImage.h"

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

void UIImage::setImage(Image& img)
{
	m_img = img;
	width() = img.width();
	height() = img.height();
}
