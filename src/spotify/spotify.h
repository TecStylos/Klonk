#pragma once

#include <mutex>
#include <string>
#include "Response.h"

class Spotify
{
public:
	Spotify();
	~Spotify();
private:
	void sendMessage(const std::string& message, bool ignoreResponse);
	std::string recvMessage();
public:
	Response exec(const std::string& command, bool ignoreResponse = true);
private:
	void close(int& fd);
	void readNum(void* buff, uint64_t num);
	void writeNum(const void* buff, uint64_t num);
private:
	struct Pipe
	{
		int fdRead = 0, fdWrite = 0;
	} m_cppToPy, m_pyToCpp;
	pid_t m_pid = 0;
	std::mutex m_mtx;
};
