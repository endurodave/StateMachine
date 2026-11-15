#include "Allocator.h"
#include <new>
#include <assert.h>

// The current new-handler. std::get_new_handler() is C++11, but GCC's library
// only has it from GCC 4.9; older toolchains (e.g. the Arduino Due's GCC 4.8)
// read it by setting and restoring it.
static std::new_handler CurrentNewHandler()
{
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 9))
    std::new_handler handler = std::set_new_handler(0);
    std::set_new_handler(handler);
    return handler;
#else
    return std::get_new_handler();
#endif
}

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
Allocator::Allocator(size_t size, uint32_t objects, char* memory, const char* name) :
    m_blockSize(BlockSize(size)),
    m_objectSize(size),
    m_maxObjects(objects),
    m_allocatorMode(HEAP_BLOCKS),
    m_pHead(NULL),
    m_pPool(NULL),
    m_poolIndex(0),
    m_blockCnt(0),
    m_blocksInUse(0),
    m_allocations(0),
    m_deallocations(0),
    m_name(name)
{
    // If using a fixed memory pool
	if (m_maxObjects)
	{
		// If caller provided an external memory pool
		if (memory)
		{
			m_pPool = memory;
			m_allocatorMode = STATIC_POOL;
		}
		else
		{
			m_pPool = (char*)new char[m_blockSize * m_maxObjects];
			m_allocatorMode = HEAP_POOL;
		}
	}
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
Allocator::~Allocator()
{
	// If using pool then destroy it, otherwise traverse free-list and
	// destroy each individual block
	if (m_allocatorMode == HEAP_POOL)
		delete [] m_pPool;
	else if (m_allocatorMode == HEAP_BLOCKS)
	{
		while(m_pHead)
			delete [] (char*)Pop();
	}
}

//------------------------------------------------------------------------------
// Allocate
//------------------------------------------------------------------------------
void* Allocator::Allocate(size_t size)
{
    assert(size <= m_objectSize);
    (void)size;

    // If can't obtain existing block then get a new one
    void* pBlock = Pop();
    if (!pBlock)
    {
        // If using a pool method then get block from pool,
        // otherwise using dynamic so get block from heap
        if (m_maxObjects)
        {
            // If we have not exceeded the pool maximum
            if(m_poolIndex < m_maxObjects)
            {
                pBlock = (void*)(m_pPool + (m_poolIndex++ * m_blockSize));
                m_blockCnt++;
            }
            else
            {
                // Pool exhausted. As operator new does, call the new-handler and
                // retry until a block is free; the handler must free a block,
                // throw or abort. With no handler, fail and return NULL.
                while (!pBlock)
                {
                    std::new_handler handler = CurrentNewHandler();
                    if (!handler)
                        break;
                    (*handler)();
                    pBlock = Pop();
                }
            }
        }
        else
        {
            // nothrow: Allocate() reports failure by returning NULL
            pBlock = (void*)new (std::nothrow) char[m_blockSize];
            if (pBlock)
                m_blockCnt++;
        }
    }

    // Count only blocks actually handed out
    if (pBlock)
    {
        m_blocksInUse++;
        m_allocations++;
    }

    return pBlock;
}

//------------------------------------------------------------------------------
// Deallocate
//------------------------------------------------------------------------------
void Allocator::Deallocate(void* pBlock)
{
    // delete of a null pointer may call operator delete(NULL)
    if (!pBlock)
        return;

    Push(pBlock);
	m_blocksInUse--;
	m_deallocations++;
}

//------------------------------------------------------------------------------
// AccountAlloc
//------------------------------------------------------------------------------
void Allocator::AccountAlloc(bool newBlock)
{
    if (newBlock)
        m_blockCnt++;
    m_blocksInUse++;
    m_allocations++;
}

//------------------------------------------------------------------------------
// Push
//------------------------------------------------------------------------------
void Allocator::Push(void* pMemory)
{
    Block* pBlock = (Block*)pMemory;
    pBlock->pNext = m_pHead;
    m_pHead = pBlock;
}

//------------------------------------------------------------------------------
// Pop
//------------------------------------------------------------------------------
void* Allocator::Pop()
{
    Block* pBlock = NULL;

    if (m_pHead)
    {
        pBlock = m_pHead;
        m_pHead = m_pHead->pNext;
    }

    return (void*)pBlock;
}
