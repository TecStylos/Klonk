#include "Response.h"

#include <stdexcept>
#include <cstring>
#include <vector>

Response::Response(const std::string& str)
	: Response(str.c_str())
{}

Response::Response(const char* str)
	: Response(peekToken(str), readToken(str, nullptr), uint64_t(readToken(str, nullptr) - str))
{}

Response::Response(Token tok, const char* str, uint64_t tokReadSize)
{
	const char* begin = str;

	switch (tok.type)
	{
	case Token::Type::None:
		m_type = ResponseType::None;
		break;
	case Token::Type::Boolean:
		m_type = ResponseType::Boolean;
		m_boolean = tok.value == "True";
		break;
	case Token::Type::Integer:
		m_type = ResponseType::Integer;
		m_integer = std::stoll(tok.value);
		break;
	case Token::Type::String:
		m_type = ResponseType::String;
		m_string = tok.value;
		break;
	case Token::Type::ListBegin:
		m_type = ResponseType::List;
		str = readToken(str, &tok);
		while (tok.type != Token::Type::ListEnd)
		{
			m_list.push_back(Response(tok, str));
			str += m_list.back().m_numRead;

			str = readToken(str, &tok);
			if (tok.type == Token::Type::ElemSeparator)
				str = readToken(str, &tok);
		}
		break;
	case Token::Type::DictBegin:
		m_type = ResponseType::Dict;
		str = readToken(str, &tok);
		while (tok.type != Token::Type::DictEnd)
		{
			if (tok.type != Token::Type::String)
				throw std::runtime_error("Expected string as dict element key!");
			auto name = tok.value;

			str = readToken(str, &tok);
			if (tok.type != Token::Type::PairSeparator)
				throw std::runtime_error("Expected separator between dict key and value!");

			str = readToken(str, &tok);
			str += m_dict.insert({ name, Response(tok, str) }).first->second.m_numRead;

			str = readToken(str, &tok);
			if (tok.type == Token::Type::ElemSeparator)
				str = readToken(str, &tok);
		}
		break;
	default:
		throw std::runtime_error("Unhandled token type!");
	}

	m_numRead = tokReadSize + uint64_t(str - begin);
}

std::string Response::toString() const
{
	std::string str;

	switch (m_type)
	{
	case ResponseType::None:
		str += "None";
		break;
	case ResponseType::Boolean:
		str += m_boolean ? "True" : "False";
		break;
	case ResponseType::Integer:
		str += std::to_string(m_integer);
		break;
	case ResponseType::String:
		str += "'" + m_string + "'";
		break;
	case ResponseType::List:
	{
		str += "[ ";
		int i = 0;
		for (auto& elem : m_list)
		{
			str += elem.toString();
			if (++i < m_list.size())
				str += ", ";
		}
		str += " ]";
		break;
	}
	case ResponseType::Dict:
	{
		str += "{ ";
		int i = 0;
		for (auto& [name, elem] : m_dict)
		{
			str += "'" + name + "': " + elem.toString();
			if (++i < m_dict.size())
				str += ", ";
		}
		str += " }";
		break;
	}
	}
	return str;
}

bool Response::has(const std::string& key) const
{
	uint64_t pos = key.find('.');
	auto first = key.substr(0, pos);

	bool isLastKey = pos == std::string::npos;

	try
	{
		uint64_t index = std::stoll(first);
		if (!has(index))
			return false;
		if (!isLastKey && !m_list[index].has(key.substr(pos + 1)))
			return false;
	}
	catch (std::invalid_argument)
	{
		if (m_dict.find(first) == m_dict.end())
			return false;
		if (!isLastKey && !m_dict.find(first)->second.has(key.substr(pos + 1)))
			return false;
	}

	return true;
}

const Response& Response::operator[](const std::string& key) const
{
	uint64_t pos = key.find('.');
	auto first = key.substr(0, pos);

	bool isLastKey = pos == std::string::npos;

	try
	{
		uint64_t index = std::stoll(first);
		return isLastKey ? m_list[index] : m_list[index][key.substr(pos + 1)];
	}
	catch (std::invalid_argument)
	{
		return isLastKey ? m_dict.find(first)->second : m_dict.find(first)->second[key.substr(pos + 1)];
	}
}

Response::Token Response::peekToken(const char* str)
{
	Token tok;
	readToken(str, &tok);
	return tok;
}

const char* Response::readToken(const char* str, Token* pToken)
{
	while (std::isspace(*str))
		++str;

	Token tok;

	enum class State
	{
		BeginToken,
		EndToken,
		TokNone,
		TokBoolean,
		TokInteger,
		TokString
	} state = State::BeginToken;

	while (*str && state != State::EndToken)
	{
		char c = *str++;
		switch (state)
		{
		case State::BeginToken:
			switch (c)
			{
			case 'N':
				tok.type = Token::Type::None;
				tok.value.push_back(c);
				state = State::TokNone;
				break;
			case 'T':
			case 'F':
				tok.type = Token::Type::Boolean;
				tok.value.push_back(c);
				state = State::TokBoolean;
				break;
			case '\'':
				tok.type = Token::Type::String;
				state = State::TokString;
				break;
			case '[':
				tok.type = Token::Type::ListBegin;
                                tok.value.push_back(c);
				state = State::EndToken;
				break;
			case ']':
				tok.type = Token::Type::ListEnd;
                                tok.value.push_back(c);
				state = State::EndToken;
				break;
			case '{':
				tok.type = Token::Type::DictBegin;
                                tok.value.push_back(c);
				state = State::EndToken;
				break;
			case '}':
				tok.type = Token::Type::DictEnd;
                                tok.value.push_back(c);
				state = State::EndToken;
				break;
			case ',':
				tok.type = Token::Type::ElemSeparator;
                                tok.value.push_back(c);
				state = State::EndToken;
				break;
			case ':':
				tok.type = Token::Type::PairSeparator;
				tok.value.push_back(c);
				state = State::EndToken;
				break;
			default:
				if (std::isdigit(c) || c == '-')
				{
					tok.type = Token::Type::Integer;
					tok.value.push_back(c);
					state = State::TokInteger;
				}
				else if (!std::isspace(c))
				{
					throw std::runtime_error("Unexpected character!");
				}
			}
			break;
		case State::TokNone:
			tok.value.push_back(c);
			if (tok.value == "None")
				state = State::EndToken;
			if (strncmp("None", tok.value.c_str(), tok.value.size()))
				throw std::runtime_error("Unexpected character while parsing 'None'!");
			break;
		case State::TokBoolean:
			tok.value.push_back(c);
			if (tok.value == "True" || tok.value == "False")
				state = State::EndToken;
			if (strncmp("True", tok.value.c_str(), tok.value.size()) && strncmp("False", tok.value.c_str(), tok.value.size()))
				throw std::runtime_error("Unexpected character while parsing boolean!");
			break;
		case State::TokInteger:
			if (std::isdigit(c))
				tok.value.push_back(c);
			else
			{
				--str;
				state = State::EndToken;
			}
			break;
		case State::TokString:
			if (c == '\'')
				state = State::EndToken;
			else
				tok.value.push_back(c);
			break;
		default:
			throw std::runtime_error("Unhandled tokenizer state!");
		}
	}

	if (state != State::EndToken)
		throw std::runtime_error("Unexpected EOF while tokenizing!");

	if (pToken)
		*pToken = tok;

	return str;
}
