#pragma once

#include "UserInterface.h"

class UIImage : public UIElement
{
public:
	UIImage(int x, int y, int w, int h);
	UIImage(int x, int y, Image& img);
private:
	void initCallbacks();
public:
	Image& getImage();
protected:
	Image m_img;
};
