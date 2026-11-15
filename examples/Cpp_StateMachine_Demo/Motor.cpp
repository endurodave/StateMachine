#include "Motor.h"
#include <stdio.h>

static void NoLog(const char*) {}
void (*Motor::Log)(const char* msg) = NoLog;

Motor::Motor() :
	StateMachine(ST_MAX_STATES),
	m_currentSpeed(0)
{
}

// Set motor speed external event
void Motor::SetSpeed(MotorData* data)
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (ST_START)						// ST_IDLE
		TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)				// ST_STOP
		TRANSITION_MAP_ENTRY (ST_CHANGE_SPEED)				// ST_START
		TRANSITION_MAP_ENTRY (ST_CHANGE_SPEED)				// ST_CHANGE_SPEED
	END_TRANSITION_MAP(data)
}

// Halt motor external event
void Motor::Halt()
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_IDLE
		TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)				// ST_STOP
		TRANSITION_MAP_ENTRY (ST_STOP)						// ST_START
		TRANSITION_MAP_ENTRY (ST_STOP)						// ST_CHANGE_SPEED
	END_TRANSITION_MAP(NULL)
}

// State machine sits here when motor is not running
STATE_DEFINE(Motor, Idle, NoEventData)
{
	Log("Motor::ST_Idle");
}

// Stop the motor
STATE_DEFINE(Motor, Stop, NoEventData)
{
	Log("Motor::ST_Stop");
	m_currentSpeed = 0;

	// Perform the stop motor processing here
	// Transition to ST_Idle via an internal event
	InternalEvent(ST_IDLE);
}

// Start the motor going
STATE_DEFINE(Motor, Start, MotorData)
{
	char buf[48];
	snprintf(buf, sizeof(buf), "Motor::ST_Start : Speed is %d", (int)data->speed);
	Log(buf);
	m_currentSpeed = data->speed;
}

// Changes the motor speed once the motor is moving
STATE_DEFINE(Motor, ChangeSpeed, MotorData)
{
	char buf[48];
	snprintf(buf, sizeof(buf), "Motor::ST_ChangeSpeed : Speed is %d", (int)data->speed);
	Log(buf);
	m_currentSpeed = data->speed;
}
