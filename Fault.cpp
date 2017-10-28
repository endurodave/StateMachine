#include "Fault.h"
#include "DataTypes.h"

#if WIN32
#include <assert.h>
#endif

//----------------------------------------------------------------------------
// FaultHandler
//----------------------------------------------------------------------------
void FaultHandler(const char* file, unsigned short line)
{
	// If you hit this line, it means one of the ASSERT macros failed.
#if WIN32
    DebugBreak();	

	assert(0);
#endif
	
	// Trap fault here
	while(1);
}
