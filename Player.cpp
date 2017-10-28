#include "Player.h"
#include <iostream>

using namespace std;

Player::Player() :
	StateMachine(ST_MAX_STATES)
{
}
	
void Player::OpenClose()
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (ST_OPEN)						// ST_EMPTY
		TRANSITION_MAP_ENTRY (ST_EMPTY)						// ST_OPEN
		TRANSITION_MAP_ENTRY (ST_OPEN)						// ST_STOPPED
		TRANSITION_MAP_ENTRY (ST_OPEN)						// ST_PAUSED
		TRANSITION_MAP_ENTRY (ST_OPEN)						// ST_PLAYING
	END_TRANSITION_MAP(NULL)
}

void Player::Play()
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_EMPTY
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_OPEN
		TRANSITION_MAP_ENTRY (ST_PLAYING)					// ST_STOPPED
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_PAUSED
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_PLAYING
	END_TRANSITION_MAP(NULL)
}

void Player::Stop()
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_EMPTY
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_OPEN
		TRANSITION_MAP_ENTRY (ST_STOPPED)					// ST_STOPPED
		TRANSITION_MAP_ENTRY (ST_STOPPED)					// ST_PAUSED
		TRANSITION_MAP_ENTRY (ST_STOPPED)					// ST_PLAYING
	END_TRANSITION_MAP(NULL)
}

void Player::Pause()
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_EMPTY
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_OPEN
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_STOPPED
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_PAUSED
		TRANSITION_MAP_ENTRY (ST_PAUSED)					// ST_PLAYING
	END_TRANSITION_MAP(NULL)
}

void Player::EndPause()
{
	BEGIN_TRANSITION_MAP			              			// - Current State -
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_EMPTY
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_OPEN
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_STOPPED
		TRANSITION_MAP_ENTRY (ST_PLAYING)					// ST_PAUSED
		TRANSITION_MAP_ENTRY (EVENT_IGNORED)				// ST_PLAYING
	END_TRANSITION_MAP(NULL)
}

STATE_DEFINE(Player, Empty, NoEventData)
{
	static BOOL CD_DetectedToggle = FALSE;
	CD_DetectedToggle = CD_DetectedToggle == FALSE ? TRUE : FALSE; 

	cout << "Player::ST_Empty" << endl;
	if (CD_DetectedToggle == TRUE)
		InternalEvent(ST_STOPPED);
}

STATE_DEFINE(Player, Open,	NoEventData)
{
	cout << "Player::ST_Open" << endl;
}

STATE_DEFINE(Player, Stopped, NoEventData)
{
	cout << "Player::ST_Stopped" << endl;
}

STATE_DEFINE(Player, Paused, NoEventData)
{
	cout << "Player::ST_Paused" << endl;
}

STATE_DEFINE(Player, Playing, NoEventData)
{
	cout << "Player::ST_Playing" << endl;
}