/*****************************************************************************
Arduino library handling ping messages for the esp8266 platform

MIT License

Copyright (c) 2018 Alessio Leoncini

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*****************************************************************************/

#ifndef ESP8266_Pinger_Arduino_Library
#define ESP8266_Pinger_Arduino_Library

#include <functional>
#include "core_version.h"
#include "PingerResponse.h"

typedef std::function<bool (const PingerResponse &)>PingerCallback;

extern "C"
{
  #include <lwip/raw.h>
  #include <user_interface.h>
}

class Pinger
{
public:
  // Constructor
  Pinger();

  // Destructor
  virtual ~Pinger();

  // Set callback to run everytime a ping response is received
  void OnReceive(PingerCallback callback);

  // Set callback to run when a group of ping requests is run
  void OnEnd(PingerCallback callback);

  // Ping an IP address a number of times, with specified timeout.
  // Return false if an error occurs
  bool Ping(IPAddress ip, u32_t requests = 4, u32_t timeout = 1000);

  // Ping a hostname a number of times, with specified timeout.
  // Return false if an error occurs
  bool Ping(const String & hostname, u32_t requests = 4, u32_t timeout = 1000);

  // Sets the ID of echo request packets. Useful to filter echo responses
  // when multiple istances of present class are used.
  void SetPacketsId(u16_t id);

  // Gets the ID used to mark every echo request packet.
  u16_t GetPacketsId();

  // Sets echo payload length, in bytes
  void SetEchoPayloadLength(u16_t len);

  // Gets echo payload length, in bytes
  u16_t SetEchoPayloadLength();

  // Stops the Stops the specified ping sequence.
  void StopPingSequence();

protected:
  // LWIP callback run when a ping response is received (static wrapper)
  static u8_t PingReceivedStatic(
    void * pinger,
    raw_pcb * pcb,
    pbuf * packetBuffer,
    const ip_addr_t * addr);

  // LWIP callback run when a ping response is received
  u8_t PingReceived(pbuf * packetBuffer, const ip_addr_t * addr);

  // Timer callback run when an Echo request timeout event occurs (static wrapper)
  static void TimeoutCallback(void * pinger);

  // Timer callback run when an Echo request timeout event occurs
  void RequestTimeoutOccurred();

  // Timer callback run when an Echo response is received
  static void ReceivedResponseCallback(void * pinger);

  // Compose echo request packet and sends it
  void BuildAndSendPacket();

  // De-register protocol control block from LWIP
  void ClearPcb();

  // User defined callback to execute when an echo response is received
  PingerCallback m_onReceive;

  // User defined callback to execute when ping sequence ends
  PingerCallback m_onEnd;

  // Structure containing destination data and ping sequence statistics
  PingerResponse m_pingResponse;

  // Counter for echo requests to send in a ping sequence
  u32_t m_requestsToSend;
  
  // Last echo request timestamp, used to evaluate echo round trip time
  u32_t m_requestTimestamp;

  // Ping sequence beginning timestamp
  u32_t m_firstRequestTimestamp;

  // Value written in ICMP id field
  u16_t m_packetId;

  // Size of the data paylod to use in echo requests. This not includes 
  // IP header nor ICMP header
  u16_t m_echoPayloadLen;

  // Timer used to check echo requests timeout
  os_timer_t m_requestTimeoutTimer;

  // Fake timer used to run the user defined OnReceive callback asynchronously
  os_timer_t m_fakeTimer;

  // Protocol control block structure passsed to LWIP stack, used to 
  // intercept icmp packets
  struct raw_pcb * m_IcmpProtocolControlBlock;
};

#endif // ESP8266_Pinger_Arduino_Library