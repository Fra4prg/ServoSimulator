# ServoSimulator

This Servo Simulator diplays the output of an RC receiver in microseconds, percentage and as graphical bars/servos.
The valid signal must be between 700-2300us.
Assuming 1000us=-100%, 1500us=0%, 2000us=+100%.
It uses an Arduino Nano to evaluate up to 8 RC signals and display them on a Nextion LCD.
The power supply is a 2S Lipo via Voltage regulator.
You can use other supplies but the battery icon then is not valid.
Connect the PWM outputs to 1-8 input channels.
CPPM and SBUS options will be added in future.
Click on the bars to change the colors of them.
Click on the servos to change the angle of a rudder horn.
Click on [nor]/[inv] to change the servo direction.



