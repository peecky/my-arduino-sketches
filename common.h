#ifndef __COMMON_H_IUkFGWDERNBEJ5d8RKvgZ3L26qYaVKSt__
#define __COMMON_H_IUkFGWDERNBEJ5d8RKvgZ3L26qYaVKSt__

#define SerialPrintValue(caption, value) { Serial.print(caption); Serial.print(": "); Serial.println(value); }
#define SerialPrintln(value) { SerialPrintValue(now, value); }

inline int toggleDigitalValue(int& value) {
  return value = value == LOW ? HIGH : LOW;
}


#endif
