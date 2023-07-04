#pragma once

#include <vector>
#include <fstream>

#include "Image.h"

typedef Pixel Color;

class Framebuffer
{
public:
	Framebuffer(int width, int height);
public:
	int width() const;
	int height() const;
public:
	void set(int x, int y, const Color& color);
	Color get(int x, int y) const;
	void clear(const Color& color);
	void flush();
public:
	void drawLine(int x1, int y1, int x2, int y2, const Color& color);
	void drawRect(int x, int y, int w, int h, const Color& color);
	void drawImage(int x, int y, const Image& img);
private:
	int getIndex(int x, int y) const;
	short toShort(const Color& color) const;
	Color toColor(short value) const;
private:
	int m_width;
	int m_height;
	std::vector<short> m_buff;
	std::ofstream m_file;
};
