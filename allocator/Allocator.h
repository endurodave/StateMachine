#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <cstddef>
#include <cstdlib>
#include <new>

/// @see https://github.com/endurodave/Allocator
/// David Lafreniere

/// Called when a fixed block allocation fails: throws std::bad_alloc, or, when
/// exceptions are disabled, asserts and aborts. Used by the operator new
/// overloads (DECLARE_ALLOCATOR, XALLOCATOR) since operator new must not
/// return NULL.
inline void AllocatorBadAlloc()
{
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
    throw std::bad_alloc();
#else
    std::abort();
#endif
}

/// @brief A fixed block memory allocator. Not thread-safe: callers sharing an
/// instance between threads must lock around it (xallocator does).
class Allocator
{
public:
    /// Constructor
    /// @param[in]  size - size of the fixed blocks
    /// @param[in]  objects - maximum number of object. If 0, new blocks are
	///		created off the heap as necessary.
	/// @param[in]	memory - pointer to a block of static memory for allocator or NULL
	///		to obtain memory from global heap. If not NULL, the objects argument
	///		defines the size of the memory block: BlockSize(size) x objects bytes,
	///		aligned for the objects it holds. AllocatorPool does both.
	///	@param[in]	name - optional allocator name string.
    Allocator(size_t size, uint32_t objects=0, char* memory = NULL, const char* name=NULL);

    /// Destructor
    ~Allocator();

    /// The block size used for objects of 'size' bytes: room for the free-list
    /// pointer that a free block holds, rounded up to the pointer's alignment.
    static constexpr size_t BlockSize(size_t size)
    {
        return ((size < sizeof(long*) ? sizeof(long*) : size) + alignof(long*) - 1)
            / alignof(long*) * alignof(long*);
    }

    /// Get a pointer to a memory block.
    /// @details When a pool is exhausted, the std::new_handler (if any) is called
    /// and the allocation retried, as operator new does; the handler must free a
    /// block, throw or abort. With no new_handler, NULL is returned.
    /// @param[in]  size - size of the block to allocate
    /// @return     Returns pointer to the block. Otherwise NULL if unsuccessful.
    void* Allocate(size_t size);

    /// Return a pointer to the memory pool. NULL is ignored.
    /// @param[in]  pBlock - block of memory deallocate (i.e push onto free-list)
    void Deallocate(void* pBlock);

    /// Update allocation stats when a caller obtains a block itself with Pop()
    /// or from the heap instead of Allocate() (xallocator does this so it can
    /// allocate from the heap without holding its lock).
    /// @param[in] newBlock - true if a fresh block was obtained from the heap.
    void AccountAlloc(bool newBlock);

    /// Push a memory block onto head of free-list.
    /// @param[in]  pMemory - block of memory to push onto free-list
    void Push(void* pMemory);

    /// Pop a memory block from head of free-list.
    /// @return     Returns pointer to the block. Otherwise NULL if unsuccessful.
    void* Pop();

    /// Get the allocator name string.
    /// @return		A pointer to the allocator name or NULL if none was assigned.
    const char* GetName() { return m_name; }

    /// Gets the fixed block memory size, in bytes, handled by the allocator.
    /// @return		The fixed block size in bytes.
    size_t GetBlockSize() const { return m_blockSize; }

    /// Gets the maximum number of blocks created by the allocator.
    /// @return		The number of fixed memory blocks created.
    uint32_t GetBlockCount() { return m_blockCnt; }

    /// Gets the number of blocks in use.
    /// @return		The number of blocks in use by the application.
    uint32_t GetBlocksInUse() { return m_blocksInUse; }

    /// Gets the total number of allocations for this allocator instance.
    /// @return		The total number of allocations.
    uint32_t GetAllocations() { return m_allocations; }

    /// Gets the total number of deallocations for this allocator instance.
    /// @return		The total number of deallocations.
    uint32_t GetDeallocations() { return m_deallocations; }

private:
    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

    struct Block
    {
        Block* pNext;
    };

	enum AllocatorMode { HEAP_BLOCKS, HEAP_POOL, STATIC_POOL };

    const size_t m_blockSize;
    const size_t m_objectSize;
    const uint32_t m_maxObjects;
	AllocatorMode m_allocatorMode;
    Block* m_pHead;
    char* m_pPool;
    uint32_t m_poolIndex;
    uint32_t m_blockCnt;
    uint32_t m_blocksInUse;
    uint32_t m_allocations;
    uint32_t m_deallocations;
    const char* m_name;
};

// Template class to create external memory pool
template <class T, uint32_t Objects>
class AllocatorPool : public Allocator
{
public:
	AllocatorPool() : Allocator(sizeof(T), Objects, m_memory)
	{
	}
private:
	// One block per object, sized like the allocator's blocks (at least a
	// pointer, pointer-aligned), and aligned for both T and the free-list
	// pointer. Blocks are a multiple of both alignments, so every block is too.
	alignas(alignof(T) > alignof(long*) ? alignof(T) : alignof(long*))
		char m_memory[Allocator::BlockSize(sizeof(T)) * Objects];
};

// macro to provide header file interface
#define DECLARE_ALLOCATOR \
    public: \
        void* operator new(size_t size) { \
            void* p = _allocator.Allocate(size); \
            if (!p) AllocatorBadAlloc(); \
            return p; \
        } \
        void operator delete(void* pObject) { \
            _allocator.Deallocate(pObject); \
        } \
    private: \
        static Allocator _allocator;

// macro to provide source file interface
#define IMPLEMENT_ALLOCATOR(class, objects, memory) \
	Allocator class::_allocator(sizeof(class), objects, memory, #class);

#endif
