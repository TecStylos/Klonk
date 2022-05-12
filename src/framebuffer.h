#pragma once

#include <vector>
#include <fstream>

struct Color
{
	float r, g, b;
};

template <int W, int H>
class Framebuffer
{
public:
	Framebuffer();
public:
	void set(int x, int y, const Color& color);
	Color get(int x, int y) const;
	void clear(const Color& color);
	void flush();
public:
	void drawLine(int x1, int y1, int x2, int y2, const Color& color);
	void drawRect(int x, int y, int w, int h, const Color& color);
private:
	int getIndex(int x, int y) const;
	short toShort(const Color& color) const;
	Color toColor(short value) const;
private:
	std::vector<short> m_buff;
	std::ofstream m_file;
};

template <int W, int H>
Framebuffer<W,H>::Framebuffer()
	: m_buff(W * H), m_file("/dev/fb0", std::ios::binary)
{}

template <int W, int H>
void Framebuffer<W,H>::set(int x, int y, const Color& color)
{
	m_buff[getIndex(x, y)] = toShort(color);
}

template <int W, int H>
Color Framebuffer<W,H>::get(int x, int y) const
{
	return toColor(m_buff[getIndex(x, y)]);
}

template <int W, int H>
void Framebuffer<W,H>::clear(const Color& color)
{
	short value = toShort(color);
	for (auto& s : m_buff)
		s = value;
}

template <int W, int H>
void Framebuffer<W,H>::flush()
{
	m_file.seekp(std::ios::beg);
	m_file.write((const char*)m_buff.data(), W * H * sizeof(short));
	m_file.flush();
}

template <int W, int H>
void Framebuffer<W,H>::drawLine(int x1, int y1, int x2, int y2, const Color& color)
{
	short s = toShort(color);

	int dx = x2 - x1;
	int dy = y2 - y1;

	int nSteps = std::max(std::abs(dx), std::abs(dy));
	if (nSteps == 0) nSteps = 1;

	for (int step = 0; step < nSteps; ++step)
	{
		int x = x1 + dx * step / nSteps;
		if (x < 0 || x >= W)
			continue;
		int y = y1 + dy * step / nSteps;
		if (y < 0 || y >= H)
			continue;
		m_buff[getIndex(x, y)] = s;
	}
}

template <int W, int H>
void Framebuffer<W,H>::drawRect(int x, int y, int w, int h, const Color& color)
{
	short s = toShort(color);
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}

	int xe = std::min(W, x + w);
	int ye = std::min(H, y + h);

	for (int yc = y; yc < ye; ++yc)
		for (int xc = x; xc < xe; ++xc)
			m_buff[getIndex(xc, yc)] = s;
}

template <int W, int H>
int Framebuffer<W,H>::getIndex(int x, int y) const
{
	return W * y + x;
}

template <int W, int H>
short Framebuffer<W,H>::toShort(const Color& color) const
{
	return
		(int(color.r * 0b00011111) << 11) |
		(int(color.g * 0b00111111) << 5) |
		(int(color.b * 0b00011111) << 0);
}

template <int W, int H>
Color Framebuffer<W,H>::toColor(short value) const
{
	Color color;
	color.r = float((value >> 11) & 0b00011111) / 0b00011111;
	color.g = float((value >> 5) & 0b00111111) / 0b00111111;
	color.b = float((value >> 0) & 0b00011111) / 0b00011111;
	return color;
}
