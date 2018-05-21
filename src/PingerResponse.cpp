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

#include "PingerResponse.h"

//////////////////////////////////////////////////////////////////////////////
// Constructor
PingerResponse::PingerResponse()
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////////
// Destructor
PingerResponse::~PingerResponse()
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////////
// Reset class
void PingerResponse::Reset()
{
  ResponseTime = 0;
  MaxResponseTime = 0;
  MinResponseTime = 0xffffffff;
  AvgResponseTime = 0.f;
  DestIPAddress = IPAddress(0, 0, 0, 0);
  DestMacAddress = nullptr;
  DestHostname = "";
  EchoMessageSize = 0;
  SequenceNumber = 0;
  ReceivedResponse = false;
  TimeToLive = 0;
  TotalSentRequests = 0;
  TotalReceivedResponses = 0;
  TotalPingingTime = 0;
  EchoRequestTimeout = 0;
}