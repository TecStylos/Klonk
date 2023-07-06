#include "AppClock.h"

#include <cmath>
#include <ctime>
#include <chrono>

#include "uibackend/GenTextImage.h"

float DegToRad(float deg)
{
	return deg * (3.1415926535f / 180.0f);
}

HrMinSec getCurrHrMinSec()
{
	time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm loc_tm = *localtime(&tt);

	HrMinSec time;

	time.hours = loc_tm.tm_hour % 12;
	time.minutes = loc_tm.tm_min;
	time.seconds = loc_tm.tm_sec;

	return time;
}

AppClock::AppClock(Framebuffer& fb)
	: Application(fb)
{
	int radius = 100;

	for (int hr = 1; hr < 13; ++hr)
	{
		auto hrImg = genTextImage(std::to_string(hr), 24);

		float rad = DegToRad(hr / 12.0f * 360.0f);

		int x = (m_uiRoot.width() / 2) + (radius * std::sin(rad));
		int y = (m_uiRoot.height() / 2) - (radius * std::cos(rad));

		auto uiHour = m_uiRoot.addElement<UIImage>(x - hrImg.width() / 2, y - hrImg.height() / 2, hrImg);
	}

	auto uiHourHand = m_uiRoot.addElement<UIElement>(m_uiRoot.width() / 2, m_uiRoot.height() / 2, radius / 2, radius / 2);
	uiHourHand->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, const void* pData)
		{
			auto& time = ((AppClock*)pData)->m_time;
			float rad = DegToRad((time.hours + (time.minutes / 60.0f)) / 12.0f * 360.0f);
			fb.drawLine(
				pElem->posX(), pElem->posY(),
				pElem->posX() + (pElem->width() * std::sin(rad)),
				pElem->posY() - (pElem->height() * std::cos(rad)),
				{ 1.0f }
			);
		}
	);

	auto uiMinuteHand = m_uiRoot.addElement<UIElement>(m_uiRoot.width() / 2, m_uiRoot.height() / 2, radius, radius);
	uiMinuteHand->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, const void* pData)
		{
			auto& time = ((AppClock*)pData)->m_time;
			float rad = DegToRad((time.minutes + time.seconds / 60.0f) / 60.0f * 360.0f);
			fb.drawLine(
				pElem->posX(), pElem->posY(),
				pElem->posX() + (pElem->width() * std::sin(rad)),
				pElem->posY() - (pElem->height() * std::cos(rad)),
				{ 1.0f }
			);
		}
	);

	auto uiSecondHand = m_uiRoot.addElement<UIElement>(m_uiRoot.width() / 2, m_uiRoot.height() / 2, radius * 2 / 3, radius * 2 / 3);
	uiSecondHand->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, const void* pData)
		{
			float rad = DegToRad(((AppClock*)pData)->m_time.seconds / 60.0f * 360.0f);
			fb.drawLine(
				pElem->posX(), pElem->posY(),
				pElem->posX() + (pElem->width() * std::sin(rad)),
				pElem->posY() - (pElem->height() * std::cos(rad)),
				{ 0.7f }
			);
		}
	);
}

const char* AppClock::onUpdate()
{
	m_time = getCurrHrMinSec();

	return Application::onUpdate();
}
