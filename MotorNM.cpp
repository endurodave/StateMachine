#include "MotorNM.h"
#include <iostream>

using namespace std;

MotorNM::MotorNM() :
	StateMachine(ST_MAX_STATES),
	m_currentSpeed(0)
{
}
	
// set motor speed external event
void MotorNM::SetSpeed(MotorNMData* data)
{
	static const BYTE TRANSITIONS[] = {
		ST_START,						// ST_IDLE
		CANNOT_HAPPEN,					// ST_STOP
		ST_CHANGE_SPEED,				// ST_START
		ST_CHANGE_SPEED,				// ST_CHANGE_SPEED
	};
    ExternalEvent(TRANSITIONS[GetCurrentState()], data); 
	C_ASSERT((sizeof(TRANSITIONS)/sizeof(BYTE)) == ST_MAX_STATES); 

	// Alternate transition map using macro support
	//BEGIN_TRANSITION_MAP			              			// - Current State -
	//	TRANSITION_MAP_ENTRY (ST_START)						// ST_IDLE
	//	TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)				// ST_STOP
	//	TRANSITION_MAP_ENTRY (ST_CHANGE_SPEED)				// ST_START
	//	TRANSITION_MAP_ENTRY (ST_CHANGE_SPEED)				// ST_CHANGE_SPEED
	//END_TRANSITION_MAP(data)
}

// halt motor external event
void MotorNM::Halt()
{
	static const BYTE TRANSITIONS[] = {
		EVENT_IGNORED,					// ST_IDLE
		CANNOT_HAPPEN,					// ST_STOP
		ST_STOP,						// ST_START
		ST_STOP,						// ST_CHANGE_SPEED
	};
    ExternalEvent(TRANSITIONS[GetCurrentState()], NULL); 
	C_ASSERT((sizeof(TRANSITIONS)/sizeof(BYTE)) == ST_MAX_STATES); 

	// Alternate transition map using macro support
	//BEGIN_TRANSITION_MAP			              			// - Current State -
	//	TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_IDLE
	//	TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)				// ST_STOP
	//	TRANSITION_MAP_ENTRY (ST_STOP)						// ST_START
	//	TRANSITION_MAP_ENTRY (ST_STOP)						// ST_CHANGE_SPEED
	//END_TRANSITION_MAP(NULL)
}

// state machine sits here when motor is not running
//STATE_DEFINE(Motor, Idle, NoEventData)
void MotorNM::ST_Idle(const NoEventData* data)
{
	cout << "MotorNM::ST_Idle" << endl;
}

// stop the motor 
//STATE_DEFINE(Motor, Stop, NoEventData)
void MotorNM::ST_Stop(const NoEventData* data)
{
	cout << "MotorNM::ST_Stop" << endl;
	m_currentSpeed = 0; 

	// perform the stop motor processing here
	// transition to Idle via an internal event
	InternalEvent(ST_IDLE);
}

// start the motor going
//STATE_DEFINE(Motor, Start, MotorData)
void MotorNM::ST_Start(const MotorNMData* data)
{
	cout << "MotorNM::ST_Start : Speed is " << data->speed << endl;
	m_currentSpeed = data->speed;

	// set initial motor speed processing here
}

// changes the motor speed once the motor is moving
//STATE_DEFINE(Motor, ChangeSpeed, MotorData)
void MotorNM::ST_ChangeSpeed(const MotorNMData* data)
{
	cout << "MotorNM::ST_ChangeSpeed : Speed is " << data->speed << endl;
	m_currentSpeed = data->speed;

	// perform the change motor speed to data->speed here
}
