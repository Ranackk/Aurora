#pragma once

#include <memory>
#include <mutex>

/*
* A fixed size in-place ring buffer on the stack
* Uses a head and a tail iterator to keep track of used and unused elements
* 
* Provides interface to 
*   - invoke a callback on the head element and increment the head iterator (push)
*   - return the tail element and increment the tail iterator (pop)
* 
* Particular useful to queue small POD objects that can be initialized by a callback and need to be read only once
*/
template <class T, const size_t SIZE>
class CircularBufferInPlace
{

private:
	std::mutex		m_Mutex;

	T				m_Buffer[SIZE];
	size_t			m_Head	= 0;
	size_t			m_Tail	= 0;

	bool			m_Full	= false;

public:
	explicit CircularBufferInPlace() : m_Buffer() {}

	// Invokes func on the head element. Also increments the head element iterator 
	template<typename Func, typename... Args>
	void CallHeadAndIncrement(Func func, Args... args)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		func(m_Buffer[m_Head], args...);

		if (m_Full)
		{
			assert(!"CallHeadAndIncrement called but head is already at tail. Will overwrite tail element");
			m_Tail = (m_Tail + 1) % SIZE;
		}

		m_Head = (m_Head + 1) % SIZE;
		m_Full = m_Head == m_Tail;
	}

	//////////////////////////////////////////////////////////////////////////

	// Returns the tail element. Also increments the tail element iterator
	const T& GetTailAndIncrement()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (IsEmpty())
		{
			assert(!"GetTailAndIncrement called but tail is already at head. Will return last valid element and not increment.");
			return m_Buffer[m_Tail];
		}

		T& returnValue = m_Buffer[m_Tail];

		m_Full = false;
		m_Tail = (m_Tail + 1) % SIZE;

		return returnValue;
	}

	//////////////////////////////////////////////////////////////////////////

	void Reset()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Head = m_Tail;
		m_Full = false;
	}

	//////////////////////////////////////////////////////////////////////////

	inline bool isFull() const 
	{
		return m_Full;
	}

	//////////////////////////////////////////////////////////////////////////

	inline bool IsEmpty() const
	{
		return !m_Full && m_Head == m_Tail;
	}

	//////////////////////////////////////////////////////////////////////////

	inline size_t GetCapacity() const 
	{
		return SIZE;
	}

	//////////////////////////////////////////////////////////////////////////

	inline size_t GetCurrentSize() const 
	{
		size_t size = SIZE;

		if (!m_Full)
		{
			if (m_Head >= m_Tail)
			{
				size = m_Head - m_Tail;
			}
			else
			{
				size = SIZE + m_Head - m_Tail;
			}
		}

		return size;
	}

	//////////////////////////////////////////////////////////////////////////
};