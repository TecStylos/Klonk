#include "framebuffer.h"

Framebuffer::Framebuffer(int width, int height)
        : m_width(width), m_height(height), m_buff(width * height), m_file("/dev/fb0", std::ios::binary)
{}

int Framebuffer::width() const
{
	return m_width;
}

int Framebuffer::height() const
{
	return m_height;
}

void Framebuffer::set(int x, int y, const Color& color)
{
        m_buff[getIndex(x, y)] = toShort(color);
}

Color Framebuffer::get(int x, int y) const
{
        return toColor(m_buff[getIndex(x, y)]);
}

void Framebuffer::clear(const Color& color)
{
        short value = toShort(color);
        for (auto& s : m_buff)
                s = value;
}

void Framebuffer::flush()
{
        m_file.seekp(std::ios::beg);
        m_file.write((const char*)m_buff.data(), m_width * m_height * sizeof(short));
        m_file.flush();
}

void Framebuffer::drawLine(int x1, int y1, int x2, int y2, const Color& color)
{
        short s = toShort(color);

        int dx = x2 - x1;
        int dy = y2 - y1;

        int nSteps = std::max(std::abs(dx), std::abs(dy));
        if (nSteps == 0) nSteps = 1;

        for (int step = 0; step < nSteps; ++step)
        {
                int x = x1 + dx * step / nSteps;
                if (x < 0 || x >= m_width)
                        continue;
                int y = y1 + dy * step / nSteps;
                if (y < 0 || y >= m_height)
                        continue;
                m_buff[getIndex(x, y)] = s;
        }
}

void Framebuffer::drawRect(int x, int y, int w, int h, const Color& color)
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

        int xe = std::min(m_width, x + w);
        int ye = std::min(m_height, y + h);

        for (int yc = y; yc < ye; ++yc)
                for (int xc = x; xc < xe; ++xc)
                        m_buff[getIndex(xc, yc)] = s;
}

void Framebuffer::drawImage(int x, int y, const Image& img)
{
        int w = std::min(img.width(), m_width - x);
        int h = std::min(img.height(), m_height - y);

        int bx = x < 0 ? -x : 0;
        int by = y < 0 ? -y : 0;

        for (int oy = by; oy < h; ++oy)
                for (int ox = bx; ox < w; ++ox)
                {
                        auto& c = img.getNC(ox, oy);
                        if (c.transparent)
                                continue;
                        set(x + ox, y + oy, c);
                }
}

int Framebuffer::getIndex(int x, int y) const
{
        return m_width * y + x;
}
short Framebuffer::toShort(const Color& color) const
{
        return
                (short(color.r * 0b00011111) << 11) |
                (short(color.g * 0b00111111) << 5) |
                (short(color.b * 0b00011111) << 0);
}

Color Framebuffer::toColor(short value) const
{
        Color color;
        color.r = float((value >> 11) & 0b00011111) / 0b00011111;
        color.g = float((value >> 5) & 0b00111111) / 0b00111111;
        color.b = float((value >> 0) & 0b00011111) / 0b00011111;
        return color;
}
