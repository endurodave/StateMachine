#include "stdafx.h"
#include "MotorNM.h"
#include "Motor.h"
#include "Player.h"
#include "CentrifugeTest.h"

using namespace std;

int main(void)
{
	// Create MotorNM (No Macro) test object
	MotorNM motorNM;

	// @see StateMachine.h comments
#if EXTERNAL_EVENT_NO_HEAP_DATA
	MotorNMData dataNM;
	dataNM.speed = 100;
	motorNM.SetSpeed(&dataNM);

	MotorNMData dataNM2;
	dataNM2.speed = 200;
	motorNM.SetSpeed(&dataNM2);

	motorNM.Halt();
	motorNM.Halt();

	// Create Motor object with macro support
	Motor motor;

	MotorData data;
	data.speed = 100;
	motor.SetSpeed(&data);

	MotorData data2;
	data2.speed = 200;
	motor.SetSpeed(&data2);

	motor.Halt();
	motor.Halt();
#else
	MotorNMData* dataNM = new MotorNMData();
	dataNM->speed = 100;
	motorNM.SetSpeed(dataNM);

	MotorNMData* dataNM2 = new MotorNMData();
	dataNM2->speed = 200;
	motorNM.SetSpeed(dataNM2);

	motorNM.Halt();
	motorNM.Halt();

	// Create Motor object with macro support
	Motor motor;

	MotorData* data = new MotorData();
	data->speed = 100;
	motor.SetSpeed(data);

	MotorData* data2 = new MotorData();
	data2->speed = 200;
	motor.SetSpeed(data2);

	motor.Halt();
	motor.Halt();
#endif

	// Create Player instance and call external event functions
	Player player;
	player.OpenClose();
	player.OpenClose();
	player.Play();
	player.Pause();
	player.EndPause();
	player.Stop();
	player.Play();
	player.Play();
	player.OpenClose();

	// Create CentrifugeTest and start test
	CentrifugeTest test;
	test.Cancel();
	test.Start();
	while (test.IsPollActive())
		test.Poll();

	return 0;
}

