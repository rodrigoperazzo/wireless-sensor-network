#ifndef HCSR04_h
#define HCSR04_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class HCSR04
{
  public:
    inline HCSR04(){};
    void begin(int trigPin, int echoPin);
    
    float read();
    float sampling(int numSamples, int sampleInterval);
    
  private:
    int _trigPin;
    int _echoPin;
};

#endif