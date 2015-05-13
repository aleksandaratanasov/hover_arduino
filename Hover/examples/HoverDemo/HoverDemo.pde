/*
* This code was written for a Fubarino Mini.
*/

#include <Wire.h>
#include <Hover.h>
#include <StringBuilder.h>

// Pin declarations for Hover
int ts    = PIN_INT0;
int reset = 19;

Hover hover = Hover(ts, reset);
StringBuilder output;
StringBuilder input;
StringBuilder output_async;
uint32_t last_time_sent = 0;

uint32_t activity_timeout = 0;

uint32_t output_period = 15;   // In milliseconds.


int8_t process_string_from_counterparty(StringBuilder* in) {
  if (in == NULL) return -1;
  if (in->length() < 11) return -2;
  
  int8_t return_value = 0;
  const char* test = (const char*) in->string();

  if (strcasestr(test, "CMD ")) {
    switch (*(test+10)) {  // Look at the eleventh char. We are getting a command.
      case '0':
        output_async.clear();
        hover.printDebug(&output_async);
        Serial.println((char*)output_async.string());
        return_value = 1;
        break;
      case '1':
        output_async.clear();
        output_async.concat(*(test+11));
        return_value = 1;
        break;
      case '4':
        hover.enableAirwheel(false);
        Serial.println("Airwheel disabled\n");
        return_value = 1;
        break;
      case '5':
        hover.enableAirwheel(true);
        Serial.println("Airwheel enabled\n");
        return_value = 1;
        break;
      case '6':
        hover.enableApproachDetect(false);
        Serial.println("Approach detect disabled\n");
        return_value = 1;
        break;
      case '7':
        hover.enableApproachDetect(true);
        Serial.println("Approach detect enabled\n");
        return_value = 1;
        break;
      case '8':
        digitalWrite(1,HIGH);
        return_value = 1;
        break;
      case '9':
        digitalWrite(1,LOW);
        return_value = 1;
        break;
      case 'p':
        {
          uint32_t op_temp = atoi((const char*)(test+11));
          if ((op_temp > 5) && (op_temp < 10000)) {
            output_period = op_temp;
            output_async.clear();
            output_async.concatf("Output period is now %dms.\n", output_period);
            Serial.println((const char*)output_async.string());
          }
          else {
            Serial.println("Valid range for output period is 5-10000 ms.\n");
          }
        }
        break;
    }
  }
  in->clear();
  return return_value;
}


void setup() {
  Serial.begin(115200);   // USB debugging.
  pinMode(1, OUTPUT);
  delay(1000);
  Wire.begin();
  Serial.println("Initializing Hover...please wait.");
  hover.begin();
  last_time_sent = millis();
  hover.service();
  hover.markClean();
}


void loop() {
  char c = '\0';
  if (Serial.available()) {
    c = Serial.read();
  }
  
  switch (c) {
    case '\n':
      // Process the string.
      input.concat("\r\n\0");
      process_string_from_counterparty(&input);
      break;
    case '\r':
      break;
    default:
      input.concat(c);
      break;
  }
  
  // Check if Hover is ready to send gesture or touch events
  hover.service();

  if ((millis() - last_time_sent) >= output_period) {
    if (hover.isDirty()) {
      output.clear();

      if (hover.isPositionDirty()) output.concatf("Position 0x%04x,0x%04x,0x%04x\r\n", hover._pos_x, hover._pos_y, hover._pos_z);
      if (hover.wheel_position)    output.concatf("AirWheel %d\r\n", hover.wheel_position);
      if (hover.last_swipe)        output.concatf("Swipe %d\r\n", hover.last_swipe);
      if (hover.last_tap)          output.concatf("Tap %d\r\n", hover.last_tap);
      if (hover.last_double_tap)   output.concatf("Double tap %d\r\n", hover.last_double_tap);
      if (hover.isTouchDirty())    output.concatf("Touch %d\r\n", hover.last_touch);
      if (hover.special)           output.concatf("Special condition %d\r\n", hover.special);

      hover.markClean();
      Serial.print((char*)output.string());
      last_time_sent = millis();
    }
  }
  
  c = '\0';
}

