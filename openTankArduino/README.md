## OpenTank Arduino

- Arduino Nano (ATmega328)
- Any IBus receiver
- DC motors
- L293D
- 12V Baterry

## Arduinio library requirements

- [IBusBM](https://github.com/bmellink/IBusBM)
- [GyverPWM](https://github.com/AlexGyver/GyverLibs/releases/download/GyverPWM/GyverPWM.zip)

<hr>

## Wiring

<img src="../img/openTankArduino.png">

| L293D| Arduino Nano  | Receiver |
|--|--|--|
|5V|Vcc|Vcc|
|Gnd|Gnd|Gnd|
| Left motor A | D3|
| Left motor B | D5|
| Right motor A | D9|
| Right motor B | D10|
| | RX| Sbus receiver output|
