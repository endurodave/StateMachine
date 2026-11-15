#include "SelfTest.h"
#include <iostream>

using namespace std;

SelfTest::SelfTest(INT maxStates) :
	StateMachine(maxStates)
{
}
	
void SelfTest::Cancel()
{
#if 1
	// External event functions in a state machine base class may use a partial transition 
	// map if preceded by PARENT_TRANSITION. The partial transition map only includes
	// entries up to the number of states known at the current level within the 
	// hierarchy. PARENT_TRANSITION must be used in anything but the most-derived
	// state machine class. The macro is as a catch-all used to capture the transition 
	// to use when the state machine current state is in a parent state. The statement
	// below is read "If the Cancel event is generated when the state machine is not in
	// ST_IDLE, ST_COMPLETE, or ST_FAILED then transition to the ST_FAILED state". 
	// If the base Cancel event is not appropriate for some parent classes, make Cancel()
	// virtual and define Cancel() in the derived class with a full transition map. 
	PARENT_TRANSITION (ST_FAILED)

	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_IDLE
		TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)				// ST_COMPLETED
		TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)				// ST_FAILED
	END_TRANSITION_MAP(NULL)

#else
	// Alternatively external events can be generated manually without a 
	// transition map. 
	if (GetCurrentState() != ST_IDLE)
		ExternalEvent(ST_FAILED);
#endif
}

STATE_DEFINE(SelfTest, Idle, NoEventData)
{
	cout << "SelfTest::ST_Idle" << endl;
}

ENTRY_DEFINE(SelfTest, EntryIdle, NoEventData)
{
	cout << "SelfTest::EN_EntryIdle" << endl;
}

STATE_DEFINE(SelfTest, Completed, NoEventData)
{
	cout << "SelfTest::ST_Completed" << endl;
	InternalEvent(ST_IDLE);
}

STATE_DEFINE(SelfTest, Failed, NoEventData)
{
	cout << "SelfTest::ST_Failed" << endl;
	InternalEvent(ST_IDLE);
}

