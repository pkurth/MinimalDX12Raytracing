#pragma once


template <typename T>
struct LinkedList
{
	T* first = nullptr;
	T* last = nullptr;

	bool empty()
	{
		return first == nullptr;
	}

	void push_front(T* item)
	{
		item->next = first;
		first = item;
		if (!last)
		{
			last = item;
		}
	}

	void push_back(T* item)
	{
		item->next = nullptr;
		if (last)
		{
			last->next = item;
		}
		last = item;
		if (!first)
		{
			first = item;
		}
	}

	T* pop_front()
	{
		T* result = first;
		if (first)
		{
			first = first->next;
		}
		if (last == result)
		{
			last = nullptr;
		}
		return result;
	}

	T* peek_front()
	{
		return first;
	}

	void remove(T* item, T* before)
	{
		if (before)
		{
			before->next = item->next;
		}
		if (first == item)
		{
			first = item->next;
		}
		if (last == item)
		{
			last = before;
		}
	}

	u32 count()
	{
		T* item = first;
		u32 result = 0;
		while (item)
		{
			item = item->next;
			++result;
		}
		return result;
	}
};
