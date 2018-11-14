
#include <RH_RF95.h>
#include <SPI.h>


// constants
const int             RFM95_CS            = 8;
const int             RFM95_RST           = 4;
const int             RFM95_INT           = 7;
const float           RF95_FREQ           = 915.0;

const uint8_t         RFPROTO_ID          = 42;       //< unique protocol ID

const uint8_t         RFADDR_HQ           = 0;       //< gateway address
const uint8_t         RFADDR_BRDCST       = 255;     //< broadcast address



#pragma pack(push)
#pragma pack(1)
struct AppState
{
  AppState()
    :m_uProtoId(RFPROTO_ID),
     m_uSrcAddr(0),
     m_uDstAddr(0),
     m_uAppMillis(0),
     m_uReportMillis(0),
     m_fVbty(0),
     m_fVcc(0),
     m_bCharging(false),
     m_bPowerOn(false)
  {}

  uint8_t         m_uProtoId;
  uint8_t         m_uSrcAddr;
  uint8_t         m_uDstAddr;
  unsigned long   m_uAppMillis;
  unsigned long   m_uReportMillis;
  float           m_fVbty;
  float           m_fVcc;
  bool            m_bCharging;
  bool            m_bPowerOn;
};
#pragma pack(pop)


#pragma pack(push)
#pragma pack(1)
struct SirenEvent
{
  SirenEvent()
    :m_uProtoId(RFPROTO_ID),
     m_uSrcAddr(0),
     m_uDstAddr(0),
     m_uTimeoutMs(0),
     m_bSirenOn(false)
     
  {}

  SirenEvent(bool _bSirenOn, unsigned long _uTimeoutMs)
    :m_uProtoId(RFPROTO_ID),
     m_uSrcAddr(0),
     m_uDstAddr(0),
     m_uTimeoutMs(_uTimeoutMs),
     m_bSirenOn(_bSirenOn)
     
  {}

  uint8_t         m_uProtoId;
  uint8_t         m_uSrcAddr;
  uint8_t         m_uDstAddr;
  unsigned long   m_uTimeoutMs;
  bool            m_bSirenOn;
};
#pragma pack(pop)



// globals
RH_RF95             radio(RFM95_CS, RFM95_INT);
uint8_t             radioBuffer[RH_RF95_MAX_MESSAGE_LEN];



// initialise radio
void setupRadio()
{
  // setup LoRa
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(500);

  // test LoRa
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!radio.init());
  
  // setup LoRa (setup freq and power)
  radio.setFrequency(RF95_FREQ);
  radio.setTxPower(23, false);
}


template <typename msg_type>
void sendRadioMsg(msg_type &_msg, uint8_t _uSrcAddr, uint8_t _uDstAddr)
{
  _msg.m_uSrcAddr = _uSrcAddr;
  _msg.m_uDstAddr = _uDstAddr;
  
  radio.send((uint8_t*)&_msg, sizeof(_msg));
}


uint8_t recvRadioMsg(uint8_t &_uSrcAddr, uint8_t &_uDstAddr)
{
  uint8_t n = 0;

  if (radio.available() == true)
  {
    n = RH_RF95_MAX_MESSAGE_LEN;
    bool bSuccess = radio.recv(radioBuffer, &n);
    if (bSuccess == true)
    {
      uint8_t uProtoId = radioBuffer[0];
      _uSrcAddr = radioBuffer[1]; 
      _uDstAddr = radioBuffer[2]; 

      if (uProtoId != RFPROTO_ID)
      {
        n = 0;
      }
    }
  }
      
  return n;
}


template <typename msg_type>
bool checkRadioMsgType(uint8_t _uSize)
{
  return (sizeof(msg_type) == _uSize) &&
         (radioBuffer[0] == RFPROTO_ID);
}


template <typename msg_type>
bool getRadioMsg(msg_type &_msg, uint8_t _uSize)
{
  if (checkRadioMsgType<msg_type>(_uSize) == true)
  {
    memcpy(&_msg, radioBuffer, sizeof(_msg));
    return true;
  }
  else
  {
    return false;
  }
}


