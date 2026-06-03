// Drivesynth ESP32-based HDD sound emulator using the XTronical DAC Audio Library
// Requires XTronical DAC Audio Library 4.2.1

#include "SoundData.h"
#include "XT_DAC_Audio.h"

XT_Wav_Class Idle(Idlesnd);
XT_Wav_Class Boot(Bootsnd);
XT_Wav_Class HeadTick(HeadTickSnd);
/*
XT_Wav_Class HeadTick(PowerOffSnd);
*/
XT_DAC_Audio_Class DacAudio(25, 0);

const int signalPin = 13;

/* TODO: assign a pin for power supply sensing */
/* const int powerPin = 12; */

const int LOOP_PERIOD=10;
typedef enum state_e
{
  STATE_BOOT,    // System booting, play disk spin up sound
  STATE_IDLE,    // Disk is idle, do nothing: Play Idle sound one time ("RepeatForever" is true). Look for led activity
  STATE_PULSING, // A pulse was sensed by GPIO pin, start counting pulse time
  STATE_TICKING, // Disk Activity detected, play Disk tick sound
  STATE_OFF      // Power was removed, play Disk spin down sound
};

state_e state = STATE_BOOT;

void setup()
{
  pinMode(signalPin, INPUT);
 
  /* pinMode(powerPin, INPUT); */

  Serial.begin(115200);
  Idle.RepeatForever = true;
  delay(200); /* Should be enough for power supply settling */
}

void loop()
{
  static unsigned long startTime = 0;

  int pinstate = digitalRead(signalPin);
  /*
  int powerstate = digitalRead(powerPin);
  if (powerstate == LOW ){
    state = STATE_OFF;
  }
  */
  switch (state)
  {
  case STATE_OFF:
    /*
    DacAudio.Play(&PowerOffSnd,false); // Disc powering down, stop all spund and play ony this one

    // Waiting for the backup power to turn off the system
    while (true)
    {
      DacAudio.FillBuffer();
      delay(LOOP_PERIOD);
    }
    */
    break;

  case STATE_BOOT:
    DacAudio.Play(&Boot); // Disc starts spinning

    // waiting for the disc starting sound to finish
    while (Boot.Playing)
    {
      DacAudio.FillBuffer();
      delay(LOOP_PERIOD);
    }
    state = STATE_IDLE;
    /* 
      A sample change its Playing status when last byte is sent to the buffer, not when last sample is sent to the DAC.
      Currently the buffer holds 80 ms of audio, so when the while loop exits, there could be up to 80 ms of audio
      to be played. Residual data could be less than 80 ms and not always the same.
      Adding another sound will mix it with residual audio samples, so transition to Idle sound should be smooth
    */
    DacAudio.Play(&Idle);
    break;

  case STATE_IDLE:
    if (pinstate == HIGH)
    {
      // Reading Pulse Start from HDD Led header
      state = STATE_PULSING;
      startTime = millis();
    }
    break;

  case STATE_PULSING:
    if (pinstate == LOW)
    {
      // Reading Pulse Stop from HDD Led header
      unsigned long sigpulse = millis() - startTime;
      Serial.println(sigpulse);

      if ((sigpulse <= 100) && ((rand() % 2) + 1 == 2))
      {
        // playing hdd activity sound when pulses comes out from the HDD LED pin header. I added a 50/50 randomicity to simulate file fragmentation
        DacAudio.Play(&HeadTick);
        state = STATE_TICKING;
      }
      else
      {
        state = STATE_IDLE;
      }
    }
    break;

  case STATE_TICKING:
    if (!HeadTick.Playing)
    {
      state = STATE_IDLE;
    }
  }
  DacAudio.FillBuffer();
  delay(LOOP_PERIOD);
}