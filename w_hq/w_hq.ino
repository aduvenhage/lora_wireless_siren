
#include <w-blink.h>
#include <w-battery.h>
#include <w-lora.h>



// constants
const int             WRN_PIN             = 6;
const int             STATUS_PIN          = 5;
const int             SIREN_PIN           = 10;
const int             TEST1_PIN           = 12;
const int             TEST2_PIN           = 11;

const int             SIREN_TIMEOUT_MS    = 2500;
const int             STATUS_TIMEOUT_MS   = 200;
const int             REPORT_TIMEOUT_MS   = 20000;

const int             MAX_DEVICES         = 32;



// globals
AppState            state;
AppState            devices[MAX_DEVICES];



// updates application state (read voltages, etc.)
void updateAppState()
{
  state.m_uAppMillis = millis();
  state.m_fVbty = readBatteryVoltage();
  state.m_fVcc = readSupplyVoltage();
  state.m_bPowerOn = state.m_fVcc > state.m_fVbty + 0.2;

  if (state.m_bCharging == false)
  {
    state.m_bCharging = (state.m_fVcc > state.m_fVbty + 0.8);
  }
  else if (state.m_bCharging == true)
  {
    state.m_bCharging = (state.m_fVcc > state.m_fVbty + 0.2);
  }
}


// report app state
void reportAppState()
{
  if (millis() > state.m_uReportMillis + REPORT_TIMEOUT_MS)
  {
    state.m_uReportMillis = millis();

    // debug
    if (Serial)
    {
      Serial.print("state: ");
      Serial.print("chr=");
      Serial.print(state.m_bCharging ? "yes" : "no");
      Serial.print(", pwr=");
      Serial.print(state.m_bPowerOn ? "yes" : "no");
      Serial.print(", bty=");
      Serial.print(state.m_fVbty);
      Serial.print(", vcc=");
      Serial.print(state.m_fVcc);
      Serial.println();
    }
  }
}


// test siren output
void testSiren()
{
  tickOutput(SIREN_TIMEOUT_MS, SIREN_PIN, true, true);
}


// send out siren event
void sendSirenOn(uint8_t _uDstAddr, unsigned long _uTimeoutMs)
{
  // send siren test event to HQ
  SirenEvent evt(true, _uTimeoutMs);
  sendRadioMsg(evt, RFADDR_HQ, _uDstAddr);
}


// read from serial port and also output to lora
void readSerial()
{
  if ( (Serial) &&
       (Serial.available()) )
  {
    int i = Serial.read();
    
    // test siren
    if (i == 's')
    {
      testSiren();
      
      // flash status LED
      tickOutput(STATUS_TIMEOUT_MS, STATUS_PIN, true, false);
    }

    // debug
    Serial.write(i);
  }
}


// read from lora
void readLora()
{
  uint8_t uSrcAddr = 0;
  uint8_t uDstAddr = 0;

  uint8_t n = recvRadioMsg(uSrcAddr, uDstAddr);
  if ( (uSrcAddr < MAX_DEVICES) &&
       (n > 0) )
  {
    // flash status LED
    tickOutput(STATUS_TIMEOUT_MS, STATUS_PIN, true, false);

    // debug
    if (Serial)
    {
      Serial.print("radio: got msg from 0x");
      Serial.print(uSrcAddr, HEX);
      Serial.print(" to 0x");
      Serial.print(uDstAddr, HEX);
      Serial.print(", size ");
      Serial.println(n);
    }
    
    // check for messages
    if ( (uDstAddr == RFADDR_HQ) || (uDstAddr == RFADDR_BRDCST) )
    {
      if (checkRadioMsgType<AppState>(n) == true)
      {
        auto &device = devices[uSrcAddr];
        bool bSuccess = getRadioMsg(device, n);
        if (bSuccess == true)
        {
          // update report timestamp to HQ time
          device.m_uReportMillis = millis();
          
          // debug
          if (Serial)
          {
            Serial.print("device: ");
            Serial.print("addr=");
            Serial.print(device.m_uSrcAddr);
            Serial.print(", chr=");
            Serial.print(device.m_bCharging ? "yes" : "no");
            Serial.print(", pwr=");
            Serial.print(device.m_bPowerOn ? "yes" : "no");
            Serial.print(", bty=");
            Serial.print(device.m_fVbty);
            Serial.print(", vcc=");
            Serial.print(device.m_fVcc);
            Serial.println();
          }
        }
      }
    }
  }
  
}


void setup()
{
  // setup output pins
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  pinMode(SIREN_PIN, OUTPUT);  
  digitalWrite(SIREN_PIN, HIGH);
  
  pinMode(STATUS_PIN, OUTPUT);
  digitalWrite(STATUS_PIN, LOW);
  
  pinMode(WRN_PIN, OUTPUT);
  digitalWrite(WRN_PIN, LOW);

  // setup input pins
  pinMode(TEST1_PIN, INPUT);  
  digitalWrite(TEST1_PIN, LOW);
  
  pinMode(TEST2_PIN, INPUT);  
  digitalWrite(TEST2_PIN, LOW);
  
  // setup serial
  Serial.begin(19200);
  
  // setup LoRa
  setupRadio();
}


void loop()
{
  // debug
  blink(500, 13);
  
  // read battery voltages, etc.
  updateAppState();

  // update WRN LED
  if (state.m_bPowerOn == false)
  {
    digitalWrite(WRN_PIN, HIGH);
  }
  else if (state.m_bCharging == true)
  {
    blink(500, WRN_PIN);
  }
  else
  {
    digitalWrite(WRN_PIN, LOW);
  }

  // read external inputs
  if (digitalRead(TEST1_PIN) == HIGH)
  {
    sendSirenOn(1, 2500);
  }
  
  if (digitalRead(TEST2_PIN) == HIGH)
  {
    sendSirenOn(2, 2500);
  }
  
  readSerial();
  readLora();

  // report app state
  reportAppState();
  
  // maintain output timers
  tickOutput(0, SIREN_PIN, false, true);
  tickOutput(0, STATUS_PIN, false, false);
}



