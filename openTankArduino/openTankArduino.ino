//version for openTank
#include <IBusBM.h>   //https://github.com/bmellink/IBusBM
#include "GyverPWM.h" //https://alexgyver.ru/gyverpwm/

#define LEFT_MOTOR 0
#define RIGHT_MOTOR 1

IBusBM IBus; // IBus object

int LeftValue = 0; //left motor
int RightValue = 0; //right motor

void setMotors(int leftMotorValue, int rightMotorValue)
{
    if (LeftValue != leftMotorValue)
    {
        LeftValue = leftMotorValue;
        setMotorPWM(LEFT_MOTOR, LeftValue);
    }
    if (RightValue != rightMotorValue)
    {
        RightValue = rightMotorValue;
        setMotorPWM(RIGHT_MOTOR, RightValue);
    }
}

void setMotorPWM(int motorNumber, int channelValue)
{
    int motorFrontPWM, motorBackPWM = 0;

    if (motorNumber == LEFT_MOTOR) //Left motor
    {
        if (channelValue > 1500) //forward
        {
            motorFrontPWM = map(channelValue, 1500, 2000, 0, 255); //255 => 8 bit timer
        }
        else if (channelValue < 1500) //backward
        {
            motorBackPWM = map(channelValue, 1500, 1000, 0, 255); //255 => 8 bit timer
        }

        PWM_16KHZ_D3(motorFrontPWM);
        PWM_16KHZ_D5(motorBackPWM);
    }
    else if (motorNumber == RIGHT_MOTOR) //Right motor
    {
        if (channelValue > 1500) //forward
        {
            motorFrontPWM = map(channelValue, 1500, 2000, 0, 1023); //1023 => 10 bit timer
        }
        else if (channelValue < 1500) //backward
        {
            motorBackPWM = map(channelValue, 1500, 1000, 0, 1023); //1023 => 10 bit timer
        }

        PWM_16KHZ_D9(motorFrontPWM);
        PWM_16KHZ_D10(motorBackPWM);
    }
}

void setup()
{
    // iBUS object connected to serial0 RX pin (rx0)
    IBus.begin(Serial);

    //ToDo
    // *Wait IBus until connect
    // *Create arming function

    pinMode(3, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);

    PWM_16KHZ_D3(0);
    PWM_16KHZ_D5(0);
    PWM_16KHZ_D9(0);
    PWM_16KHZ_D10(0);

    delay(100);
}

void loop()
{
    //channel values has range 1000..2000 1000 is minimum, 2000is maximum, 1500 equals center
    uint16_t ch_1 = IBus.readChannel(1); //left stick Y
    uint16_t ch_2 = IBus.readChannel(2); //right stick Y

    setMotors(ch_1, ch_2);
}
