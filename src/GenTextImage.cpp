#include "GenTextImage.h"

#include <map>

Image genTextImage(const std::string& text, int height)
{
	static const Image srcFont = Image("resource/font.jpg");
	static const int firstChar = ' ';
	static const int lastChar = '~';
	static const int charWidth = srcFont.width() / (lastChar - firstChar + 1);
	static const int charHeight = srcFont.height();

	Image textImg(charWidth * text.size(), charHeight);

	for (int i = 0; i < text.size(); ++i)
	{
		char c = text[i];
		if (c < firstChar || lastChar < c)
			continue;

		int charIndex = c - firstChar;
		int srcOffset = charIndex * charWidth;
		int textOffset = i * charWidth;

		for (int y = 0; y < charHeight; ++y)
			for (int x = 0; x < charWidth; ++x)
				textImg(textOffset + x, y) = srcFont(srcOffset + x, y);
	}

	Image finalImg(textImg.width() * (height) / charHeight, height);
	finalImg.downscaleFrom(textImg);

	return finalImg;
}
