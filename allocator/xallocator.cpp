#include "Allocator.h"
#include "xallocator.h"
#include <cstring>
#include <cstddef>
#include <cstdio>
#include <new>

using namespace std;

#ifndef CHAR_BIT
#define CHAR_BIT	8
#endif

// Bytes before the caller's memory in each block, holding the block's Allocator*.
// Sized so the caller's memory keeps the block's alignment (see xallocator.h).
static const size_t BLOCK_HEADER_SIZE = XALLOC_BLOCK_HEADER_SIZE;
static_assert(BLOCK_HEADER_SIZE >= sizeof(Allocator*), "header must hold an Allocator*");

// Define STATIC_POOLS to switch from heap blocks mode to static pools mode
//#define STATIC_POOLS
#ifdef STATIC_POOLS
	// Update this section as necessary if you want to use static memory pools.
	// See also xalloc_init() and xalloc_destroy() for additional updates required.
	// Block sizes are powers of two, so every block in a pool keeps the pool's
	// alignment. Blocks include the BLOCK_HEADER_SIZE header, so the largest
	// xmalloc() size is 4096 - BLOCK_HEADER_SIZE.
	#define MAX_ALLOCATORS	10
	#define MAX_BLOCKS		32

	// Create static storage for each static allocator instance
	alignas(AllocatorPool<char[8], MAX_BLOCKS>) static char _allocator8 [sizeof(AllocatorPool<char[8], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[16], MAX_BLOCKS>) static char _allocator16 [sizeof(AllocatorPool<char[16], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[32], MAX_BLOCKS>) static char _allocator32 [sizeof(AllocatorPool<char[32], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[64], MAX_BLOCKS>) static char _allocator64 [sizeof(AllocatorPool<char[64], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[128], MAX_BLOCKS>) static char _allocator128 [sizeof(AllocatorPool<char[128], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[256], MAX_BLOCKS>) static char _allocator256 [sizeof(AllocatorPool<char[256], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[512], MAX_BLOCKS>) static char _allocator512 [sizeof(AllocatorPool<char[512], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[1024], MAX_BLOCKS>) static char _allocator1024 [sizeof(AllocatorPool<char[1024], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[2048], MAX_BLOCKS>) static char _allocator2048 [sizeof(AllocatorPool<char[2048], MAX_BLOCKS>)];
	alignas(AllocatorPool<char[4096], MAX_BLOCKS>) static char _allocator4096 [sizeof(AllocatorPool<char[4096], MAX_BLOCKS>)];

	// Array of pointers to all allocator instances
	static Allocator* _allocators[MAX_ALLOCATORS];

#else
	#define MAX_ALLOCATORS  15
	static Allocator* _allocators[MAX_ALLOCATORS];
#endif	// STATIC_POOLS

// For C++ applications, must define AUTOMATIC_XALLOCATOR_INIT_DESTROY to
// correctly ensure allocators are initialized before any static user C++
// construtor/destructor executes which might call into the xallocator API.
// This feature costs 1-byte of RAM per C++ translation unit. This feature
// can be disabled only under the following circumstances:
//
// 1) The xallocator is only used within C files.
// 2) STATIC_POOLS is undefined and the application never exits main (e.g.
// an embedded system).
//
// In either of the two cases above, call xalloc_init() in main at startup,
// and xalloc_destroy() before main exits. In all other situations
// XallocInitDestroy must be used to call xalloc_init() and xalloc_destroy().
#ifdef AUTOMATIC_XALLOCATOR_INIT_DESTROY
XallocRefCount XallocInitDestroy::refCount(0);
XallocInitDestroy::XallocInitDestroy()
{
	// Track how many static instances of XallocInitDestroy are created
	if (refCount++ == 0)
		xalloc_init();
}

XallocInitDestroy::~XallocInitDestroy()
{
	// Last static instance to have destructor called?
	if (--refCount == 0)
		xalloc_destroy();
}
#endif	// AUTOMATIC_XALLOCATOR_INIT_DESTROY

/// Returns the next higher powers of two. For instance, pass in 12 and
/// the value returned would be 16.
/// @param[in] k - numeric value to compute the next higher power of two.
/// @return	The next higher power of two based on the input k.
template <class T>
T nexthigher(T k)
{
    k--;
    for (size_t i=1; i<sizeof(T)*CHAR_BIT; i<<=1)
        k |= (k >> i);
    return k+1;
}

// Lock guarding the allocator table and each Allocator. It is held only for a
// table search and a free-list push or pop (heap calls are made outside it), so
// on an RTOS a short critical section fits better than a mutex: no kernel
// object, no priority inversion, and it works before the scheduler starts.
// Selected at compile time; define at most one option for the whole build:
//
//   (default)                std::mutex, when the C++ library provides it
//                            (Windows, Linux, macOS, ESP32); otherwise none.
//   XALLOC_LOCK_FREERTOS     FreeRTOS critical section (taskENTER_CRITICAL, or
//                            the _FROM_ISR form inside an interrupt on Cortex-M;
//                            portENTER_CRITICAL_SAFE spinlock on ESP32).
//   XALLOC_LOCK_THREADX      ThreadX tx_interrupt_control() interrupt masking.
//   XALLOC_LOCK_ZEPHYR       Zephyr irq_lock()/irq_unlock().
//   XALLOC_LOCK_CMSIS_RTOS2  Cortex-M PRIMASK interrupt masking (CMSIS-Core),
//                            for any CMSIS-RTOS2 kernel (RTX5, FreeRTOS, ...).
//   XALLOC_LOCK_USER         you define extern "C" void xalloc_lock() and
//                            void xalloc_unlock(). They may be called before
//                            main() and before the RTOS starts, so they must
//                            work then too.
//   XALLOC_LOCK_NONE         no lock: single-threaded use only.
//
// DelegateMQ's DMQ_THREAD_FREERTOS, _THREADX, _ZEPHYR and _CMSIS_RTOS2 select
// the matching option. Bare-metal toolchains without threads support (e.g.
// arm-none-eabi as used by STM32duino and Arduino-Pico) have no std::mutex, so
// there the default is XALLOC_LOCK_NONE: pick an RTOS option if xallocator is
// called from more than one task.
//
// Calling xmalloc()/xfree() from an interrupt: only with STATIC_POOLS (heap
// blocks mode calls operator new) and an interrupt-masking option (FreeRTOS on
// Cortex-M or ESP32, ThreadX, Zephyr, CMSIS-RTOS2).
#if !defined(XALLOC_LOCK_FREERTOS) && !defined(XALLOC_LOCK_THREADX) && \
	!defined(XALLOC_LOCK_ZEPHYR) && !defined(XALLOC_LOCK_CMSIS_RTOS2) && \
	!defined(XALLOC_LOCK_USER) && !defined(XALLOC_LOCK_NONE)
	#if defined(DMQ_THREAD_FREERTOS)
		#define XALLOC_LOCK_FREERTOS
	#elif defined(DMQ_THREAD_THREADX)
		#define XALLOC_LOCK_THREADX
	#elif defined(DMQ_THREAD_ZEPHYR)
		#define XALLOC_LOCK_ZEPHYR
	#elif defined(DMQ_THREAD_CMSIS_RTOS2)
		#define XALLOC_LOCK_CMSIS_RTOS2
	#elif defined(__GLIBCXX__) && !defined(_GLIBCXX_HAS_GTHREADS)
		#define XALLOC_LOCK_NONE
	#endif
#endif

#if (defined(XALLOC_LOCK_FREERTOS) + defined(XALLOC_LOCK_THREADX) + \
	defined(XALLOC_LOCK_ZEPHYR) + defined(XALLOC_LOCK_CMSIS_RTOS2) + \
	defined(XALLOC_LOCK_USER) + defined(XALLOC_LOCK_NONE)) > 1
	#error "Define only one XALLOC_LOCK_ option"
#endif

/// State saved by xlock() and restored by xunlock(), e.g. the interrupt mask.
/// Kept on the caller's stack, so nested and interrupt-context locks each
/// restore their own state.
struct XLockState
{
	unsigned long saved;
	int mode;
};

#if defined(XALLOC_LOCK_FREERTOS)
	#if defined(ESP_PLATFORM)
		#include "freertos/FreeRTOS.h"
		#include "freertos/task.h"
		// ESP32 cores share one spinlock; the _SAFE forms work in tasks and ISRs
		static portMUX_TYPE _xallocMux = portMUX_INITIALIZER_UNLOCKED;
		static inline XLockState xlock()
		{
			portENTER_CRITICAL_SAFE(&_xallocMux);
			XLockState state = { 0, 0 };
			return state;
		}
		static inline void xunlock(XLockState) { portEXIT_CRITICAL_SAFE(&_xallocMux); }
	#else
		#include "FreeRTOS.h"
		#include "task.h"
		enum { XLOCK_NONE, XLOCK_TASK, XLOCK_ISR };
		static inline XLockState xlock()
		{
			XLockState state = { 0, XLOCK_NONE };
		#if (INCLUDE_xTaskGetSchedulerState == 1) || (configUSE_TIMERS == 1)
			// Before the scheduler starts there is one thread of execution.
			// A FreeRTOS critical section here would leave interrupts disabled
			// until the scheduler starts, stalling e.g. Arduino delay() and Serial.
			if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
				return state;
		#endif
		#if defined(__arm__) || defined(__ARM_ARCH)
			if (xPortIsInsideInterrupt() != pdFALSE)
			{
				state.saved = (unsigned long)taskENTER_CRITICAL_FROM_ISR();
				state.mode = XLOCK_ISR;
				return state;
			}
		#endif
			taskENTER_CRITICAL();
			state.mode = XLOCK_TASK;
			return state;
		}
		static inline void xunlock(XLockState state)
		{
			if (state.mode == XLOCK_ISR)
				taskEXIT_CRITICAL_FROM_ISR((UBaseType_t)state.saved);
			else if (state.mode == XLOCK_TASK)
				taskEXIT_CRITICAL();
		}
	#endif
#elif defined(XALLOC_LOCK_THREADX)
	#include "tx_api.h"
	static inline XLockState xlock()
	{
		XLockState state = { (unsigned long)tx_interrupt_control(TX_INT_DISABLE), 0 };
		return state;
	}
	static inline void xunlock(XLockState state) { tx_interrupt_control((UINT)state.saved); }
#elif defined(XALLOC_LOCK_ZEPHYR)
	#include <zephyr/kernel.h>
	static inline XLockState xlock()
	{
		XLockState state = { (unsigned long)irq_lock(), 0 };
		return state;
	}
	static inline void xunlock(XLockState state) { irq_unlock((unsigned int)state.saved); }
#elif defined(XALLOC_LOCK_CMSIS_RTOS2)
	#include "cmsis_compiler.h"
	static inline XLockState xlock()
	{
		XLockState state = { (unsigned long)__get_PRIMASK(), 0 };
		__disable_irq();
		return state;
	}
	static inline void xunlock(XLockState state) { __set_PRIMASK((uint32_t)state.saved); }
#elif defined(XALLOC_LOCK_USER)
	extern "C" void xalloc_lock();
	extern "C" void xalloc_unlock();
	static inline XLockState xlock()
	{
		xalloc_lock();
		XLockState state = { 0, 0 };
		return state;
	}
	static inline void xunlock(XLockState) { xalloc_unlock(); }
#elif defined(XALLOC_LOCK_NONE)
	static inline XLockState xlock()
	{
		XLockState state = { 0, 0 };
		return state;
	}
	static inline void xunlock(XLockState) {}
#else
	#include <mutex>
	static std::mutex& get_mutex()
	{
		// Intentionally never destroyed. A function-local static mutex would be
		// destroyed during static destruction, and a static object destroyed after
		// it that calls xfree() or xalloc_destroy() would lock a destroyed mutex.
		// The OS reclaims the memory at process exit.
		static std::mutex* _mutex = new std::mutex();
		return *_mutex;
	}
	static inline XLockState xlock()
	{
		get_mutex().lock();
		XLockState state = { 0, 0 };
		return state;
	}
	static inline void xunlock(XLockState) { get_mutex().unlock(); }
#endif

/// Stored a pointer to the allocator instance within the block region.
///	a pointer to the client's area within the block.
/// @param[in] block - a pointer to the raw memory block.
///	@param[in] allocator - the allocator that owns the block.
/// @return	A pointer to the client's address within the raw memory block.
static inline void *set_block_allocator(void* block, Allocator* allocator)
{
	*static_cast<Allocator**>(block) = allocator;

	// The client's memory starts after the header
	return static_cast<char*>(block) + BLOCK_HEADER_SIZE;
}

/// Gets the allocator instance stored within the block.
/// @param[in] block - a pointer to the client's memory block.
/// @return	The original allocator instance stored in the memory block.
static inline Allocator* get_block_allocator(void* block)
{
	return *reinterpret_cast<Allocator**>(static_cast<char*>(block) - BLOCK_HEADER_SIZE);
}

/// Returns the raw memory block pointer given a client memory pointer.
/// @param[in] block - a pointer to the client memory block.
/// @return	A pointer to the original raw memory block address.
static inline void *get_block_ptr(void* block)
{
	return static_cast<char*>(block) - BLOCK_HEADER_SIZE;
}

/// Returns an allocator instance matching the size provided. Caller holds the lock.
/// @param[in] size - allocator block size
/// @return Allocator instance handling requested block size or NULL
/// if no allocator exists.
static inline Allocator* find_allocator(size_t size)
{
	for (int i=0; i<MAX_ALLOCATORS; i++)
	{
		if (_allocators[i] == 0)
			break;

		if (_allocators[i]->GetBlockSize() == size)
			return _allocators[i];
	}

	return NULL;
}

#ifndef STATIC_POOLS
/// Insert an allocator instance into the array. Caller holds the lock.
/// @param[in] allocator - An allocator instance
/// @return true if inserted, false if the array is full.
static inline bool insert_allocator(Allocator* allocator)
{
	for (int i=0; i<MAX_ALLOCATORS; i++)
	{
		if (_allocators[i] == 0)
		{
			_allocators[i] = allocator;
			return true;
		}
	}

	return false;
}
#endif	// STATIC_POOLS

/// This function must be called exactly one time *before* any other xallocator
/// API is called. XallocInitDestroy constructor calls this function automatically.
extern "C" void xalloc_init()
{
#ifdef STATIC_POOLS
	XLockState lock = xlock();

	// For STATIC_POOLS mode, the allocators must be initialized before any other
	// static user class constructor is run. Therefore, use placement new to initialize
	// each allocator into the previously reserved static memory locations.
	_allocators[0] = new (&_allocator8) AllocatorPool<char[8], MAX_BLOCKS>();
	_allocators[1] = new (&_allocator16) AllocatorPool<char[16], MAX_BLOCKS>();
	_allocators[2] = new (&_allocator32) AllocatorPool<char[32], MAX_BLOCKS>();
	_allocators[3] = new (&_allocator64) AllocatorPool<char[64], MAX_BLOCKS>();
	_allocators[4] = new (&_allocator128) AllocatorPool<char[128], MAX_BLOCKS>();
	_allocators[5] = new (&_allocator256) AllocatorPool<char[256], MAX_BLOCKS>();
	_allocators[6] = new (&_allocator512) AllocatorPool<char[512], MAX_BLOCKS>();
	_allocators[7] = new (&_allocator1024) AllocatorPool<char[1024], MAX_BLOCKS>();
	_allocators[8] = new (&_allocator2048) AllocatorPool<char[2048], MAX_BLOCKS>();
	_allocators[9] = new (&_allocator4096) AllocatorPool<char[4096], MAX_BLOCKS>();

	xunlock(lock);
#endif
}

/// Called one time when the application exits to cleanup any allocated memory.
/// ~XallocInitDestroy destructor calls this function automatically.
extern "C" void xalloc_destroy()
{
	XLockState lock = xlock();

#ifdef STATIC_POOLS
	for (int i=0; i<MAX_ALLOCATORS; i++)
	{
		if (_allocators[i])
			_allocators[i]->~Allocator();
		_allocators[i] = 0;
	}
#else
	for (int i=0; i<MAX_ALLOCATORS; i++)
	{
		if (_allocators[i] == 0)
			break;
		delete _allocators[i];
		_allocators[i] = 0;
	}
#endif

	xunlock(lock);
}

/// Get an Allocator instance based upon the client's requested block size.
/// If a Allocator instance is not currently available to handle the size,
///	then a new Allocator instance is create.
///	@param[in] size - the client's requested block size.
///	@return An Allocator instance that handles blocks of the requested
///	size, or NULL if none can (STATIC_POOLS: no pool that large; otherwise
///	the allocator table is full or out of memory).
Allocator* xallocator_get_allocator(size_t size)
{
	// Too large to round up to a power of two without overflowing size_t
	const size_t maxBlockSize = (static_cast<size_t>(-1) >> 1) + 1;
	if (size > maxBlockSize - BLOCK_HEADER_SIZE)
		return NULL;

	// Add the header to the requested size and round up to the next power of
	// two, so every block keeps the alignment of the memory it came from.
	size_t blockSize = nexthigher<size_t>(size + BLOCK_HEADER_SIZE);

	XLockState lock = xlock();
	Allocator* allocator = find_allocator(blockSize);
	xunlock(lock);

#ifndef STATIC_POOLS
	// If there is not an allocator already created to handle this block size
	if (allocator == NULL)
	{
		// Created outside the lock: heap allocation under it risks deadlock
		// with the C++ library's own heap locks.
		Allocator* created = new (std::nothrow) Allocator(blockSize, 0, 0, "xallocator");
		if (created == NULL)
			return NULL;

		// Re-check and insert under one lock, so two threads can't both add
		// an allocator for the same size
		XLockState lock2 = xlock();
		allocator = find_allocator(blockSize);
		if (allocator == NULL && insert_allocator(created))
			allocator = created;
		xunlock(lock2);

		// Another thread inserted one first, or the table is full
		if (allocator != created)
			delete created;
	}
#endif

	return allocator;
}

/// Allocates a memory block of the requested size. The blocks are created from
///	the fixed block allocators.
///	@param[in] size - the client requested size of the block.
/// @return	A pointer to the client's memory block, or NULL if none is available.
extern "C" void *xmalloc(size_t size)
{
	Allocator* allocator = xallocator_get_allocator(size);
	if (allocator == NULL)
		return NULL;

	void* blockMemoryPtr = NULL;

#ifdef STATIC_POOLS
	// The pool hands out its slots and tracks its stats; NULL when exhausted
	XLockState lock = xlock();
	blockMemoryPtr = allocator->Allocate(allocator->GetBlockSize());
	xunlock(lock);
#else
	// Reuse a freed block if there is one
	XLockState lock = xlock();
	blockMemoryPtr = allocator->Pop();
	if (blockMemoryPtr != NULL)
		allocator->AccountAlloc(false);
	xunlock(lock);

	if (blockMemoryPtr == NULL)
	{
		// Get a new block from the heap outside the lock (see above). nothrow:
		// an exception must not leave this extern "C" function.
		blockMemoryPtr = new (std::nothrow) char[allocator->GetBlockSize()];
		if (blockMemoryPtr != NULL)
		{
			XLockState lock2 = xlock();
			allocator->AccountAlloc(true);
			xunlock(lock2);
		}
	}
#endif

	if (blockMemoryPtr == NULL)
		return NULL;

	// Set the block Allocator* within the raw memory block region
	return set_block_allocator(blockMemoryPtr, allocator);
}

/// Frees a memory block previously allocated with xalloc. The blocks are returned
///	to the fixed block allocator that originally created it.
///	@param[in] ptr - a pointer to a block created with xalloc.
extern "C" void xfree(void* ptr)
{
	if (ptr == 0)
		return;

	// Extract the original allocator instance from the caller's block pointer
	Allocator* allocator = get_block_allocator(ptr);

	// Convert the client pointer into the original raw block pointer
	void* blockPtr = get_block_ptr(ptr);

	XLockState lock = xlock();

	// Deallocate the block
	allocator->Deallocate(blockPtr);

	xunlock(lock);
}

/// Reallocates a memory block previously allocated with xalloc.
///	@param[in] ptr - a pointer to a block created with xalloc.
///	@param[in] size - the client requested block size to create.
extern "C" void *xrealloc(void *oldMem, size_t size)
{
	if (oldMem == 0)
		return xmalloc(size);

	if (size == 0)
	{
		xfree(oldMem);
		return 0;
	}
	else
	{
		// Create a new memory block
		void* newMem = xmalloc(size);
		if (newMem != 0)
		{
			// Get the original allocator instance from the old memory block
			Allocator* oldAllocator = get_block_allocator(oldMem);
			size_t oldSize = oldAllocator->GetBlockSize() - BLOCK_HEADER_SIZE;

			// Copy the bytes from the old memory block into the new (as much as will fit)
			memcpy(newMem, oldMem, (oldSize < size) ? oldSize : size);

			// Free the old memory block
			xfree(oldMem);

			// Return the client pointer to the new memory block
			return newMem;
		}
		return 0;
	}
}

/// Output xallocator usage statistics
extern "C" void xalloc_stats()
{
	XLockState lock = xlock();

	for (int i=0; i<MAX_ALLOCATORS; i++)
	{
		if (_allocators[i] == 0)
			break;

		const char* name = _allocators[i]->GetName();
		printf("%s Block Size: %lu Block Count: %lu Blocks In Use: %lu\n",
			name != NULL ? name : "",
			(unsigned long)_allocators[i]->GetBlockSize(),
			(unsigned long)_allocators[i]->GetBlockCount(),
			(unsigned long)_allocators[i]->GetBlocksInUse());
	}

	xunlock(lock);
}
