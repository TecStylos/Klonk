#pragma once

#include <queue>
#include <mutex>

template <typename T>
class SharedQueue
{
public:
	SharedQueue() = default;
	SharedQueue(const SharedQueue& other) = default;
	~SharedQueue() = default;
public:
	// Can only be used by one supplier
	bool empty() const
	{
		std::lock_guard lock(m_mtx);
		return m_queue.empty();
	}
	T pop()
	{
		std::lock_guard lock(m_mtx);
		T elem = m_queue.front();
		m_queue.pop();
		return elem;
	}
	void push(const T& elem)
	{
		std::lock_guard lock(m_mtx);
		m_queue.push(elem);
	}
private:
	std::queue<T> m_queue;
	mutable std::mutex m_mtx;
};
