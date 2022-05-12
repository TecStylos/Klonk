#include <iostream>

#include "framebuffer.h"
#include "touch.h"
#include "spotify.h"

#include "Image.h"

#define WIDTH 320
#define HEIGHT 240

Color PixToCol(const Blomp::Pixel& pixel)
{
	Color color;
	color.r = pixel.r;
	color.g = pixel.g;
	color.b = pixel.b;
	return color;
}

int modeQuery()
{
	Spotify spotify;
        while (true)
        {
                std::string command;
                std::cout << " >> ";
                std::getline(std::cin, command);
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

int modePlayback()
{
	Framebuffer<WIDTH, HEIGHT> fb;
	fb.flush();

	Spotify spotify;

	Response response;
	int trackPos = 0;
	int trackLen = 1;

	while (true)
	{
		response = spotify.exec("spotify.currently_playing()");
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

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " [mode]" << std::endl;
		return 1;
	}

	std::string mode = argv[1];

	if (mode == "query")
		return modeQuery();

	if (mode == "playback")
		return modePlayback();

	std::cout << "Unknown mode '" << mode << "'!" << std::endl;

	return 1;
}
