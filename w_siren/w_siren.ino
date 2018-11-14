
#include <w-blink.h>
#include <w-battery.h>
#include <w-lora.h>



// constants
const uint8_t         APP_ADDR            = 1;

const int             WRN_PIN             = 6;
const int             STATUS_PIN          = 5;
const int             SIREN_PIN           = 10;
const int             TEST_PIN            = 12;

const int             SIREN_TIMEOUT_MS    = 2500;
const int             STATUS_TIMEOUT_MS   = 200;
const int             REPORT_TIMEOUT_MS   = 4000;

const float           VBTY_FULL           = 4.10;    ///< full charge level



// globals
AppState            state;



// updates application state (read voltages, etc.)
void updateAppState()
{
  state.m_uAppMillis = millis();
  state.m_fVbty = readBatteryVoltage();
  state.m_fVcc = readSupplyVoltage();
  state.m_bPowerOn = state.m_fVcc > state.m_fVbty + 0.2;

  const float fVbtyFull = state.m_bCharging ? VBTY_FULL : (VBTY_FULL-0.02);
  state.m_bCharging = (state.m_bPowerOn == true) && (state.m_fVbty < fVbtyFull);
}


// report app state
void reportAppState()
{
  static int uReportTimeoutMs = REPORT_TIMEOUT_MS;

  if (millis() > state.m_uReportMillis + uReportTimeoutMs)
  {
    state.m_uReportMillis = millis();
    uReportTimeoutMs = REPORT_TIMEOUT_MS + random(REPORT_TIMEOUT_MS/2);

    // report app state to HQ
    sendRadioMsg(state, APP_ADDR, RFADDR_HQ);

    // debug
    if (Serial)
    {
      Serial.print("state: ");
      Serial.print("addr=");
      Serial.print(APP_ADDR, HEX);
      Serial.print(", chr=");
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


// test siren output and write siren event to radio
void testSiren()
{
  // set siren output
  tickOutput(SIREN_TIMEOUT_MS, SIREN_PIN, true, true);

  // send siren test event to HQ
  SirenEvent evt(true, 2500);
  sendRadioMsg(evt, APP_ADDR, RFADDR_HQ);
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
  if (n > 0)
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
    if ( (uDstAddr == APP_ADDR) || (uDstAddr == RFADDR_BRDCST) )
    {
      if (checkRadioMsgType<SirenEvent>(n) == true)
      {
        SirenEvent evt;
        if (getRadioMsg(evt, n) == true)
        {
          // set siren output
          tickOutput(SIREN_TIMEOUT_MS, SIREN_PIN, evt.m_bSirenOn, true);
          
          if (Serial)
          {
            Serial.print("radio: got sirent event (");
            Serial.print(evt.m_bSirenOn);
            Serial.print(", ");
            Serial.print(evt.m_uTimeoutMs);
            Serial.println(")");
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
  pinMode(TEST_PIN, INPUT);  
  digitalWrite(TEST_PIN, LOW);
  
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
  if (digitalRead(TEST_PIN) == HIGH)
  {
    testSiren();
  }
  
  readSerial();
  readLora();

  // report app state
  reportAppState();
  
  // maintain output timers
  tickOutput(0, SIREN_PIN, false, true);
  tickOutput(0, STATUS_PIN, false, false);
}



