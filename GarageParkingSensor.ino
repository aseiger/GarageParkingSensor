#include <EEPROM.h>

#define DURATION_FILTER_LENGTH 4

long duration;
long durationChange;
long durationFilterDerivative[DURATION_FILTER_LENGTH - 1];

const long durationChangeThreshold = 500;

long lastMillis;

long signEnabledCount = 0;
long vehiclePositionThreshold;
const long vehiclePositionHysteriesis = 500 ;


long durationFilter[DURATION_FILTER_LENGTH];

// this constant won't change. It's the pin number of the sensor's output:
const int pingPin = 7;

bool isButtonPressed = false;
const int buttonPin = 8;

bool walkSignOn = false;
bool dontWalkSignOn = false;
const int walkSignPin = 2;
const int dontWalkSignPin = 3;

void setup() {
  // initialize serial communication:
  Serial.begin(115200);
  lastMillis = millis();

  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(walkSignPin, OUTPUT);
  pinMode(dontWalkSignPin, OUTPUT);

  digitalWrite(walkSignPin, HIGH);
  digitalWrite(dontWalkSignPin, HIGH);

  vehiclePositionThreshold = 0;
  for(int i = 0; i < sizeof(long); i++)
  {
    vehiclePositionThreshold = vehiclePositionThreshold | (EEPROM.read(i) << (8*i));
  }

}

void loop() {
  // establish variables for duration of the ping, and the distance result
  // in inches and centimeters:

  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH pulse
  // whose duration is the time (in microseconds) from the sending of the ping
  // to the reception of its echo off of an object.
  pinMode(pingPin, INPUT);
  duration = pulseIn(pingPin, HIGH);

  //index to the next filter step
  for(int i = DURATION_FILTER_LENGTH-1; i > 0; i--)
  {
    durationFilter[i] = durationFilter[i-1];
  }
  durationFilter[0] = duration;

  //compute the averaged out duration - moving average filter
  duration = 0;
  for(int i = 0; i < DURATION_FILTER_LENGTH; i++)
  {
    duration = duration + durationFilter[i];
  }
  duration = duration / DURATION_FILTER_LENGTH;

  //compute the derivative of the filter
  durationChange = 0;
  for(int i = 0; i < DURATION_FILTER_LENGTH-1; i++)
  {
    durationFilterDerivative[i] = durationFilter[i+1] - durationFilter[i];
  }
  //compute the average of the derivative
  for(int i = 0; i < DURATION_FILTER_LENGTH-1; i++)
  {
    durationChange = durationChange + durationFilterDerivative[i];
  }

  //determine if signs should be enabled
  if((durationChange > durationChangeThreshold) & (signEnabledCount == 0))
  { 
    signEnabledCount = 2400;
  }

  if((digitalRead(buttonPin) == LOW) & (isButtonPressed == false))
  {
    isButtonPressed = true;
    vehiclePositionThreshold = duration;
    for(int i = 0; i < sizeof(long); i++)
    {
      EEPROM.write(i, 0xFF & (vehiclePositionThreshold >> (8*i)));
    }
  }

  if(digitalRead(buttonPin) == HIGH)
  {
    isButtonPressed = false;
  }

  

  Serial.print(duration);
  Serial.print(",");
  if(signEnabledCount > 0)
  {
    signEnabledCount--;
    Serial.print(vehiclePositionThreshold);
    Serial.print(",");
    Serial.print("SIGNS_ENABLED");
    Serial.print(",");
    if((duration > vehiclePositionThreshold + vehiclePositionHysteriesis) | (duration < vehiclePositionThreshold - vehiclePositionHysteriesis))
    {
      Serial.println("CAR IN BAD PLACE");
      digitalWrite(walkSignPin, HIGH);
      digitalWrite(dontWalkSignPin, LOW);
    }
    else
    {
      Serial.println("OK");
      digitalWrite(walkSignPin, LOW);
      digitalWrite(dontWalkSignPin, HIGH);
    }
  }
  else
  {
    digitalWrite(walkSignPin, HIGH);
    digitalWrite(dontWalkSignPin, HIGH);
    Serial.println();
  }
  
  delay(100 - (millis() - lastMillis));
  lastMillis = millis();
}
