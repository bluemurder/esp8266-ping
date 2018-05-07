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

#ifndef ESP8266_Ping_Arduino_Library
#define ESP8266_Ping_Arduino_Library

#include <Arduino.h>
#include <ESP8266WiFi.h>

extern "C"
{
  #include <ping.h>
  void esp_schedule();
  void esp_yield();
}

class Pinger
{
public:
  // Constructor
  Pinger();
  
  // Destructor
  virtual ~Pinger();

  // Returns the response time in milliseconds, -1 if error
  int Ping(IPAddress ip);

  // Returns the response time in milliseconds, -1 if error
  int Ping(const char * hostname);
  
  // Get last ping statistics
  ping_resp GetLastPingResponse();

protected:
  // Response time (-1 if error)
  static int m_responseTime;
  
  // Ping response
  static ping_resp m_pingResponse;

  // Received ping response callback
  static void ReceivedResponseCallback(void * options, void * response);
};

#endif // ESP8266_Ping_Arduino_Library