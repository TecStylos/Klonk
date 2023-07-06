#include "UISpace.h"

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