#include "AppSettings.h"

#include "uibackend/GenTextImage.h"

AppSettings::AppSettings(Framebuffer& fb)
    : Application(fb)
{
    m_uiRoot.addElement<UIImage>(10, 100, genTextImage("IP: ", 24));
}

const char* AppSettings::onUpdate()
{
    return Application::onUpdate();
}