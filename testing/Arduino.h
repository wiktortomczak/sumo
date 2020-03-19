// Forward declarations of a few basic symbols from Arduino.h,
// to allow compilation of this project's code in a separate compiler invocation
// (with STL) without pulling real Arduino.h (which does not work with STL).
//
// TODO: Remove when the STL port is fixed well enough to work with
// real Arduino.h.

extern "C" {

#define LED_BUILTIN 13
#define OUTPUT 0x1
#define HIGH 0x1
#define LOW 0x0

  
extern void init();

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

void delay(unsigned long ms);

}


#define SERIAL_8N1 0x06

class Print {
public:
  void println(const char*);
  virtual void flush();
};

class HardwareSerial : public Print {
 public:
  static_assert(sizeof(unsigned long) == 4);
  void begin(unsigned long baud) { begin(baud, SERIAL_8N1); }
  void begin(unsigned long, uint8_t);
};

extern HardwareSerial Serial;
