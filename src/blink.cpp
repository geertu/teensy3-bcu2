// This example is taken and modified from the Teensyduino 'Blink' sketch
// See: https://www.pjrc.com/teensy/teensyduino.html

#include <WProgram.h>

const int ledPin = 13;

// the setup() method runs once, when the sketch starts
void setup()
{
    // initialize the digital pin as an output.
    pinMode(ledPin, OUTPUT);
}

// the loop() methor runs over and over again,
// as long as the board has power
void loop()
{
    digitalWrite(ledPin, HIGH);   // set the LED on
    delay(1000);                  // wait for a second
    digitalWrite(ledPin, LOW);    // set the LED off
    delay(1000);                  // wait for a second
}

// This is needed as the starting point, as the standard main.cpp gets
// excluded during the build process.
extern "C" int main()
{
    setup();
    for (;;)
    {
        loop();
        yield();
    }
}

