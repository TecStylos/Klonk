#include "MakeHomeButton.h"

#include "apps/Application.h"

UIImage* makeHomeButton(UISpace* root)
{
	auto uiBtnRetToHome = root->addElement<UIImage>(12, 12, 24, 24);
	uiBtnRetToHome->getImage().downscaleFrom(Image("./resource/btn-ret-to-home.jpg"));
	uiBtnRetToHome->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			if (!pElem->isHit(x, y))
				return false;

			((Application*)pData)->appToSwitchTo = "Home";

			return true;
		}
	);

	return uiBtnRetToHome;
}
