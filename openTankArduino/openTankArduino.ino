//version for openTank
#include <IBusBM.h>   //https://github.com/bmellink/IBusBM
#include "GyverPWM.h" //https://alexgyver.ru/gyverpwm/

IBusBM IBus; // IBus object

void setMotors(int leftMotor, int rightMotor)
{
    //Left motor
    int leftMotorFrontPWM, leftMotorBackPWM = 0;

    if (leftMotor > 1500) //left motor forward
    {
        leftMotorFrontPWM = map(leftMotor, 1500, 2000, 0, 255); //255 => 8 bit timer
    }
    else if (leftMotor < 1500) //left motor forward
    {
        leftMotorBackPWM = map(leftMotor, 1500, 1000, 0, 255); //255 => 8 bit timer
    }

    //Right motor
    int rightMotorFrontPWM, rightMotorBackPWM = 0;

    if (rightMotor > 1500) //right motor forward
    {
        rightMotorFrontPWM = map(rightMotor, 1500, 2000, 0, 1023); //1023 => 10 bit timer
    }
    else if (rightMotor < 1500) //right motor forward
    {
        rightMotorBackPWM = map(rightMotor, 1500, 1000, 0, 1023); //1023 => 10 bit timer
    }

    PWM_16KHZ_D3(leftMotorFrontPWM);
    PWM_16KHZ_D5(leftMotorBackPWM);
    PWM_16KHZ_D9(rightMotorFrontPWM);
    PWM_16KHZ_D10(rightMotorBackPWM);
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
    uint16_t ch_1 = IBus.readChannel(2); //left stick Y
    uint16_t ch_2 = IBus.readChannel(1); //right stick Y

    setMotors(ch_1, ch_2);
}
