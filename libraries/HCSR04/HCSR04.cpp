#include <HCSR04.h>

void HCSR04::begin(int trigPin, int echoPin)
{
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT); 
  
  _trigPin = trigPin;
  _echoPin = echoPin;
}

float HCSR04::read()
{
	// Start Ranging
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);
    
    // Compute distance
    return ((float)pulseIn(_echoPin, HIGH))/58;
}

float HCSR04::sampling(int numSamples, int sampleInterval)
{
  float smoothedDistance = read();
  float currentDistance = 0;
  int i = 0;
 
  for (i = 1; i < numSamples; i++)
  {
      delay(sampleInterval);
      currentDistance = read();
      
      // low pass filter 
      smoothedDistance += (1/numSamples)*(currentDistance-smoothedDistance);
  }
  
  return smoothedDistance;
}