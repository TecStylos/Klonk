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

std::string getImageURL(const Response& response, const std::string& imagesPath)
{
	if (!response.has(imagesPath + ".0.url"))
		return "";

	return response[imagesPath + ".0.url"].getString();
}

int modePlayback(int argc, char** argv)
{
	Framebuffer<WIDTH, HEIGHT> fb;
	fb.flush();

	Spotify spotify;

	Response response;

	int trackPos = 0;
	int trackLen = 1;
	std::string coverURL = "";
	Image coverImg(128, 128);
	Pixel accentColor = { 0.5f, 0.5f, 0.5f };
	const Pixel seekbarColorFilled = { 0.1f, 0.7f, 0.2f };
	const Pixel seekbarColorEmpty = { 0.2f, 0.2f, 0.2f };
	const int seekbarHeight = 7;
	const int seekbarWidth = 280;

	while (true)
	{
		fb.clear({ 0.0f, 0.0f, 0.0f });

		response = spotify.exec("spotify.currently_playing()");

		auto newImgURL = getImageURL(response, "item.album.images");
		if (!newImgURL.empty() && coverURL != newImgURL)
		{
			coverURL = newImgURL;
			if (spotify.exec("urllib.request.urlretrieve('" + coverURL + "', 'data/cover.jpg')").toString().find("data/cover.jpg") != std::string::npos)
			{
				coverImg.downscaleFrom(Image("data/cover.jpg"));
				accentColor = { 0.0f, 0.0f, 0.0f };
				for (int y = 0; y < coverImg.height(); ++y)
					for (int x = 0; x < coverImg.width(); ++x)
						accentColor += coverImg.getNC(x, y);
				accentColor /= { float(coverImg.width() * coverImg.height()) };
			}
			else
				std::cout << "[ ERR ]: Could not download " << coverURL << std::endl;
		}

		if (response.has("progress_ms") && response.has("item.duration_ms"))
		{
			int x = (WIDTH - seekbarWidth) / 2;
			int y = 195 + 45 / 2 - seekbarHeight / 2;
			int widthFilled = seekbarWidth * response["progress_ms"].getInteger() / response["item.duration_ms"].getInteger();
			int widthEmpty = seekbarWidth - widthFilled;
			fb.drawRect(x, y, widthFilled, seekbarHeight, seekbarColorFilled);
			fb.drawRect(x + widthFilled, y, widthEmpty, seekbarHeight, seekbarColorEmpty);
		}
		else
		{
			std::cout << "[ ERR ]: Invalid response:\n    " << response.toString() << std::endl;
			fb.clear({ 1.0f, 0.0f, 0.0f });
		}

		fb.drawRect(85, 45, 150, 150, accentColor);
		fb.drawImage(96, 56, coverImg);
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

	Image img(128, 128);
	img.downscaleFrom(Image(path));

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
