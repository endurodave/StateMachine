#include "Fault.h"
#include "DataTypes.h"
#include <assert.h>
#include <stdlib.h>

// The default FaultHandler. On GCC and Clang it is a weak symbol: define your
// own FaultHandler() (e.g. to log and reset) and it replaces this one. That
// also lets StateMachine link with other libraries that provide a
// FaultHandler (C_Allocator, SimpleSocketProtocol).
#if defined(__GNUC__) || defined(__clang__)
	#define FAULT_WEAK __attribute__((weak))
#else
	#define FAULT_WEAK
#endif

//----------------------------------------------------------------------------
// FaultHandler
//----------------------------------------------------------------------------
FAULT_WEAK void FaultHandler(const char* file, unsigned short line)
{
	(void)file;
	(void)line;

	// If you hit this line, it means one of the ASSERT macros failed.
#if WIN32
	// Without a debugger attached, DebugBreak() would just crash the program
	if (IsDebuggerPresent())
		DebugBreak();
#endif

	assert(0);

	// assert() does nothing with NDEBUG (e.g. the STM32 Arduino core defines
	// it): a fault must not return to the code that detected it
	abort();
}
