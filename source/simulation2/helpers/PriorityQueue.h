/* Copyright (C) 2015 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_PRIORITYQUEUE
#define INCLUDED_PRIORITYQUEUE

/*
 * Priority queues for pathfinder.
 * (These probably aren't suitable for more general uses.)
 */

#ifdef NDEBUG
#define PRIORITYQUEUE_DEBUG 0
#else
#define PRIORITYQUEUE_DEBUG 1
#endif

template <typename Item, typename CMP>
struct QueueItemPriority
{
	bool operator()(const Item& a, const Item& b) const
	{
		if (CMP()(b.rank, a.rank)) // higher costs are lower priority
			return true;
		if (CMP()(a.rank, b.rank))
			return false;
		// Need to tie-break to get a consistent ordering
		if (CMP()(b.h, a.h)) // higher heuristic costs are lower priority
			return true;
		if (CMP()(a.h, b.h))
			return false;
		if (a.id < b.id)
			return true;
		if (b.id < a.id)
			return false;
#if PRIORITYQUEUE_DEBUG
		debug_warn(L"duplicate tiles in queue");
#endif
		return false;
	}
};


/**
 * Priority queue implemented as a binary heap.
 * This is quite dreadfully slow in MSVC's debug STL implementation,
 * so we shouldn't use it unless we reimplement the heap functions more efficiently.
 */
template <typename ID, typename R, typename H, typename CMP = std::less<R> >
class PriorityQueueHeap
{
public:
	struct Item
	{
		ID id;
		R rank; // f = g+h (estimated total cost of path through here)
		H h; // heuristic cost
	};

	void push(const Item& item)
	{
		m_Heap.push_back(item);
		push_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority<Item, CMP>());
	}

	void promote(ID id, R UNUSED(oldrank), R newrank, H newh)
	{
		// Loop backwards since it seems a little faster in practice
		for (ssize_t n = m_Heap.size() - 1; n >= 0; --n)
		{
			if (m_Heap[n].id == id)
			{
#if PRIORITYQUEUE_DEBUG
				ENSURE(m_Heap[n].rank > newrank);
#endif
				m_Heap[n].rank = newrank;
				m_Heap[n].h = newh;
				push_heap(m_Heap.begin(), m_Heap.begin()+n+1, QueueItemPriority<Item, CMP>());
				return;
			}
		}
	}

	Item pop()
	{
#if PRIORITYQUEUE_DEBUG
		ENSURE(m_Heap.size());
#endif
		Item r = m_Heap[0];
		pop_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority<Item, CMP>());
		m_Heap.pop_back();
		return r;
	}

	bool empty()
	{
		return m_Heap.empty();
	}

	size_t size()
	{
		return m_Heap.size();
	}

	void clear()
	{
		m_Heap.clear();
	}

	std::vector<Item> m_Heap;
};

/**
 * Priority queue implemented as an unsorted array.
 * This means pop() is O(n), but push and promote are O(1), and n is typically small
 * (average around 50-100 in some rough tests).
 * It seems fractionally slower than a binary heap in optimised builds, but is
 * much simpler and less susceptible to MSVC's painfully slow debug STL.
 */
template <typename ID, typename R, typename H, typename CMP = std::less<R> >
class PriorityQueueList
{
public:
	struct Item
	{
		ID id;
		R rank; // f = g+h (estimated total cost of path through here)
		H h; // heuristic cost
	};

	void push(const Item& item)
	{
		m_List.push_back(item);
	}

	Item* find(ID id)
	{
		for (size_t n = 0; n < m_List.size(); ++n)
		{
			if (m_List[n].id == id)
				return &m_List[n];
		}
		return NULL;
	}

	void promote(ID id, R UNUSED(oldrank), R newrank, H newh)
	{
		find(id)->rank = newrank;
		find(id)->h = newh;
	}

	Item pop()
	{
#if PRIORITYQUEUE_DEBUG
		ENSURE(m_List.size());
#endif
		// Loop backwards looking for the best (it's most likely to be one
		// we've recently pushed, so going backwards saves a bit of copying)
		Item best = m_List.back();
		size_t bestidx = m_List.size()-1;
		for (ssize_t i = (ssize_t)bestidx-1; i >= 0; --i)
		{
			if (QueueItemPriority<Item, CMP>()(best, m_List[i]))
			{
				bestidx = i;
				best = m_List[i];
			}
		}
		// Swap the matched element with the last in the list, then pop the new last
		m_List[bestidx] = m_List[m_List.size()-1];
		m_List.pop_back();
		return best;
	}

	bool empty()
	{
		return m_List.empty();
	}

	size_t size()
	{
		return m_List.size();
	}


	void clear()
	{
		m_List.clear();
	}

	std::vector<Item> m_List;
};

#endif // INCLUDED_PRIORITYQUEUE
