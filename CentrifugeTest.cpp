#include "CentrifugeTest.h"
#include <iostream>

using namespace std;

CentrifugeTest::CentrifugeTest() :
	SelfTest(ST_MAX_STATES),
	m_pollActive(FALSE),
	m_speed(0)
{
}
	
void CentrifugeTest::Start()
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (ST_START_TEST)				// ST_IDLE
		TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)				// ST_COMPLETED
		TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)				// ST_FAILED
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_START_TEST
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_ACCELERATION
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_WAIT_FOR_ACCELERATION
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_DECELERATION
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_WAIT_FOR_DECELERATION
	END_TRANSITION_MAP(NULL)
}

void CentrifugeTest::Poll()
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_IDLE
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_COMPLETED
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_FAILED
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_START_TEST
		TRANSITION_MAP_ENTRY (ST_WAIT_FOR_ACCELERATION)		// ST_ACCELERATION
		TRANSITION_MAP_ENTRY (ST_WAIT_FOR_ACCELERATION)		// ST_WAIT_FOR_ACCELERATION
		TRANSITION_MAP_ENTRY (ST_WAIT_FOR_DECELERATION)		// ST_DECELERATION
		TRANSITION_MAP_ENTRY (ST_WAIT_FOR_DECELERATION)		// ST_WAIT_FOR_DECELERATION
	END_TRANSITION_MAP(NULL)
}

// Idle state here overrides the SelfTest Idle state. 
STATE_DEFINE(CentrifugeTest, Idle, NoEventData)
{
	cout << "CentrifugeTest::ST_Idle" << endl;

	// Call base class Idle state
	SelfTest::ST_Idle(data);	
	StopPoll();
}

// Start the centrifuge test state.
STATE_DEFINE(CentrifugeTest, StartTest, NoEventData)
{
	cout << "CentrifugeTest::ST_StartTest" << endl;
	InternalEvent(ST_ACCELERATION);
}

// Guard condition to detemine whether StartTest state is executed.
GUARD_DEFINE(CentrifugeTest, GuardStartTest, NoEventData)
{
	cout << "CentrifugeTest::GD_GuardStartTest" << endl;
	if (m_speed == 0)
		return TRUE;	// Centrifuge stopped. OK to start test.
	else
		return FALSE;	// Centrifuge spinning. Can't start test.
}

// Start accelerating the centrifuge.
STATE_DEFINE(CentrifugeTest, Acceleration, NoEventData)
{
	cout << "CentrifugeTest::ST_Acceleration" << endl;	

	// Start polling while waiting for centrifuge to ramp up to speed
	StartPoll();
}

// Wait in this state until target centrifuge speed is reached.
STATE_DEFINE(CentrifugeTest, WaitForAcceleration, NoEventData)
{
	cout << "CentrifugeTest::ST_WaitForAcceleration : Speed is " << m_speed << endl;
	if (++m_speed >= 5)
		InternalEvent(ST_DECELERATION);
}

// Exit action when WaitForAcceleration state exits.
EXIT_DEFINE(CentrifugeTest, ExitWaitForAcceleration)
{
	cout << "CentrifugeTest::EX_ExitWaitForAcceleration" << endl;

	// Acceleration over, stop polling
	StopPoll();
}

// Start decelerating the centrifuge.
STATE_DEFINE(CentrifugeTest, Deceleration, NoEventData)
{
	cout << "CentrifugeTest::ST_Deceleration" << endl;

	// Start polling while waiting for centrifuge to ramp down to 0
	StartPoll();
}

// Wait in this state until centrifuge speed is 0.
STATE_DEFINE(CentrifugeTest, WaitForDeceleration, NoEventData)
{
	cout << "CentrifugeTest::ST_WaitForDeceleration : Speed is " << m_speed << endl;
	if (m_speed-- == 0)
		InternalEvent(ST_COMPLETED);
}

// Exit action when WaitForDeceleration state exits.
EXIT_DEFINE(CentrifugeTest, ExitWaitForDeceleration)
{
	cout << "CentrifugeTest::EX_ExitWaitForDeceleration" << endl;

	// Deceleration over, stop polling
	StopPoll();
}
