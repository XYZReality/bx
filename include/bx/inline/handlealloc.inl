/*
 * Copyright 2010-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HANDLE_ALLOC_H_HEADER_GUARD
#	error "Must be included from bx/handlealloc.h!"
#endif // BX_HANDLE_ALLOC_H_HEADER_GUARD

namespace bx
{
	inline HandleAlloc::HandleAlloc(bgfx_handle _maxHandles)
		: m_numHandles(0)
		, m_maxHandles(_maxHandles)
	{
		reset();
	}

	inline HandleAlloc::~HandleAlloc()
	{
	}

	inline const bgfx_handle* HandleAlloc::getHandles() const
	{
		return getDensePtr();
	}

	inline bgfx_handle HandleAlloc::getHandleAt(bgfx_handle _at) const
	{
		return getDensePtr()[_at];
	}

	inline bgfx_handle HandleAlloc::getNumHandles() const
	{
		return m_numHandles;
	}

	inline bgfx_handle HandleAlloc::getMaxHandles() const
	{
		return m_maxHandles;
	}

	inline bgfx_handle HandleAlloc::alloc()
	{
		if (m_numHandles < m_maxHandles)
		{
			bgfx_handle index = m_numHandles;
			++m_numHandles;

			bgfx_handle* dense  = getDensePtr();
			bgfx_handle  handle = dense[index];
			bgfx_handle* sparse = getSparsePtr();
			sparse[handle] = index;
			return handle;
		}

		return kInvalidHandle;
	}

	inline bool HandleAlloc::isValid(bgfx_handle _handle) const
	{
		bgfx_handle* dense  = getDensePtr();
		bgfx_handle* sparse = getSparsePtr();
		bgfx_handle  index  = sparse[_handle];

		return index < m_numHandles
			&& dense[index] == _handle
			;
	}

	inline void HandleAlloc::free(bgfx_handle _handle)
	{
		bgfx_handle* dense  = getDensePtr();
		bgfx_handle* sparse = getSparsePtr();
		bgfx_handle index = sparse[_handle];
		--m_numHandles;
		bgfx_handle temp = dense[m_numHandles];
		dense[m_numHandles] = _handle;
		sparse[temp] = index;
		dense[index] = temp;
	}

	inline void HandleAlloc::reset()
	{
		m_numHandles = 0;
		bgfx_handle* dense = getDensePtr();
		for (bgfx_handle ii = 0, num = m_maxHandles; ii < num; ++ii)
		{
			dense[ii] = ii;
		}
	}

	inline bgfx_handle* HandleAlloc::getDensePtr() const
	{
		uint8_t* ptr = (uint8_t*)reinterpret_cast<const uint8_t*>(this);
		return (bgfx_handle*)&ptr[sizeof(HandleAlloc)];
	}

	inline bgfx_handle* HandleAlloc::getSparsePtr() const
	{
		return &getDensePtr()[m_maxHandles];
	}

	inline HandleAlloc* createHandleAlloc(AllocatorI* _allocator, bgfx_handle _maxHandles)
	{
		uint8_t* ptr = (uint8_t*)BX_ALLOC(_allocator, sizeof(HandleAlloc) + 2*_maxHandles*sizeof(bgfx_handle) );
		return BX_PLACEMENT_NEW(ptr, HandleAlloc)(_maxHandles);
	}

	inline void destroyHandleAlloc(AllocatorI* _allocator, HandleAlloc* _handleAlloc)
	{
		_handleAlloc->~HandleAlloc();
		BX_FREE(_allocator, _handleAlloc);
	}

	template <bgfx_handle MaxHandlesT>
	inline HandleAllocT<MaxHandlesT>::HandleAllocT()
		: HandleAlloc(MaxHandlesT)
	{
	}

	template <bgfx_handle MaxHandlesT>
	inline HandleAllocT<MaxHandlesT>::~HandleAllocT()
	{
	}

	template <bgfx_handle MaxHandlesT>
	inline HandleListT<MaxHandlesT>::HandleListT()
	{
		reset();
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::pushBack(bgfx_handle _handle)
	{
		insertAfter(m_back, _handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleListT<MaxHandlesT>::popBack()
	{
		bgfx_handle last = kInvalidHandle != m_back
			? m_back
			: m_front
			;

		if (kInvalidHandle != last)
		{
			remove(last);
		}

		return last;
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::pushFront(bgfx_handle _handle)
	{
		insertBefore(m_front, _handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleListT<MaxHandlesT>::popFront()
	{
		bgfx_handle front = m_front;

		if (kInvalidHandle != front)
		{
			remove(front);
		}

		return front;
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleListT<MaxHandlesT>::getFront() const
	{
		return m_front;
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleListT<MaxHandlesT>::getBack() const
	{
		return m_back;
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleListT<MaxHandlesT>::getNext(bgfx_handle _handle) const
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		const Link& curr = m_links[_handle];
		return curr.m_next;
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleListT<MaxHandlesT>::getPrev(bgfx_handle _handle) const
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		const Link& curr = m_links[_handle];
		return curr.m_prev;
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::remove(bgfx_handle _handle)
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		Link& curr = m_links[_handle];

		if (kInvalidHandle != curr.m_prev)
		{
			Link& prev  = m_links[curr.m_prev];
			prev.m_next = curr.m_next;
		}
		else
		{
			m_front = curr.m_next;
		}

		if (kInvalidHandle != curr.m_next)
		{
			Link& next  = m_links[curr.m_next];
			next.m_prev = curr.m_prev;
		}
		else
		{
			m_back = curr.m_prev;
		}

		curr.m_prev = kInvalidHandle;
		curr.m_next = kInvalidHandle;
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::reset()
	{
		memSet(m_links, 0xff, sizeof(m_links) );
		m_front = kInvalidHandle;
		m_back  = kInvalidHandle;
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::insertBefore(bgfx_handle _before, bgfx_handle _handle)
	{
		Link& curr = m_links[_handle];
		curr.m_next = _before;

		if (kInvalidHandle != _before)
		{
			Link& link = m_links[_before];
			if (kInvalidHandle != link.m_prev)
			{
				Link& prev = m_links[link.m_prev];
				prev.m_next = _handle;
			}

			curr.m_prev = link.m_prev;
			link.m_prev = _handle;
		}

		updateFrontBack(_handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::insertAfter(bgfx_handle _after, bgfx_handle _handle)
	{
		Link& curr = m_links[_handle];
		curr.m_prev = _after;

		if (kInvalidHandle != _after)
		{
			Link& link = m_links[_after];
			if (kInvalidHandle != link.m_next)
			{
				Link& next = m_links[link.m_next];
				next.m_prev = _handle;
			}

			curr.m_next = link.m_next;
			link.m_next = _handle;
		}

		updateFrontBack(_handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline bool HandleListT<MaxHandlesT>::isValid(bgfx_handle _handle) const
	{
		return _handle < MaxHandlesT;
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::updateFrontBack(bgfx_handle _handle)
	{
		Link& curr = m_links[_handle];

		if (kInvalidHandle == curr.m_prev)
		{
			m_front = _handle;
		}

		if (kInvalidHandle == curr.m_next)
		{
			m_back = _handle;
		}
	}

	template <bgfx_handle MaxHandlesT>
	inline HandleAllocLruT<MaxHandlesT>::HandleAllocLruT()
	{
		reset();
	}

	template <bgfx_handle MaxHandlesT>
	inline HandleAllocLruT<MaxHandlesT>::~HandleAllocLruT()
	{
	}

	template <bgfx_handle MaxHandlesT>
	inline const bgfx_handle* HandleAllocLruT<MaxHandlesT>::getHandles() const
	{
		return m_alloc.getHandles();
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleAllocLruT<MaxHandlesT>::getHandleAt(bgfx_handle _at) const
	{
		return m_alloc.getHandleAt(_at);
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleAllocLruT<MaxHandlesT>::getNumHandles() const
	{
		return m_alloc.getNumHandles();
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleAllocLruT<MaxHandlesT>::getMaxHandles() const
	{
		return m_alloc.getMaxHandles();
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleAllocLruT<MaxHandlesT>::alloc()
	{
		bgfx_handle handle = m_alloc.alloc();
		if (kInvalidHandle != handle)
		{
			m_list.pushFront(handle);
		}
		return handle;
	}

	template <bgfx_handle MaxHandlesT>
	inline bool HandleAllocLruT<MaxHandlesT>::isValid(bgfx_handle _handle) const
	{
		return m_alloc.isValid(_handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::free(bgfx_handle _handle)
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		m_list.remove(_handle);
		m_alloc.free(_handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::touch(bgfx_handle _handle)
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		m_list.remove(_handle);
		m_list.pushFront(_handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleAllocLruT<MaxHandlesT>::getFront() const
	{
		return m_list.getFront();
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleAllocLruT<MaxHandlesT>::getBack() const
	{
		return m_list.getBack();
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleAllocLruT<MaxHandlesT>::getNext(bgfx_handle _handle) const
	{
		return m_list.getNext(_handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline bgfx_handle HandleAllocLruT<MaxHandlesT>::getPrev(bgfx_handle _handle) const
	{
		return m_list.getPrev(_handle);
	}

	template <bgfx_handle MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::reset()
	{
		m_list.reset();
		m_alloc.reset();
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline HandleHashMapT<MaxCapacityT, KeyT>::HandleHashMapT()
		: m_maxCapacity(MaxCapacityT)
	{
		reset();
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline HandleHashMapT<MaxCapacityT, KeyT>::~HandleHashMapT()
	{
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::insert(KeyT _key, bgfx_handle _handle)
	{
		if (kInvalidHandle == _handle)
		{
			return false;
		}

		const KeyT hash = mix(_key);
		const uint32_t firstIdx = hash % MaxCapacityT;
		uint32_t idx = firstIdx;
		do
		{
			if (m_handle[idx] == kInvalidHandle)
			{
				m_key[idx]    = _key;
				m_handle[idx] = _handle;
				++m_numElements;
				return true;
			}

			if (m_key[idx] == _key)
			{
				return false;
			}

			idx = (idx + 1) % MaxCapacityT;

		} while (idx != firstIdx);

		return false;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::removeByKey(KeyT _key)
	{
		uint32_t idx = findIndex(_key);
		if (UINT32_MAX != idx)
		{
			removeIndex(idx);
			return true;
		}

		return false;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::removeByHandle(bgfx_handle _handle)
	{
		if (kInvalidHandle != _handle)
		{
			for (uint32_t idx = 0; idx < MaxCapacityT; ++idx)
			{
				if (m_handle[idx] == _handle)
				{
					removeIndex(idx);
				}
			}
		}

		return false;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bgfx_handle HandleHashMapT<MaxCapacityT, KeyT>::find(KeyT _key) const
	{
		uint32_t idx = findIndex(_key);
		if (UINT32_MAX != idx)
		{
			return m_handle[idx];
		}

		return kInvalidHandle;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline void HandleHashMapT<MaxCapacityT, KeyT>::reset()
	{
		memSet(m_handle, 0xff, sizeof(m_handle) );
		m_numElements = 0;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint32_t HandleHashMapT<MaxCapacityT, KeyT>::getNumElements() const
	{
		return m_numElements;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint32_t HandleHashMapT<MaxCapacityT, KeyT>::getMaxCapacity() const
	{
		return m_maxCapacity;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline typename HandleHashMapT<MaxCapacityT, KeyT>::Iterator HandleHashMapT<MaxCapacityT, KeyT>::first() const
	{
		Iterator it;
		it.handle = kInvalidHandle;
		it.pos    = 0;
		it.num    = m_numElements;

		if (0 == it.num)
		{
			return it;
		}

		++it.num;
		next(it);
		return it;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::next(Iterator& _it) const
	{
		if (0 == _it.num)
		{
			return false;
		}

		for (
			;_it.pos < MaxCapacityT && kInvalidHandle == m_handle[_it.pos]
			; ++_it.pos
			);
		_it.handle = m_handle[_it.pos];
		++_it.pos;
		--_it.num;
		return true;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint32_t HandleHashMapT<MaxCapacityT, KeyT>::findIndex(KeyT _key) const
	{
		const KeyT hash = mix(_key);

		const uint32_t firstIdx = hash % MaxCapacityT;
		uint32_t idx = firstIdx;
		do
		{
			if (m_handle[idx] == kInvalidHandle)
			{
				return UINT32_MAX;
			}

			if (m_key[idx] == _key)
			{
				return idx;
			}

			idx = (idx + 1) % MaxCapacityT;

		} while (idx != firstIdx);

		return UINT32_MAX;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline void HandleHashMapT<MaxCapacityT, KeyT>::removeIndex(uint32_t _idx)
	{
		m_handle[_idx] = kInvalidHandle;
		--m_numElements;

		for (uint32_t idx = (_idx + 1) % MaxCapacityT
				; m_handle[idx] != kInvalidHandle
				; idx = (idx + 1) % MaxCapacityT)
		{
			if (m_handle[idx] != kInvalidHandle)
			{
				const KeyT key = m_key[idx];
				if (idx != findIndex(key) )
				{
					const bgfx_handle handle = m_handle[idx];
					m_handle[idx] = kInvalidHandle;
					--m_numElements;
					insert(key, handle);
				}
			}
		}
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint32_t HandleHashMapT<MaxCapacityT, KeyT>::mix(uint32_t _x) const
	{
		const uint32_t tmp0   = uint32_mul(_x,   UINT32_C(2246822519) );
		const uint32_t tmp1   = uint32_rol(tmp0, 13);
		const uint32_t result = uint32_mul(tmp1, UINT32_C(2654435761) );
		return result;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint64_t HandleHashMapT<MaxCapacityT, KeyT>::mix(uint64_t _x) const
	{
		const uint64_t tmp0   = uint64_mul(_x,   UINT64_C(14029467366897019727) );
		const uint64_t tmp1   = uint64_rol(tmp0, 31);
		const uint64_t result = uint64_mul(tmp1, UINT64_C(11400714785074694791) );
		return result;
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline HandleHashMapAllocT<MaxHandlesT, KeyT>::HandleHashMapAllocT()
	{
		reset();
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline HandleHashMapAllocT<MaxHandlesT, KeyT>::~HandleHashMapAllocT()
	{
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline bgfx_handle HandleHashMapAllocT<MaxHandlesT, KeyT>::alloc(KeyT _key)
	{
		bgfx_handle handle = m_alloc.alloc();
		if (kInvalidHandle == handle)
		{
			return kInvalidHandle;
		}

		bool ok = m_table.insert(_key, handle);
		if (!ok)
		{
			m_alloc.free(handle);
			return kInvalidHandle;
		}

		return handle;
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline void HandleHashMapAllocT<MaxHandlesT, KeyT>::free(KeyT _key)
	{
		bgfx_handle handle = m_table.find(_key);
		if (kInvalidHandle == handle)
		{
			return;
		}

		m_table.removeByKey(_key);
		m_alloc.free(handle);
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline void HandleHashMapAllocT<MaxHandlesT, KeyT>::free(bgfx_handle _handle)
	{
		m_table.removeByHandle(_handle);
		m_alloc.free(_handle);
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline bgfx_handle HandleHashMapAllocT<MaxHandlesT, KeyT>::find(KeyT _key) const
	{
		return m_table.find(_key);
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline const bgfx_handle* HandleHashMapAllocT<MaxHandlesT, KeyT>::getHandles() const
	{
		return m_alloc.getHandles();
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline bgfx_handle HandleHashMapAllocT<MaxHandlesT, KeyT>::getHandleAt(bgfx_handle _at) const
	{
		return m_alloc.getHandleAt(_at);
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline bgfx_handle HandleHashMapAllocT<MaxHandlesT, KeyT>::getNumHandles() const
	{
		return m_alloc.getNumHandles();
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline bgfx_handle HandleHashMapAllocT<MaxHandlesT, KeyT>::getMaxHandles() const
	{
		return m_alloc.getMaxHandles();
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline bool HandleHashMapAllocT<MaxHandlesT, KeyT>::isValid(bgfx_handle _handle) const
	{
		return m_alloc.isValid(_handle);
	}

	template <bgfx_handle MaxHandlesT, typename KeyT>
	inline void HandleHashMapAllocT<MaxHandlesT, KeyT>::reset()
	{
		m_table.reset();
		m_alloc.reset();
	}

} // namespace bx
