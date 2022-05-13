#include <iostream>

#include "spotify.h"

#include "UserInterface.h"

#define WIDTH 320
#define HEIGHT 240

int modeQuery(int argc, char** argv)
{
	Spotify spotify;
        while (true)
        {
                std::string command;
                std::cout << " >> ";
                std::getline(std::cin, command);
		if (command == "q")
			break;

                auto response = spotify.exec(command);
                std::cout << "RESPONSE: " << response.toString() << std::endl;

                std::string query = "";
                while (true)
                {
                        std::cout << " QUERY > ";
                        std::getline(std::cin, query);
                        if (query == "q")
                                break;
                        bool exists = response.has(query);
                        std::cout << (exists ? response[query].toString() : " QUERY DOES NOT EXIST!") << std::endl;
                }
        }

	return 0;
}

std::string getImageURL(const Response& response, const std::string& imagesPath)
{
	if (!response.has(imagesPath + ".0.url"))
		return "";

	return response[imagesPath + ".0.url"].getString();
}

Pixel getAvgColor(const Image& img)
{
	Pixel avgColor = { 0.0f, 0.0f, 0.0f };
	for (int y = 0; y < img.height(); ++y)
		for (int x = 0; x < img.width(); ++x)
			avgColor += img.getNC(x, y);
	return avgColor / Pixel(float(img.width() * img.height()));
}

struct UIInfo
{
	std::string coverURL = "";
	int trackPos = 0;
	int trackLen = 1;
	Pixel accentColor = { 0.5f };
	Spotify spotify;
};

#define makeUIInfo() UIInfo& uiInfo = *(UIInfo*)pData

int modePlayback(int argc, char** argv)
{
	Framebuffer fb(WIDTH, HEIGHT);
	fb.flush();

	Response response;

	UIInfo uiInfo;

	UISpace uiRoot(0, 0, WIDTH, HEIGHT);

	auto uiTrackView = uiRoot.addElement<UISpace>(85, 45, 150, 150);
	auto uiTrackViewAccent = uiTrackView->addElement<UIElement>(0, 0, 150, 150);
	uiTrackViewAccent->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, void* pData)
		{
			makeUIInfo();
			fb.drawRect(
				pElem->posX(),
				pElem->posY(),
				pElem->width(),
				pElem->height(),
				uiInfo.accentColor
			);
		}
	);
	auto uiCover = uiTrackView->addElement<UIImage>(11, 11, 128, 128);
	uiCover->setCbOnDown(
		[](UIElement* pElem, int x, int y, void* pData)
		{
			makeUIInfo();
			if (!pElem->isHit(x, y))
				return false;

			uiInfo.spotify.exec("spotify.pause_playback()");

			return true;
		}
	);

	auto uiSeekbar = uiRoot.addElement<UIElement>(20, 218, 280, 7);
	uiSeekbar->setCbOnRender(
		[](const UIElement* pElem, Framebuffer& fb, void* pData)
		{
			makeUIInfo();
			int x = pElem->posX();
			int y = pElem->posY();
			int wFilled = pElem->width() * uiInfo.trackPos / uiInfo.trackLen;
			int wEmpty = pElem->width() - wFilled;
			fb.drawRect(x, y, wFilled, pElem->height(), { 0.1f, 0.7f, 0.2f });
			fb.drawRect(x + wFilled, y, wEmpty, pElem->height(), { 0.2f });
		}
	);

	while (true)
	{
		response = uiInfo.spotify.exec("spotify.currently_playing()");

		auto newImgURL = getImageURL(response, "item.album.images");
		if (!newImgURL.empty() && uiInfo.coverURL != newImgURL)
		{
			uiInfo.coverURL = newImgURL;
			if (uiInfo.spotify.exec("urllib.request.urlretrieve('" + uiInfo.coverURL + "', 'data/cover.jpg')").toString().find("data/cover.jpg") != std::string::npos)
			{
				uiCover->getImage().downscaleFrom(Image("data/cover.jpg"));
				uiInfo.accentColor = getAvgColor(uiCover->getImage());
			}
			else
				std::cout << "[ ERR ]: Could not download " << uiInfo.coverURL << std::endl;
		}

		if (response.has("progress_ms") && response.has("item.duration_ms"))
		{
			uiInfo.trackPos = response["progress_ms"].getInteger();
			uiInfo.trackLen = response["item.duration_ms"].getInteger();
		}
		else
		{
			std::cout << "[ ERR ]: Invalid response:\n    " << response.toString() << std::endl;
		}

		uiRoot.onUpdate(&uiInfo);

		fb.clear(uiInfo.accentColor * Pixel(0.2f));
		uiRoot.onRender(fb, &uiInfo);
		fb.flush();

		sleep(2);
	}

	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "Usage: " << argv[0] << " [mode] [args*]" << std::endl;
		return 1;
	}

	std::string mode = argv[1];

	argc -= 2;
	argv += 2;

	if (mode == "query")
		return modeQuery(argc, argv);

	if (mode == "playback")
		return modePlayback(argc, argv);

	std::cout << "Unknown mode '" << mode << "'!" << std::endl;

	return 1;
}
