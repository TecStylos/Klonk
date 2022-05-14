#pragma once

#include <fstream>

#include <unistd.h>
#include <linux/input.h>

struct TouchPos
{
	int x = 0;
	int y = 0;
};

template <int W, int H, int MIN, int MAX>
class Touch
{
public:
	Touch();
public:
	TouchPos pos() const;
	bool down() const;
	bool up() const;
	bool eventAvail();
	int fetchSingle();
	bool fetchToSync();
private:
	int convVal(int val, int newMax) const;
private:
	struct TouchInfo
	{
		TouchPos pos = {};
		bool isDown = false;
		int pressure = 0;
	} m_infoComplete, m_infoRolling;
	std::ifstream m_file;
};

template <int W, int H, int MIN, int MAX>
Touch<W,H,MIN,MAX>::Touch()
	: m_file("/dev/input/by-path/platform-20204000.spi-cs-1-event", std::ios::binary)
{}

template <int W, int H, int MIN, int MAX>
TouchPos Touch<W,H,MIN,MAX>::pos() const
{
	return m_infoComplete.pos;
}

template <int W, int H, int MIN, int MAX>
bool Touch<W,H,MIN,MAX>::down() const
{
	return m_infoComplete.isDown;
}

template <int W, int H, int MIN, int MAX>
bool Touch<W,H,MIN,MAX>::up() const
{
	return !down();
}

// Return values:
// < 0 -> No event read
// = 0 -> Normal event read
// > 0 -> Sync event read
template <int W, int H, int MIN, int MAX>
int Touch<W,H,MIN,MAX>::fetchSingle()
{
	input_event ie;
	m_file.read((char*)&ie, sizeof(ie));

	bool wasSync = false;
	switch (ie.type)
	{
	case EV_SYN:
		wasSync = true;
		m_infoComplete = m_infoRolling;
		break;
	case EV_KEY:
		m_infoRolling.isDown = ie.value;
		break;
	case EV_ABS:
		switch (ie.code)
		{
		case ABS_X:
			m_infoRolling.pos.x = W - convVal(ie.value, W);
			break;
		case ABS_Y:
			m_infoRolling.pos.y = convVal(ie.value, H);
			break;
		case ABS_PRESSURE:
			m_infoRolling.pressure = ie.value;
			break;
		}
		break;
	}

	return wasSync ? 1 : 0;
}

template <int W, int H, int MIN, int MAX>
bool Touch<W,H,MIN,MAX>::fetchToSync()
{
	int result;
	while (!(result = fetchSingle())); // Fetch a new event as long as a normal event is available
	return result > 0;
}


template <int W, int H, int MIN, int MAX>
int Touch<W,H,MIN,MAX>::convVal(int val, int newMax) const
{
	val = std::min(MAX, std::max(MIN, val));
	val -= MIN;
	val *= newMax;
	val /= MAX - MIN;
	return val;
}
