#ifndef _MOTOR_H
#define _MOTOR_H

#include "StateMachine.h"

class MotorData : public EventData
{
public:
	INT speed;
};

class Motor : public StateMachine
{
public:
	Motor();

	// External events taken by this state machine
	void SetSpeed(MotorData* data);
	void Halt();

private:
	INT m_currentSpeed; 

	// State enumeration order must match the order of state method entries
	// in the state map.
	enum States
	{
		ST_IDLE,
		ST_STOP,
		ST_START,
		ST_CHANGE_SPEED,
		ST_MAX_STATES
	};

	// Define the state machine state functions with event data type
	STATE_DECLARE(Motor, 	Idle,			NoEventData)
	STATE_DECLARE(Motor, 	Stop,			NoEventData)
	STATE_DECLARE(Motor, 	Start,			MotorData)
	STATE_DECLARE(Motor, 	ChangeSpeed,	MotorData)

	// State map to define state object order. Each state map entry defines a
	// state object.
	BEGIN_STATE_MAP
		STATE_MAP_ENTRY(&Idle)
		STATE_MAP_ENTRY(&Stop)
		STATE_MAP_ENTRY(&Start)
		STATE_MAP_ENTRY(&ChangeSpeed)
	END_STATE_MAP	
};

#endif