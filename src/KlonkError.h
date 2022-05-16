#pragma once

#include <stdexcept>
#include <string>

class KlonkError : public std::runtime_error
{
public:
	KlonkError(const std::string& message)
		: std::runtime_error("KlonkError"), m_what(message)
	{}
public:
	virtual const char* what() const noexcept override
	{ return m_what.c_str(); }
private:
	std::string m_what;
};
