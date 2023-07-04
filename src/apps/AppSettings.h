#pragma once

#include "Application.h"

class AppSettings : public Application
{
public:
    AppSettings() = delete;
    AppSettings(Framebuffer& fb);
    virtual ~AppSettings() = default;
public:
    virtual const char* onUpdate() override;
    virtual const char* getName() const override { return "Settings"; }
};