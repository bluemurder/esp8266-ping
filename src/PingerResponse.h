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

#ifndef ESP8266_PingerResponse_Arduino_Library
#define ESP8266_PingerResponse_Arduino_Library

#include "IPAddress.h"

extern "C"
{
  #include <netif/etharp.h> // required for eth_addr
}

class PingerResponse
{
public:
  // Constructor
  PingerResponse();

  // Destructor
  virtual ~PingerResponse();

  // Reset class
  void Reset();

  // Response time in millseconds
  u32_t ResponseTime;
  
  // Maximum response time in millseconds
  u32_t MaxResponseTime;
  
  // Minimum response time in millseconds
  u32_t MinResponseTime;

  // Average response time in milliseconds
  float AvgResponseTime;

  // Destination IP Address
  IPAddress DestIPAddress;

  // Destination MAC address
  struct eth_addr * DestMacAddress;

  // Destination hostname
  String DestHostname;

  // Echo message size (icmp echo header and data payload)
  u16_t EchoMessageSize;

  // Sequence number
  u32_t SequenceNumber;

  // Ping result (false if timeout occurred)
  bool ReceivedResponse;

  // Time to live
  u16_t TimeToLive;

  // Total sent requests
  u32_t TotalSentRequests;

  // Total received responses 
  u32_t TotalReceivedResponses;

  // Total time spent to handling echo messages
  u32_t TotalPingingTime;

  // Timeout in milliseconds
  u32_t EchoRequestTimeout;
};

#endif // ESP8266_PingerResponse_Arduino_Library