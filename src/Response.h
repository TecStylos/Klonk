#pragma once

#include <string>
#include <vector>
#include <map>

enum class ResponseType { None, Boolean, Integer, Float, String, List, Dict };

class Response
{
private:
	struct Token
        {
                enum class Type { None, Boolean, Integer, Float, String, ListBegin, ListEnd, DictBegin, DictEnd, ElemSeparator, PairSeparator };
                Type type;
                std::string value;
        };
public:
	Response() = default;
	Response(const std::string& str);
private:
	Response(const char* str);
	Response(Token tok, const char* str, uint64_t tokReadSize = 0);
public:
	ResponseType getType() const { return m_type; }
	bool isNone() const { return m_type == ResponseType::None; }
	bool isBoolean() const { return m_type == ResponseType::Boolean; }
	bool isInteger() const { return m_type == ResponseType::Integer; }
	bool isFloat() const { return m_type == ResponseType::Float; }
	bool isString() const { return m_type == ResponseType::String; }
	bool isList() const { return m_type == ResponseType::List; }
	bool isDict() const { return m_type == ResponseType::Dict; }
	const std::string& getString() const { return m_string; }
	bool getBoolean() const { return m_boolean; }
	int64_t getInteger() const { return m_integer; }
	float getFloat() const { return m_float; }
	const std::vector<Response>& getList() const { return m_list; }
	const std::map<std::string, Response>& getDict() const { return m_dict; }
	std::string toString() const;
public:
	uint64_t size() const { return m_list.size(); }
	bool has(uint64_t index) const { return index < size(); }
	bool has(const std::string& key) const;
	const Response& operator[](uint64_t index) const { return m_list[index]; }
	const Response& operator[](const std::string& key) const;
private:
	static Token peekToken(const char* str);
        static const char* readToken(const char* str, Token* pToken);
private:
	uint64_t m_numRead = 0;
	ResponseType m_type = ResponseType::None;
	bool m_boolean = false;
	int64_t m_integer = 0;
	float m_float = 0.0f;
	std::string m_string = "";
	std::vector<Response> m_list = {};
	std::map<std::string, Response> m_dict = {};
};
