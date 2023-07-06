#include "AppSettings.h"

#include <sys/ioctl.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "util/TimeInMs.h"
#include "uibackend/GenTextImage.h"

std::string getIPAddress()
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (sock < 0)
        return "No IP found";

    struct ifreq ifr{};
    strcpy(ifr.ifr_name, "wlan0");
    ioctl(sock, SIOCGIFADDR, &ifr);
    close(sock);

    char ip[INET_ADDRSTRLEN];
    strcpy(ip, inet_ntoa(((sockaddr_in*)&ifr.ifr_addr)->sin_addr));

    return ip;
}

AppSettings::AppSettings(Framebuffer& fb)
    : Application(fb)
{
}

const char* AppSettings::onUpdate()
{
    static UIImage* ipImage = m_uiRoot.addElement<UIImage>(10, 48, genTextImage("IP: ", 24));
    static std::string oldIP = "";

    static uint64_t lastUpdate = 0;
    uint64_t now = timeInMs();

    if (now - lastUpdate > 10000)
    {
        lastUpdate = now;

        std::string newIP = getIPAddress();

        if (oldIP != newIP)
        {
            oldIP = newIP;
            auto newImg = genTextImage("IP: " + newIP, 24);
            ipImage->setImage(newImg);
        }
    }

    return Application::onUpdate();
}