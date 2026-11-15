// StateMachine PlatformIO example: a motor state machine driven by external
// events, with typed event data. Runs on Arduino boards and the desktop
// (native).
//
// Event data is created with new and deleted by the state machine after the
// state function runs.
//
// @see https://github.com/endurodave/StateMachine

#include "Motor.h"
#include <stdio.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

// Set while Stress() runs, so its 1000 state changes are not logged
static bool quiet = false;

static void Log(const char* msg)
{
    if (quiet)
        return;
#ifdef ARDUINO
    Serial.println(msg);
#else
    printf("%s\n", msg);
#endif
}

static void SetSpeed(Motor& motor, INT speed)
{
    MotorData* data = new MotorData();      // deleted by the state machine
    data->speed = speed;
    motor.SetSpeed(data);
}

static bool Demo()
{
    Motor motor;
    bool ok = true;

    SetSpeed(motor, 100);                   // Idle -> Start
    ok = ok && motor.GetCurrentState() == Motor::ST_START && motor.GetSpeed() == 100;
    SetSpeed(motor, 200);                   // Start -> ChangeSpeed
    ok = ok && motor.GetCurrentState() == Motor::ST_CHANGE_SPEED && motor.GetSpeed() == 200;
    motor.Halt();                           // ChangeSpeed -> Stop -> Idle
    ok = ok && motor.GetCurrentState() == Motor::ST_IDLE && motor.GetSpeed() == 0;
    motor.Halt();                           // ignored in Idle
    ok = ok && motor.GetCurrentState() == Motor::ST_IDLE;
    return ok;
}

// Many events: each event's data must be deleted again, or the heap runs out
static bool Stress()
{
    Motor motor;
    quiet = true;
    bool ok = true;
    for (INT i = 1; i <= 1000 && ok; i++)
    {
        SetSpeed(motor, i);
        ok = motor.GetSpeed() == i;
        if (i % 100 == 0)
        {
            motor.Halt();
            ok = ok && motor.GetCurrentState() == Motor::ST_IDLE;
        }
    }
    quiet = false;
    Log(ok ? "1000 events processed" : "stress test failed");
    return ok;
}

static bool Run()
{
    Motor::Log = Log;
    bool ok = Demo() && Stress();
    Log(ok ? "StateMachine demo PASSED" : "StateMachine demo FAILED");
    return ok;
}

#ifdef ARDUINO
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Log("StateMachine demo");
    Run();
}

void loop()
{
}
#else
int main()
{
    Log("StateMachine demo");
    return Run() ? 0 : 1;
}
#endif
