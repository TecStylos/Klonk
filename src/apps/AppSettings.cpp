#include "AppSettings.h"

#include <sys/ioctl.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "uibackend/GenTextImage.h"

std::string getIPAddress()
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (sock < 0)
        return "";

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
    static UIImage* ipImage = m_uiRoot.addElement<UIImage>(10, 30, genTextImage("IP: ", 24));
    static std::string oldIP = getIPAddress();

    std::string newIP = getIPAddress();

    if (oldIP != newIP)
    {
        oldIP = newIP;
        auto newImg = genTextImage("IP: " + newIP, 24);
        ipImage->setImage(newImg);
    }

    return Application::onUpdate();
}