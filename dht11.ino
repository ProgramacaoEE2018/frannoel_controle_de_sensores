#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#define DEBUG_PRINTER Serial
#ifdef DHT_DEBUG
#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
#define DEBUG_PRINT(...) {}
#define DEBUG_PRINTLN(...) {}
#endif
#define DHT11 11
#define DHT22 22
#define DHT21 21
#define AM2301 21
#define DHTPIN 2
#define DHTTYPE DHT11


class DHT {
  public:
    DHT(uint8_t pin, uint8_t type, uint8_t count = 6);
    void begin(void);
    float readTemperature(bool S = false, bool force = false);
    float convertCtoF(float);
    float convertFtoC(float);
    float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit = true);
    float readHumidity(bool force = false);
    boolean read(bool force = false);

  private:
    uint8_t data[5];
    uint8_t _pin, _type;
#ifdef __AVR
    uint8_t _bit, _port;
#endif
    uint32_t _lastreadtime, _maxcycles;
    bool _lastresult;
    uint32_t expectPulse(bool level);
};

class InterruptLock {
  public:
    InterruptLock() {
      noInterrupts();
    }
    ~InterruptLock() {
      interrupts();
    }

};

#define MIN_INTERVAL 2000

DHT::DHT(uint8_t pin, uint8_t type, uint8_t count) {
  _pin = pin;
  _type = type;
#ifdef __AVR
  _bit = digitalPinToBitMask(pin);
  _port = digitalPinToPort(pin);
#endif
  _maxcycles = microsecondsToClockCycles(1000);  
}

void DHT::begin(void) {
  pinMode(_pin, INPUT_PULLUP);
  _lastreadtime = -MIN_INTERVAL;
  DEBUG_PRINT("Max clock cycles: "); DEBUG_PRINTLN(_maxcycles, DEC);
}

//boolean S == Scale.  True == Fahrenheit; False == Celcius
float DHT::readTemperature(bool S, bool force) {
  float f = NAN;

  if (read(force)) {
    switch (_type) {
      case DHT11:
        f = data[2];
        if (S) {
          f = convertCtoF(f);
        }
        break;
      case DHT22:
      case DHT21:
        f = data[2] & 0x7F;
        f *= 256;
        f += data[3];
        f *= 0.1;
        if (data[2] & 0x80) {
          f *= -1;
        }
        if (S) {
          f = convertCtoF(f);
        }
        break;
    }
  }
  return f;
}

float DHT::convertCtoF(float c) {
  return c * 1.8 + 32;
}

float DHT::convertFtoC(float f) {
  return (f - 32) * 0.55555;
}

float DHT::readHumidity(bool force) {
  float f = NAN;
  if (read()) {
    switch (_type) {
      case DHT11:
        f = data[0];
        break;
      case DHT22:
      case DHT21:
        f = data[0];
        f *= 256;
        f += data[1];
        f *= 0.1;
        break;
    }
  }
  return f;
}

float DHT::computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit) {
 
  float hi;

  if (!isFahrenheit)
    temperature = convertCtoF(temperature);

  hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

  if (hi > 79) {
    hi = -42.379 +
         2.04901523 * temperature +
         10.14333127 * percentHumidity +
         -0.22475541 * temperature * percentHumidity +
         -0.00683783 * pow(temperature, 2) +
         -0.05481717 * pow(percentHumidity, 2) +
         0.00122874 * pow(temperature, 2) * percentHumidity +
         0.00085282 * temperature * pow(percentHumidity, 2) +
         -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if ((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

    else if ((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  return isFahrenheit ? hi : convertFtoC(hi);
}

boolean DHT::read(bool force) {
  
  uint32_t currenttime = millis();
  if (!force && ((currenttime - _lastreadtime) < 2000)) {
    return _lastresult; // return last correct measurement
  }
  _lastreadtime = currenttime;

  
  data[0] = data[1] = data[2] = data[3] = data[4] = 0;

  digitalWrite(_pin, HIGH);
  delay(250);

 
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  delay(20);

  uint32_t cycles[80];
  {
    
    InterruptLock lock;

    digitalWrite(_pin, HIGH);
    delayMicroseconds(40);

    pinMode(_pin, INPUT_PULLUP);
    delayMicroseconds(10);  // Delay a bit to let sensor pull data line low.

    
    if (expectPulse(LOW) == 0) {
      DEBUG_PRINTLN(F("Timeout waiting for start signal low pulse."));
      _lastresult = false;
      return _lastresult;
    }
    if (expectPulse(HIGH) == 0) {
      DEBUG_PRINTLN(F("Timeout waiting for start signal high pulse."));
      _lastresult = false;
      return _lastresult;
    }

    
    for (int i = 0; i < 80; i += 2) {
      cycles[i]   = expectPulse(LOW);
      cycles[i + 1] = expectPulse(HIGH);
    }
  }
  for (int i = 0; i < 40; ++i) {
    uint32_t lowCycles  = cycles[2 * i];
    uint32_t highCycles = cycles[2 * i + 1];
    if ((lowCycles == 0) || (highCycles == 0)) {
      DEBUG_PRINTLN(F("Timeout waiting for pulse."));
      _lastresult = false;
      return _lastresult;
    }
    data[i / 8] <<= 1;
    
    if (highCycles > lowCycles) {
     
      data[i / 8] |= 1;
    }
   
  }

  DEBUG_PRINTLN(F("Received:"));
  DEBUG_PRINT(data[0], HEX); DEBUG_PRINT(F(", "));
  DEBUG_PRINT(data[1], HEX); DEBUG_PRINT(F(", "));
  DEBUG_PRINT(data[2], HEX); DEBUG_PRINT(F(", "));
  DEBUG_PRINT(data[3], HEX); DEBUG_PRINT(F(", "));
  DEBUG_PRINT(data[4], HEX); DEBUG_PRINT(F(" =? "));
  DEBUG_PRINTLN((data[0] + data[1] + data[2] + data[3]) & 0xFF, HEX);

  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    _lastresult = true;
    return _lastresult;
  }
  else {
    DEBUG_PRINTLN(F("Checksum failure!"));
    _lastresult = false;
    return _lastresult;
  }
}

uint32_t DHT::expectPulse(bool level) {
  uint32_t count = 0;
  
#ifdef __AVR
  uint8_t portState = level ? _bit : 0;
  while ((*portInputRegister(_port) & _bit) == portState) {
    if (count++ >= _maxcycles) {
      return 0; // Exceeded timeout, fail.
    }
  }
 
#else
  while (digitalRead(_pin) == level) {
    if (count++ >= _maxcycles) {
      return 0; // Exceeded timeout, fail.
    }
  }
#endif

  return count;
}

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);

  dht.begin();
}

void loop() {
  delay(500);

  int h = dht.readHumidity();
  int t = dht.readTemperature();
  int f = dht.readTemperature(true);

  char x = Serial.read();
  
  if(x == 'a'){
    Serial.print(h);
    Serial.print(","); 
    Serial.print(t);
    Serial.print(","); 
    Serial.println(f);
  }
}
