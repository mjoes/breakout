#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE(); 
// Pins
int pin_startstop_button = 2;
int pin_clk_4 = 4;
int pin_clk_6 = 5;
int pin_clk_8 = 6;
int pin_clk_12 = 7;
int pin_clk_16 = 8;
int pin_bpm_input = A0;
int pin_rotary = A1;

// General variables
int clock_pulse = 0;
const int numBeats = 5;
const int beatPins[numBeats] = {pin_clk_4, pin_clk_6, pin_clk_8, pin_clk_12, pin_clk_16};
const int beatValues[numBeats] = {24, 18, 12, 8, 6};
unsigned long previousMillis = 0;
boolean isClockRunning = false;

// User BPM
const int min_bpm = 60;
const int max_bpm = 100;
int steps_bpm = 1024 / (max_bpm - min_bpm);


// Ableton
int play_flag = 0;
byte midi_start = 0xfa;
byte midi_stop = 0xfc;
byte midi_clock = 0xf8;
byte midi_continue = 0xfb;
byte data;

// Switch
int switchState = 1;    
int lastSwitchState = 1; 
unsigned long lastDebounceTime = 0;  
unsigned long debounceDelay = 50;


void setup() {
  pinMode(pin_clk_4, OUTPUT);
  pinMode(pin_clk_6, OUTPUT);
  pinMode(pin_clk_8, OUTPUT);
  pinMode(pin_clk_12, OUTPUT);
  pinMode(pin_clk_16, OUTPUT);
  pinMode(pin_startstop_button, INPUT_PULLUP);

  MIDI.begin(1);
  MIDI.turnThruOff();
  // Serial.begin(31250);
  delay(50);
}

void loop() {
  int reading = analogRead(pin_rotary);
  MIDI.setInputChannel(1);

  if (reading < 256) {
    switchState = 1;
  }
  else if (reading < 768) {
    switchState = 2;
  }
  else {
    switchState = 3;
  }

  if (switchState != lastSwitchState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (switchState != lastSwitchState) {
      clock_pulse = 0;  
      lastSwitchState = switchState;
    }
    
    switch (lastSwitchState) {
      case 1:
        externalBPM();
        break;

      case 2:
        abletonBPM();
        break;

      case 3:
        userBPM();
        break;
    }
  }
}

void writePulse(int clock_pulse) {
  for (int i = 0; i < numBeats; ++i) {
    digitalWrite(beatPins[i], clock_pulse % beatValues[i] == 0 ? HIGH : LOW); // clock divider
  }
  if (clock_pulse == 72) {
    clock_pulse = 0;
  } 
}

void externalBPM() {
  if (MIDI.read()) {               
    
    switch (MIDI.getType()) {
      case midi::Clock:
        clock_pulse ++;

        writePulse(clock_pulse);
        break;

      case midi::Stop:
        clock_pulse = 0;
        break;
    }
  }
}

void abletonBPM() {
  if (MIDI.read()) {               
    
    switch (MIDI.getType()) {
      case midi::Start:
        play_flag = 1;
        clock_pulse = 0;
        break;

      case midi::Continue:
        play_flag = 1;
        break;

      case midi::Stop:
        play_flag = 0;
        clock_pulse = 0;
        break;
        
      case midi::Clock:
        if (play_flag == 1) {
          clock_pulse ++;
          writePulse(clock_pulse);
        }
        break;
    }
  }
  // if(Serial.available() > 0) {
  //   data = Serial.read();

  //   if(data == midi_start) {
  //     play_flag = 1;
  //   }
  //   else if(data == midi_continue) {
  //     play_flag = 1;
  //   }
  //   else if(data == midi_stop) {
  //     play_flag = 0;
  //   }
  //   else if((data == midi_clock) && (play_flag == 1)) {
  //     clock_pulse ++;
  //     writePulse(clock_pulse);
  //   }
  // }
}


void userBPM() {
  int clock_rate = analogRead(pin_bpm_input);
  int bpm = round(min_bpm + (clock_rate * steps_bpm));
  float beatInterval = 60000 / (bpm * 24);

  unsigned long currentMillis = millis();

  if (digitalRead(pin_startstop_button) == LOW) {
    isClockRunning = !isClockRunning;

    if (isClockRunning) {
      previousMillis = currentMillis;
    }
    delay(200); 
  }

  if (isClockRunning && (currentMillis - previousMillis >= beatInterval)) {
    clock_pulse ++;
    previousMillis = currentMillis; 
    writePulse(clock_pulse);
  }
}



