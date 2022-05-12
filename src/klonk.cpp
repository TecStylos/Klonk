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

int main(int argc, char** argv)
{
	Framebuffer<WIDTH,HEIGHT> fb;
	fb.flush();

	if (argc == 2)
	{
		Blomp::Image img(argv[1]);
		int width = std::min(img.width(), WIDTH);
		int height = std::min(img.height(), HEIGHT);

		for (int x = 0; x < width; ++x)
			for (int y = 0; y < height; ++y)
				fb.set(x, y, PixToCol(img.get(x, y)));
		fb.flush();
		return 0;
	}

	Spotify spotify;
	while (true)
	{
		std::string command;
		std::cout << " >> ";
		std::getline(std::cin, command);
		auto response = spotify.exec(command);
		std::cout << "RESPONSE: " << response.toString() << std::endl;
	}

	for (int y = 0; y < HEIGHT; ++y)
		for (int x = 0; x < WIDTH; ++x)
			fb.set(x, y, { float(x) / WIDTH, float(y) / HEIGHT, 0.0f });

	fb.flush();
	fb.clear({ 0.0f, 0.0f, 0.0f });
	sleep(1);
	fb.flush();

	Touch<WIDTH,HEIGHT,200,3800> t;

	for (;;)
	{
		t.fetchToSync();

		TouchPos pos = t.pos();

		if (pos.x < 10 && pos.y < 10 && t.down())
		{
			fb.clear({ 0.0f, 0.0f, 0.0f });
			for (int y = 0; y < 10; ++y)
				for (int x = 0; x < 10; ++x)
					fb.set(x, y, { 1.0f, 0.0f, 0.0f });
			fb.flush();
		}
		else if (t.down())
		{
			fb.set(pos.x, pos.y, { 1.0f, 1.0f, 1.0f });
			fb.flush();
		}
	}

	return 0;
}
