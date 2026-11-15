// Motor state machine: Idle -> Start -> ChangeSpeed -> Stop -> Idle.
// The same state machine as Motor.h in the repository root, logging through
// a callback instead of std::cout so it shows on an Arduino serial port.

#ifndef _MOTOR_H
#define _MOTOR_H

// The library's main header: "StateMachine.h" alone could resolve to another
// Arduino library of that name
#include <Cpp_StateMachine.h>

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

	INT GetSpeed() const { return m_currentSpeed; }

	// Called with a line of text on every state change
	static void (*Log)(const char* msg);

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

private:
	INT m_currentSpeed;

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
