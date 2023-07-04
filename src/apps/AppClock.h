#pragma once

#include "Application.h"

struct HrMinSec
{
	int hours, minutes, seconds;
};

class AppClock : public Application
{
public:
	AppClock() = delete;
	AppClock(Framebuffer& fb);
	virtual ~AppClock() = default;
public:
	virtual const char* onUpdate() override;
	virtual const char* getName() const override { return "Clock"; }
private:
	HrMinSec m_time;
};
