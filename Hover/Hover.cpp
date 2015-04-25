/*  ===========================================================================
#  This is the library for Hover. 
#  
#  Hover is a development kit that lets you control your hardware projects in a whole new way.  
#  Wave goodbye to physical buttons. Hover detects hand movements in the air for touch-less interaction.  
#  It also features five touch-sensitive regions for even more options.
#  Hover uses I2C and 2 digital pins. It is compatible with Arduino, Raspberry Pi and more.
#
#  Hover can be purchased here: http://www.justhover.com
#
#  Written by Emran Mahbub and Jonathan Li for Gearseven Studios.  
#  BSD license, all text above must be included in any redistribution
#  ===========================================================================
#
#  INSTALLATION
#  The 3 library files (Hover.cpp, Hover.h and keywords.txt) in the Hover folder should be placed in your Arduino Library folder.
#  Run the HoverDemo.ino file from your Arduino IDE.
#
#  SUPPORT
#  For questions and comments, email us at support@gearseven.com
#  v2.1
#  ===========================================================================*/

#include <Hover.h>


#if (defined(__MK20DX128__) || defined(__MK20DX256__))
  #include <i2c_t3.h>

#elif defined(ARDUINO)    // Perhaps this is an arduino-style env?
  #include <Wire.h>

#endif   // Platform Wire.h case-offs


Hover::Hover(uint8_t addr) {
  _i2caddr = addr;
}

void Hover::begin(int ts, int rst) {
  Wire.begin();
  pinMode(ts, INPUT);    //Used by TS line on MGC3130
  pinMode(rst, OUTPUT);    //Used by reset line on MGC3130
  digitalWrite(rst, LOW);
  pinMode(rst, INPUT);    
  delay(3000);
  Serial.println("Hover is ready");
}

void Hover::setRelease(int ts) {
    digitalWrite(ts, HIGH);
    pinMode(ts, INPUT);
}

boolean Hover::getStatus(int ts) {
  if (digitalRead(ts) == 1) {
    pinMode(ts, OUTPUT);
    digitalWrite(ts, LOW);
    return 0;
  }  
  return 1;
}


/**
* 
*/
byte Hover::getEvent(void) {
  String output;
  byte data;
  byte event;
  int c = 0;
  
  uint16_t data_set = 0;
  
  uint16_t _pos_x;
  uint16_t _pos_y;
  uint16_t _pos_z;
  
  bool pos_valid = false;
  
  // Pick some safe number. We might limit this depending on what the sensor has to say.
  uint8_t bytes_expected = 32;  
  Wire.requestFrom((uint8_t)_i2caddr, (uint8_t) bytes_expected);    // request 26 bytes from slave device at 0x42

  while(Wire.available() || (0 == bytes_expected--)) {     
    #if defined(_BOARD_FUBARINO_MINI_)    // Fubarino
	  data = Wire.receive(); // receive a byte as character
	#else
	  data = Wire.read(); // receive a byte as character
	#endif

    switch (c) {
      case 0:   // Length of the transfer by the sensor's reckoning.
        if (bytes_expected < (data-1)) {
          bytes_expected = data - 1;  // Minus 1 because: we have already read one.
        }
        break;
      case 1:   // Flags.
        break;
      case 2:   // Sequence number
        break;
      case 3:   // Unique ID
        break;
      case 4:   // Data output config mask is a 16-bit value.
        data_set = data;
        break;
      case 5:   // Data output config mask is a 16-bit value.
        data_set += data << 8;
        break;
      case 6:   // Timestamp (by the sensor's clock). Mostly useless unless we are paranoid about sample drop.
        break;
      case 7:   // System info.
        pos_valid = (data & 0x01);   // So far this is the only bit we care about.
        break;
      
      /* Below this point, we enter the "variable-length" area. God speed.... */
      case 8:   //  DSP info
      case 9: 
        break;
      case 10:  // GestureInfo
      case 11: 
      case 12: 
      case 13: 
        break;
      case 14:  // TouchInfo
      case 15: 
      case 16: 
      case 17: 
        break;
      case 18:  // AirWheelInfo 
      case 19: 
        break;
      case 20:  // Position. This is a vector of 3 uint16's, but we're going to convert to float
        _pos_x = data;
        break;
      case 21:  // and move the origin.
        _pos_x = data << 8;
        break;
      case 22: 
        _pos_y = data;
        break;
      case 23: 
        _pos_y = data << 8;
        break;
      case 24: 
        _pos_z = data;
        break;
      case 25:
        _pos_z = data << 8;
        if ((pos_valid) && (_pos_x & _pos_y & _pos_z)) {
          pos_x = (0xFFFF / (float) _pos_x);
          pos_y = (0xFFFF / (float) _pos_y);
          pos_z = (0xFFFF / (float) _pos_z);
          output.concat("Position: (");
          output.concat(pos_x);
          output.concat(", ");
          output.concat(pos_y);
          output.concat(", ");
          output.concat(pos_z);
          output.concat(")");
        }
        break;

      case 26:   // NoisePower 
      case 27: 
      case 28: 
      case 29: 
        break;
        
      default:
        // There may be up to 40 more bytes after this, but we aren't dealing with them.
        break;
    }
    
    if (c == 1 && data > 1) {
      event = (B00000001 << (data-1)) | B00100000;
      //return event;
    }
    else if (c == 10 && data > 1) {
      event = (B00000001 << (data-1)) | B00100000;
      //return event;
    }
    else if (c == 14 && data > B11111) {
      event = ((data & B11100000) >> 5) | B01000000 ;
      //return event;
    }
    else if (c == 15 && data > 0) {
      event = (((data & B0011) << 3) | B01000000);
      //return event;
    }

    c++;
  }
  
  Serial.println(output);
  return event;
}



String Hover::getEventString(byte eventByte) {

  //Serial.println("inside string fcn");
  //return "Test";
  //Serial.println(eventByte);
    if (eventByte == B00100010) {
        //Serial.println("Right swipe");
    return "Right Swipe";
    } else if (eventByte == B00100100) {
        //Serial.println("Left swipe"); 
    return "Left Swipe";

    } else if (eventByte == B00101000) {
        //Serial.println("Up swipe");  
    return "Up Swipe";
    
    } else if (eventByte == B00110000) {
        //Serial.println("Down swipe"); 
    return "Down Swipe";
    
    } else if (eventByte == B01000001) {
        //Serial.println("Tap south");
    return "Tap South";
    
    } else if (eventByte == B01000010) {
        //Serial.println("Tap West");
    return "Tap West";
    
    } else if (eventByte == B01010000) {
        //Serial.println("Tap Center");
    return "Tap Center";
    
    } else if (eventByte == B01001000) {
        //Serial.println("Tap East"); 
    return "Tap East";
    
    } else if (eventByte == B01000100) {
        //Serial.println("Tap NORTH");     
    return "Tap North";
    
    } 
  return "";
}
