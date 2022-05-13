#include <iostream>

#include "framebuffer.h"
#include "touch.h"
#include "spotify.h"

#include "Image.h"

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

std::string largestImageURL(const Response& response, const std::string& imagesPath)
{
	if (!response.has(imagesPath))
		return "";
	auto& imgs = response[imagesPath];

	const Response* largest = nullptr;

	for (int i = 0; i < imgs.size(); ++i)
	{
		auto& img = imgs[i];

		if (!largest)
			largest = &img;

		if (!img["width"].isInteger())
			continue;

		if (!(*largest)["width"].isInteger() || img["width"].getInteger() > (*largest)["width"].getInteger())
			largest = &img;
	}

	if (!largest)
		return "";

	return (*largest)["url"].getString();
}

int modePlayback(int argc, char** argv)
{
	Framebuffer<WIDTH, HEIGHT> fb;
	fb.flush();

	Spotify spotify;

	Response response;
	int trackPos = 0;
	int trackLen = 1;
	std::string imgURL = "";

	while (true)
	{
		response = spotify.exec("spotify.currently_playing()");

		auto newImgURL = largestImageURL(response, "item.album.images");
		if (!newImgURL.empty() && imgURL != newImgURL)
		{
			imgURL = newImgURL;
			std::cout << "NEW COVER! - " << spotify.exec("urllib.request.urlretrieve('" + imgURL + "', 'data/cover.jpg')").toString() << std::endl;
		}

		if (response.has("progress_ms") && response.has("item.duration_ms"))
		{
			trackPos = response["progress_ms"].getInteger();
			trackLen = response["item.duration_ms"].getInteger();

			fb.clear({ 0.0f, 0.0f, 0.0f });
			fb.drawRect(0, 0, WIDTH * trackPos / trackLen, HEIGHT, { 0.7f, 0.7f, 0.7f });
		}
		else
		{
			fb.clear({ 1.0f, 0.0f, 0.0f });
		}

		fb.flush();

		sleep(2);
	}

	return 0;
}

int modeImage(int argc, char** argv)
{
	if (argc < 3)
	{
		std::cout << "Usage (image): [imgPath] [x] [y]" << std::endl;
		return 1;
	}

	std::string path = argv[0];
	int x = std::stoi(argv[1]);
	int y = std::stoi(argv[2]);

	Framebuffer<WIDTH, HEIGHT> fb;
	fb.flush();

	Image img(path);

	fb.drawImage(x, y, img);
	fb.flush();

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

	if (mode == "image")
		return modeImage(argc, argv);

	std::cout << "Unknown mode '" << mode << "'!" << std::endl;

	return 1;
}
