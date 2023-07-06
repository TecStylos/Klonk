#include "spotify.h"

#include <unistd.h>
#include <vector>
#include <sstream>

Spotify::Spotify()
{
	if (pipe((int*)&m_cppToPy) || pipe((int*)&m_pyToCpp))
		throw std::runtime_error("Unable to open pipes!");

	m_pid = fork();

	if (m_pid == 0)
	{
		std::ostringstream oss;
		oss << "export PY_READ_FILE_DESC=" << m_cppToPy.fdRead << " && "
		    << "export PY_WRITE_FILE_DESC=" << m_pyToCpp.fdWrite << " && "
		    << "export PYTHONUNBUFFERED=true && "
		    << "./runSpotify.sh";

		auto cmd = oss.str();
		system(cmd.c_str());
		exit(0);
	}
	else if (m_pid < 0)
	{
		throw std::runtime_error("Fork failed!");
	}
}

Spotify::~Spotify()
{
	close(m_pyToCpp.fdRead);
	close(m_pyToCpp.fdWrite);
	close(m_cppToPy.fdRead);
	close(m_cppToPy.fdWrite);
}

void Spotify::sendMessage(const std::string& message, bool ignoreResponse)
{
	uint32_t msgSize = message.size();
	writeNum(&ignoreResponse, sizeof(ignoreResponse));
	writeNum(&msgSize, sizeof(msgSize));
	writeNum(message.c_str(), msgSize);
}

std::string Spotify::recvMessage()
{
	uint32_t msgSize;
	readNum(&msgSize, sizeof(msgSize));
	std::vector<char> buff(msgSize + 1);
	readNum(buff.data(), msgSize);
	buff[msgSize] = '\0';
	return std::string(buff.data());
}

Response Spotify::exec(const std::string& command, bool ignoreResponse)
{
	static const Response ignoredResponse = Response(std::string("{ \"type\": \"IgnoredResponse\" }"));
	
	std::lock_guard lock(m_mtx);

	sendMessage(command, ignoreResponse);

	if (ignoreResponse)
		return ignoredResponse;

	return Response(recvMessage());
}

void Spotify::readNum(void* buff, uint64_t num)
{
	while (num > 0)
	{
		ssize_t nRead = read(m_pyToCpp.fdRead, buff, num);
		if (nRead == 0)
			throw std::runtime_error("Read nothing!");
		else if (nRead < 0)
			throw std::runtime_error("Reached EOF!");
		else
		{
			buff = (char*)buff + nRead;
			num -= nRead;
		}
	}
}

void Spotify::writeNum(const void* buff, uint64_t num)
{
	while (num > 0)
	{
		ssize_t nWritten = write(m_cppToPy.fdWrite, buff, num);
		if (nWritten == 0)
			throw std::runtime_error("Wrote nothing!");
		else if (nWritten < 0)
			throw std::runtime_error("Unable to write!");
		else
		{
			buff = (char*)buff + nWritten;
			num -= nWritten;
		}
	}
}

void Spotify::close(int& fd)
{
	if (fd)
	{
		::close(fd);
		fd = 0;
	}
}
