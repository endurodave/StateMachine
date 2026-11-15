#ifndef _XALLOCATOR_H
#define _XALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

// @see https://github.com/endurodave/xallocator
// David Lafreniere

// Each xmalloc block starts with a header holding its Allocator*. The header is
// sized so the caller's memory after it keeps the alignment of the underlying
// block (16 bytes on 64-bit, 8 on 32-bit), enough for long double and SSE types.
#define XALLOC_BLOCK_HEADER_SIZE (sizeof(void*) > 4 ? 16 : 8)

#ifdef __cplusplus
#include <new>
#include <cstdlib>
#include <atomic>

// Define AUTOMATIC_XALLOCATOR_INIT_DESTROY to automatically call xalloc_init() and
// xalloc_destroy() when using xallocator in C++ projects. On embedded systems that
// never exit, you can save 1-byte of RAM storage per translation unit by undefining
// AUTOMATIC_XALLOCATOR_INIT_DESTROY and calling xalloc_init() manually before the OS
// starts.
#define AUTOMATIC_XALLOCATOR_INIT_DESTROY
#ifdef AUTOMATIC_XALLOCATOR_INIT_DESTROY
/// If a C++ translation unit, create a static instance of XallocInitDestroy. Any C++
/// file including xallocator.h will have the xallocDestroy instance declared first
/// within the translation unit and thus will be constructed first. Destruction
/// will occur in the reverse order so xallocInitDestroy is called last. This way,
/// any static user objects relying on xallocator will be destroyed first before
/// xalloc_destroy() is called.
/// Embedded systems that never exit can remove the XallocInitDestroy class entirely.
// Static construction and destruction run on one thread, so the count only
// needs atomics where they are cheap: ARMv6-M (Cortex-M0/M0+, e.g. Arduino
// Zero) has no atomic instructions and its cores don't provide the library
// calls std::atomic falls back to.
#if defined(__ARM_ARCH_6M__)
typedef int32_t XallocRefCount;
#else
typedef std::atomic<int32_t> XallocRefCount;
#endif

class XallocInitDestroy
{
public:
	XallocInitDestroy();
	~XallocInitDestroy();
private:
	static XallocRefCount refCount;
};
static XallocInitDestroy xallocInitDestroy;
#endif	// AUTOMATIC_XALLOCATOR_INIT_DESTROY

/// Called by the XALLOCATOR operator new overloads when xmalloc() fails:
/// throws std::bad_alloc, or aborts when exceptions are disabled. Defined
/// outside extern "C" so it may throw.
inline void xalloc_bad_alloc()
{
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
	throw std::bad_alloc();
#else
	std::abort();
#endif
}
#endif	// __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

/// This function must be called exactly one time before the operating system
/// threading starts. If using xallocator exclusively in C files within your application
/// code, you must call this function before the OS starts. If using C++, client code
/// does not call xalloc_init. Let XallocInitDestroy() call xalloc_init automatically.
/// Embedded systems that never exit can call xalloc_init() manually at startup
/// and eliminate XallocInitDestroy usage. When the system is still single threaded at
/// startup, the xallocator API does not need mutex protection.
void xalloc_init();

/// This function must be called once when the application exits.  Never call xalloc_destroy()
/// manually except if using xallocator in a C-only application. If using xallocator
/// exclusively in C files within your application code, you must call this function before
/// the program exits. If using C++, ~XallocInitDestroy() must call xalloc_destroy automatically.
/// Embedded systems that never exit need not call this function at all.
void xalloc_destroy();

/// Allocate a block of memory
/// @param[in] size - the size of the block to allocate.
/// @return A pointer to the block, or NULL if no memory is available.
void *xmalloc(size_t size);

/// Frees a previously xalloc allocated block
/// @param[in] ptr - a pointer to a previously allocated memory using xalloc.
void xfree(void* ptr);

/// Reallocates an existing xalloc block to a new size
/// @param[in] ptr - a pointer to a previously allocated memory using xalloc.
/// @param[in] size - the size of the new block
void *xrealloc(void *ptr, size_t size);

/// Output allocator statistics to the standard output
void xalloc_stats();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class Allocator;

/// Get the Allocator instance that handles blocks for the requested size.
/// @param[in] size - the client's requested block size.
/// @return An Allocator instance that handles blocks of the requested size.
Allocator* xallocator_get_allocator(size_t size);

// Macro to overload new/delete with xalloc/xfree. Add macro to any class to enable
// fixed-block memory allocation. Add to a base class provides fixed-block memory
// for the base and all derived classes. Like the global operator new, the throwing
// forms call xalloc_bad_alloc() if no memory is available; the nothrow form
// returns NULL.
#define XALLOCATOR \
    public: \
        void* operator new(size_t size) { \
            void* p = xmalloc(size); \
            if (!p) xalloc_bad_alloc(); \
            return p; \
        } \
        void* operator new(size_t /*size*/, void* mem) { \
            return mem; \
        } \
        void* operator new(size_t size, const std::nothrow_t& /*nt*/) noexcept { \
            return xmalloc(size); \
        } \
        void* operator new[](size_t size) { \
            void* p = xmalloc(size); \
            if (!p) xalloc_bad_alloc(); \
            return p; \
        } \
        void operator delete(void* pObject) { \
            xfree(pObject); \
        } \
        void operator delete(void* pObject, const std::nothrow_t& /*nt*/) { \
            xfree(pObject); \
        } \
        void operator delete[](void* pData) { \
            xfree(pData); \
        }
#endif	// __cplusplus

#endif
