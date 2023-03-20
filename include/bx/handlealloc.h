/*
 * Copyright 2010-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HANDLE_ALLOC_H_HEADER_GUARD
#define BX_HANDLE_ALLOC_H_HEADER_GUARD

#include "bx.h"
#include "allocator.h"
#include "uint32_t.h"

#ifndef bgfx_handle
#if BGFX_CONFIG_USE_32BIT_HANDLES
typedef uint32_t bgfx_handle;
#else
typedef uint16_t bgfx_handle;
#endif
#endif

namespace bx
{
#if BGFX_CONFIG_USE_32BIT_HANDLES
	constexpr bgfx_handle kInvalidHandle = UINT32_MAX;
#else
	constexpr bgfx_handle kInvalidHandle = UINT16_MAX;
#endif

	///
	class HandleAlloc
	{
	public:
		///
		HandleAlloc(bgfx_handle _maxHandles);

		///
		~HandleAlloc();

		///
		const bgfx_handle* getHandles() const;

		///
		bgfx_handle getHandleAt(bgfx_handle _at) const;

		///
		bgfx_handle getNumHandles() const;

		///
		bgfx_handle getMaxHandles() const;

		///
		bgfx_handle alloc();

		///
		bool isValid(bgfx_handle _handle) const;

		///
		void free(bgfx_handle _handle);

		///
		void reset();

	private:
		HandleAlloc();

		///
		bgfx_handle* getDensePtr() const;

		///
		bgfx_handle* getSparsePtr() const;

		bgfx_handle m_numHandles;
		bgfx_handle m_maxHandles;
	};

	///
	HandleAlloc* createHandleAlloc(AllocatorI* _allocator, bgfx_handle _maxHandles);

	///
	void destroyHandleAlloc(AllocatorI* _allocator, HandleAlloc* _handleAlloc);

	///
	template <bgfx_handle MaxHandlesT>
	class HandleAllocT : public HandleAlloc
	{
	public:
		///
		HandleAllocT();

		///
		~HandleAllocT();

	private:
		bgfx_handle m_padding[2*MaxHandlesT];
	};

	///
	template <bgfx_handle MaxHandlesT>
	class HandleListT
	{
	public:
		///
		HandleListT();

		///
		void pushBack(bgfx_handle _handle);

		///
		bgfx_handle popBack();

		///
		void pushFront(bgfx_handle _handle);

		///
		bgfx_handle popFront();

		///
		bgfx_handle getFront() const;

		///
		bgfx_handle getBack() const;

		///
		bgfx_handle getNext(bgfx_handle _handle) const;

		///
		bgfx_handle getPrev(bgfx_handle _handle) const;

		///
		void remove(bgfx_handle _handle);

		///
		void reset();

	private:
		///
		void insertBefore(bgfx_handle _before, bgfx_handle _handle);

		///
		void insertAfter(bgfx_handle _after, bgfx_handle _handle);

		///
		bool isValid(bgfx_handle _handle) const;

		///
		void updateFrontBack(bgfx_handle _handle);

		bgfx_handle m_front;
		bgfx_handle m_back;

		struct Link
		{
			bgfx_handle m_prev;
			bgfx_handle m_next;
		};

		Link m_links[MaxHandlesT];
	};

	///
	template <bgfx_handle MaxHandlesT>
	class HandleAllocLruT
	{
	public:
		///
		HandleAllocLruT();

		///
		~HandleAllocLruT();

		///
		const bgfx_handle* getHandles() const;

		///
		bgfx_handle getHandleAt(bgfx_handle _at) const;

		///
		bgfx_handle getNumHandles() const;

		///
		bgfx_handle getMaxHandles() const;

		///
		bgfx_handle alloc();

		///
		bool isValid(bgfx_handle _handle) const;

		///
		void free(bgfx_handle _handle);

		///
		void touch(bgfx_handle _handle);

		///
		bgfx_handle getFront() const;

		///
		bgfx_handle getBack() const;

		///
		bgfx_handle getNext(bgfx_handle _handle) const;

		///
		bgfx_handle getPrev(bgfx_handle _handle) const;

		///
		void reset();

	private:
		HandleListT<MaxHandlesT>  m_list;
		HandleAllocT<MaxHandlesT> m_alloc;
	};

	///
	template <uint32_t MaxCapacityT, typename KeyT = bgfx_handle>
	class HandleHashMapT
	{
	public:
		///
		HandleHashMapT();

		///
		~HandleHashMapT();

		///
		bool insert(KeyT _key, bgfx_handle _handle);

		///
		bool removeByKey(KeyT _key);

		///
		bool removeByHandle(bgfx_handle _handle);

		///
		bgfx_handle find(KeyT _key) const;

		///
		void reset();

		///
		uint32_t getNumElements() const;

		///
		uint32_t getMaxCapacity() const;

		///
		struct Iterator
		{
			bgfx_handle handle;

		private:
			friend class HandleHashMapT<MaxCapacityT, KeyT>;
			uint32_t pos;
			uint32_t num;
		};

		///
		Iterator first() const;

		///
		bool next(Iterator& _it) const;

	private:
		///
		uint32_t findIndex(KeyT _key) const;

		///
		void removeIndex(uint32_t _idx);

		///
		uint32_t mix(uint32_t _x) const;

		///
		uint64_t mix(uint64_t _x) const;

		uint32_t m_maxCapacity;
		uint32_t m_numElements;

		KeyT     m_key[MaxCapacityT];
		bgfx_handle m_handle[MaxCapacityT];
	};

	///
	template <bgfx_handle MaxHandlesT, typename KeyT = uint32_t>
	class HandleHashMapAllocT
	{
	public:
		///
		HandleHashMapAllocT();

		///
		~HandleHashMapAllocT();

		///
		bgfx_handle alloc(KeyT _key);

		///
		void free(KeyT _key);

		///
		void free(bgfx_handle _handle);

		///
		bgfx_handle find(KeyT _key) const;

		///
		const bgfx_handle* getHandles() const;

		///
		bgfx_handle getHandleAt(bgfx_handle _at) const;

		///
		bgfx_handle getNumHandles() const;

		///
		bgfx_handle getMaxHandles() const;

		///
		bool isValid(bgfx_handle _handle) const;

		///
		void reset();

	private:
		HandleHashMapT<MaxHandlesT+MaxHandlesT/2, KeyT> m_table;
		HandleAllocT<MaxHandlesT> m_alloc;
	};

} // namespace bx

#include "inline/handlealloc.inl"

#endif // BX_HANDLE_ALLOC_H_HEADER_GUARD
