#include "AppHome.h"

#include "GenTextImage.h"

#define APP_HOME_TILE_WIDTH 100
#define APP_HOME_TILE_HEIGHT 100
#define APP_HOME_TILE_PADDING_X 5
#define APP_HOME_TILE_PADDING_Y 5

AppHome::AppHome(Framebuffer& fb, std::map<std::string, ApplicationRef>& apps)
	: Application(fb), m_apps(apps)
{
	auto titleImg = genTextImage("Home", 18);
	auto uiTitle = m_uiRoot.addElement<UIImage>((m_uiRoot.width() - titleImg.width()) / 2, 0, titleImg);
}

const char* AppHome::onUpdate()
{
	if (!m_initialized)
	{
		m_initialized = true;

		int x = 10;
		int y = 20;

		for (auto& app : m_apps)
		{
			if (app.first == "Home")
				continue;

			auto uiTile = m_uiRoot.addElement<UISpace>(x, y, APP_HOME_TILE_WIDTH, APP_HOME_TILE_HEIGHT);
			uiTile->setCbOnDown(
				[](UIElement* pElem, int x, int y, void* pData)
				{
					auto& app = *(AppHome*)pData;

					if (!pElem->isHit(x, y))
						return false;

					app.appToSwitchTo = app.m_tiles.find(pElem)->second;

					return true;
				}
			);

			auto uiBkg = uiTile->addElement<UIElement>(APP_HOME_TILE_PADDING_X, APP_HOME_TILE_PADDING_Y, APP_HOME_TILE_WIDTH - APP_HOME_TILE_PADDING_X * 2, APP_HOME_TILE_HEIGHT - APP_HOME_TILE_PADDING_Y * 2);
			uiBkg->setCbOnRender(
				[](const UIElement* pElem, Framebuffer& fb, const void* pData)
				{
					fb.drawRect(
						pElem->posX(), pElem->posY(),
						pElem->width(), pElem->height(),
						{ 0.3f }
					);
				}
			);

			auto nameImg = genTextImage(app.first, 16);
			auto uiTileName = uiTile->addElement<UIImage>((APP_HOME_TILE_WIDTH - nameImg.width()) / 2, APP_HOME_TILE_HEIGHT - APP_HOME_TILE_PADDING_Y - nameImg.height(), nameImg);

			m_tiles.insert({ uiTile, app.second->getName() });

			x += APP_HOME_TILE_WIDTH;
			if (x > m_uiRoot.width() - APP_HOME_TILE_WIDTH)
			{
				x = 0;
				y += APP_HOME_TILE_WIDTH;
			}
		}
	}

	return Application::onUpdate();
}
