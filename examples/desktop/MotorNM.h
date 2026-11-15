#ifndef _MOTOR_NM_H
#define _MOTOR_NM_H

#include "StateMachine.h"

/// @brief No macro (NM) test class using StateMachine without code macros. This 
/// class shows the macro expansion to assist in code comprehension. 
class MotorNMData : public EventData
{
public:
	INT speed;
};

class MotorNM : public StateMachine
{
public:
	MotorNM();

	// External events taken by this state machine
	void SetSpeed(MotorNMData* data);
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
	void ST_Idle(const NoEventData*);
	StateAction<MotorNM, NoEventData, &MotorNM::ST_Idle> Idle;

	void ST_Stop(const NoEventData*);
	StateAction<MotorNM, NoEventData, &MotorNM::ST_Stop> Stop;

	void ST_Start(const MotorNMData*);
	StateAction<MotorNM, MotorNMData, &MotorNM::ST_Start> Start;

	void ST_ChangeSpeed(const MotorNMData*);
	StateAction<MotorNM, MotorNMData, &MotorNM::ST_ChangeSpeed> ChangeSpeed;

	// Alternate state definitions using macro support
	//STATE_DECLARE(Motor, 	Idle,			NoEventData)
	//STATE_DECLARE(Motor, 	Stop,			NoEventData)
	//STATE_DECLARE(Motor, 	Start,			MotorData)
	//STATE_DECLARE(Motor, 	ChangeSpeed,	MotorData)

	// State map to define state object order. Each state map entry defines a
	// state object.
private:
	virtual const StateMapRowEx* GetStateMapEx() { return NULL; }
	virtual const StateMapRow* GetStateMap() {
		static const StateMapRow STATE_MAP[] = { 
			&Idle,
			&Stop,
			&Start,
			&ChangeSpeed,
		}; 
		C_ASSERT((sizeof(STATE_MAP)/sizeof(StateMapRow)) == ST_MAX_STATES); 
		return &STATE_MAP[0]; }

	// Alternate state map using macro support
	//BEGIN_STATE_MAP
	//	STATE_MAP_ENTRY(&Idle)
	//	STATE_MAP_ENTRY(&Stop)
	//	STATE_MAP_ENTRY(&Start)
	//	STATE_MAP_ENTRY(&ChangeSpeed)
	//END_STATE_MAP	
};

#endif