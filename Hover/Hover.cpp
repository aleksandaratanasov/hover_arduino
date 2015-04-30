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


#include <StringBuilder.h>

volatile Hover* Hover::INSTANCE = NULL;



void hover_ts_irq() {
  ((Hover*) Hover::INSTANCE)->set_isr_mark(MGC3130_ISR_MARKER_TS);
}


void gest_0() {
  ((Hover*) Hover::INSTANCE)->set_isr_mark(MGC3130_ISR_MARKER_G0);
}


void gest_1() {
  ((Hover*) Hover::INSTANCE)->set_isr_mark(MGC3130_ISR_MARKER_G1);
}


void gest_2() {
  ((Hover*) Hover::INSTANCE)->set_isr_mark(MGC3130_ISR_MARKER_G2);
}


void gest_3() {
  ((Hover*) Hover::INSTANCE)->set_isr_mark(MGC3130_ISR_MARKER_G3);
}



void Hover::set_isr_mark(uint8_t mask) {
  service_flags |= mask;
  if (MGC3130_ISR_MARKER_TS == mask) {
	pinMode(_ts_pin, OUTPUT);
	digitalWrite(_ts_pin, 0);
  }
}



Hover::Hover(int ts, int rst, uint8_t addr) {
  _i2caddr   = addr;
  _ts_pin    = (uint8_t) ts;
  _reset_pin = (uint8_t) rst;

  service_flags = 0x00;
  class_state = 0x00;
  
  _irq_pin_0 = 0;
  _irq_pin_1 = 0;
  _irq_pin_2 = 0;
  _irq_pin_3 = 0;
  
  _pos_x = -1;
  _pos_y = -1;
  _pos_z = -1;
  wheel_position = 0;
  last_swipe = 0;
  last_tap = 0;
  
  events_received = 0;
  last_event = 0;
  INSTANCE = this;
}


#if defined(BOARD_IRQS_AND_PINS_DISTINCT)
/*
* If you are dealing with a board that has pins/IRQs that don't match, case it off
* in the fxn below.
* A return value of -1 means invalid IRQ/unhandled case.
*/
int Hover::get_irq_num_by_pin(int _pin) {
  #if defined(_BOARD_FUBARINO_MINI_)    // Fubarino has goofy IRQ/Pin relationships.
	switch (_pin) {
	  case PIN_INT0: return 0;
	  case PIN_INT1: return 1;
	  case PIN_INT2: return 2;
	  case PIN_INT3: return 3;
	  case PIN_INT4: return 4;
	}
  #endif
  return -1;
}
#endif


void Hover::begin() {
  pinMode(_ts_pin, INPUT);     //Used by TS line on MGC3130
  pinMode(_reset_pin, OUTPUT);   //Used by reset line on MGC3130
  digitalWrite(_reset_pin, 0);
  
  #if defined(BOARD_IRQS_AND_PINS_DISTINCT)
	int fubar_irq_number = get_irq_num_by_pin(_ts_pin);
	if (fubar_irq_number >= 0) {
		attachInterrupt(fubar_irq_number, hover_ts_irq, FALLING);
	}
  #else
	attachInterrupt(_ts_pin, hover_ts_irq, FALLING);
  #endif
  
  if (_irq_pin_0) {
	pinMode(_irq_pin_0, INPUT);
	#if defined(BOARD_IRQS_AND_PINS_DISTINCT)
	  int fubar_irq_number = get_irq_num_by_pin(_irq_pin_0);
	  if (fubar_irq_number >= 0) {
		attachInterrupt(fubar_irq_number, gest_0, FALLING);
	  }
	#else
	  attachInterrupt(_irq_pin_0, gest_0, FALLING);
	#endif
  }
  
  if (_irq_pin_1) {
	pinMode(_irq_pin_1, INPUT);
	#if defined(BOARD_IRQS_AND_PINS_DISTINCT)
	  int fubar_irq_number = get_irq_num_by_pin(_irq_pin_1);
	  if (fubar_irq_number >= 0) {
		attachInterrupt(fubar_irq_number, gest_1, FALLING);
	  }
	#else
	  attachInterrupt(_irq_pin_1, gest_1, FALLING);
	#endif
  }
  
  if (_irq_pin_2) {
	pinMode(_irq_pin_2, INPUT);
	#if defined(BOARD_IRQS_AND_PINS_DISTINCT)
	  int fubar_irq_number = get_irq_num_by_pin(_irq_pin_2);
	  if (fubar_irq_number >= 0) {
		attachInterrupt(fubar_irq_number, gest_2, FALLING);
	  }
	#else
	  attachInterrupt(_irq_pin_2, gest_2, FALLING);
	#endif
  }
  
  if (_irq_pin_3) {
	pinMode(_irq_pin_3, INPUT);
	#if defined(BOARD_IRQS_AND_PINS_DISTINCT)
	  int fubar_irq_number = get_irq_num_by_pin(_irq_pin_3);
	  if (fubar_irq_number >= 0) {
		attachInterrupt(fubar_irq_number, gest_3, FALLING);
	  }
	#else
	  attachInterrupt(_irq_pin_3, gest_3, FALLING);
	#endif
  }

  delay(400);
  digitalWrite(_reset_pin, 1);
  class_state = 0x01;
}



// Bleh... needs rework.
int8_t Hover::setIRQPin(uint8_t _mask, int pin) {
  switch(_mask) {
	case MGC3130_ISR_MARKER_G0:
	  _irq_pin_0 = pin;
	  break;
	  
	case MGC3130_ISR_MARKER_G1:
	  _irq_pin_1 = pin;
	  break;
	  
	case MGC3130_ISR_MARKER_G2:
	  _irq_pin_2 = pin;
	  break;
	  
	case MGC3130_ISR_MARKER_G3:
	  _irq_pin_3 = pin;
	  break;
	  
	default:
	  return -1;
  }

  return 0;
}



int8_t Hover::service() {
  int8_t return_value = 0;
  if (service_flags) {
	if (service_flags & MGC3130_ISR_MARKER_TS) {
	  service_flags &= ~((uint8_t) MGC3130_ISR_MARKER_TS);

	  if (getEvent()) {
		return_value++;
	  }

      digitalWrite(_ts_pin, 1);
      pinMode(_ts_pin, INPUT);
	}
	else if (service_flags & MGC3130_ISR_MARKER_G0) {
	  service_flags &= ~((uint8_t) MGC3130_ISR_MARKER_G0);
	  Serial.println("G0");
	  return_value = 1;
	}
	else if (service_flags & MGC3130_ISR_MARKER_G1) {
	  service_flags &= ~((uint8_t) MGC3130_ISR_MARKER_G1);
	  Serial.println("G1");
	  return_value = 1;
	}
	else if (service_flags & MGC3130_ISR_MARKER_G2) {
	  service_flags &= ~((uint8_t) MGC3130_ISR_MARKER_G2);
	  Serial.println("G2");
	  return_value = 1;
	}
	else if (service_flags & MGC3130_ISR_MARKER_G3) {
	  service_flags &= ~((uint8_t) MGC3130_ISR_MARKER_G3);
	  Serial.println("G3");
	  return_value = 1;
	}
	else {
	  service_flags = 0;;
	}
  }  
  return return_value;
}


/**
* 
*/
uint8_t Hover::getEvent(void) {
  byte data;
  int c = 0;
  events_received++;
  uint16_t data_set = 0;
  uint8_t return_value = 0;
  
  bool pos_valid = false;
  bool wheel_valid = false;
  
  // Pick some safe number. We might limit this depending on what the sensor has to say.
  uint8_t bytes_expected = 32;  
  Wire.requestFrom((uint8_t)_i2caddr, (uint8_t) bytes_expected);    // request 26 bytes from slave device at 0x42

  while(Wire.available() || (0 == bytes_expected--)) {     
    #if defined(_BOARD_FUBARINO_MINI_)    // Fubarino
	  data = Wire.receive(); // receive a byte as character
	#else
	  data = Wire.read(); // receive a byte as character
	#endif

    switch (c++) {
      case 0:   // Length of the transfer by the sensor's reckoning.
        if (bytes_expected < (data-1)) {
          bytes_expected = data - 1;  // Minus 1 because: we have already read one.
        }
        break;
      case 1:   // Flags.
		last_event = (B00000001 << (data-1)) | B00100000;
        break;
      case 2:   // Sequence number
        break;
      case 3:   // Unique ID
		// data ought to always be 0x91 at this point.
        break;
      case 4:   // Data output config mask is a 16-bit value.
        data_set = data;
        break;
      case 5:   // Data output config mask is a 16-bit value.
        data_set += data << 8;
        break;
      case 6:   // Timestamp (by the sensor's clock). Mostly useless unless we are paranoid about sample drop.
        break;
      case 7:   // System info. Tells us what data is valid.
        pos_valid   = (data & 0x01);  // Position
		wheel_valid = (data & 0x02);  // Air wheel
        break;
      
      /* Below this point, we enter the "variable-length" area. God speed.... */
      case 8:   //  DSP info
      case 9: 
        break;
      case 10:  // GestureInfo
		if (data > 1) {
		  last_swipe = (B00000001 << (data-1)) | B00100000;
		  return_value++;
		}
        break;
      case 11: 
      case 12: 
      case 13: 
        break;
      case 14:  // TouchInfo
		if (data > B11111) {
		  last_tap = ((data & B11100000) >> 5) | B01000000;
		  return_value++;
		}
        break;
      case 15:
		if (data > 0) {
		  last_tap = (((data & B0011) << 3) | B01000000);
		  return_value++;
		}
        break;
      case 16: 
      case 17: 
        break;
      case 18:  // AirWheelInfo 
		if (wheel_valid) {
		  wheel_position += wheel_valid;
		  return_value++;
		}
        break;
      case 19:  // AirWheelInfo, but the MSB is reserved.
        break;

      case 20:  // Position. This is a vector of 3 uint16's, but we store as 32-bit.
        if (pos_valid) _pos_x = data;
        break;
      case 21:
        if (pos_valid) _pos_x += data << 8;
        break;
      case 22: 
        if (pos_valid) _pos_y = data;
        break;
      case 23: 
        if (pos_valid) _pos_y += data << 8;
        break;
      case 24: 
        if (pos_valid) _pos_z = data;
        break;
      case 25:
        if (pos_valid) _pos_z += data << 8;
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
  }
  
  if (pos_valid) return_value++;
  
  return return_value;
}


void Hover::markClean() {
  last_tap   = 0;
  last_swipe = 0;
  last_event = 0;
  _pos_x = -1;
  _pos_y = -1;
  _pos_z = -1;
  wheel_position = 0;
}


void Hover::printBrief(StringBuilder* output) {
  if (last_tap) {
	output->concat(getEventString(last_tap));
  }
  else if (last_swipe) {
	output->concat(getEventString(last_swipe));
  }
  else if ((_pos_x >= 0) || (_pos_y >= 0) || (_pos_z >= 0)) {
	output->concatf("(%d, %d, %d)\n", _pos_x, _pos_y, _pos_z);
  }
}


const char* Hover::getEventString(uint8_t eventByte) {
  switch (eventByte) {
	case B00100010:	return "Right Swipe";
	case B00100100:	return "Left Swipe";
	case B00101000:	return "Up Swipe";
	case B00110000:	return "Down Swipe";
	case B01000001:	return "Tap South";
	case B01000010:	return "Tap West";
	case B01000100:	return "Tap North";
	case B01001000:	return "Tap East";
	case B01010000:	return "Tap Center";
  } 
  return "";
}


void Hover::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  output->concatf("--- MGC3130\n----------------------------------\n-- Last position: (%d, %d, %d)\n", _pos_x, _pos_y, _pos_z);
  output->concatf("-- Last swipe: %s\n-- Last tap: %s\n\n", getEventString(last_swipe), getEventString(last_tap));
  output->concatf("-- Airwheel:   0x%08x\n", wheel_position);
}

