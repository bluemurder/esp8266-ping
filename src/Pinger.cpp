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

#include "Pinger.h"

extern "C"
{
  void esp_schedule();
  void esp_yield();
}

int Pinger::m_responseTime = -1;

//////////////////////////////////////////////////////////////////////////////
// Returns the response time in milliseconds, -1 if error
int Pinger::Ping(IPAddress ip)
{
  // Reset state
  m_responseTime = -1;

  // Build ping_options structure
  ping_option pingOptions;

  // Reset options
  memset(&pingOptions, 0, sizeof(struct ping_option));

  // Seconds between ping messages
  pingOptions.coarse_time = 1000;

  // Number of ping messages
  pingOptions.count = 1;
  
  // Destination address
  pingOptions.ip = ip;
  
  // Callback when response is received
  pingOptions.recv_function = reinterpret_cast<ping_recv_function>(&Pinger::ReceivedResponseCallback);

  // No callback when ping request is sent
  pingOptions.sent_function = nullptr;
  
  // Empty reverse field
  pingOptions.reverse = nullptr;

  // Send ping request and block until response is received
  if(ping_start(&pingOptions))
  {
    esp_yield();
  }

  // Return response time (filled by ReceivedResponseCallback)
  return m_responseTime;
}

//////////////////////////////////////////////////////////////////////////////
// Returns the response time in milliseconds, -1 if error
int Pinger::Ping(const char * hostname)
{
  // Check parameter
  if(hostname == nullptr)
  {
    return -1;
  }

  // Resolve remote host address
  IPAddress ipAddress;
  if(WiFi.hostByName(hostname, ipAddress))
  {
    // Call main Ping routine
    return Ping(ipAddress);
  }
  else
  {
    return -1;
  }
}

//////////////////////////////////////////////////////////////////////////////
// Constructor
Pinger::Pinger()
{
}

//////////////////////////////////////////////////////////////////////////////
// Destructor
Pinger::~Pinger()
{
}

//////////////////////////////////////////////////////////////////////////////
// Received ping response callback
void Pinger::ReceivedResponseCallback(void *, void * response)
{
  m_responseTime = -1;

  // Check response parameter
  if(response != nullptr)
  {

    // Decode response fields
    ping_resp* pingResponse = reinterpret_cast<struct ping_resp*>(response);
    if(pingResponse == nullptr)
    {

      // If errors in response, exit
      if(pingResponse->ping_err != -1)
      {

        // Update response time
        m_responseTime = pingResponse->resp_time;


          /*Serial.printf("DEBUG: ping reply\n"
            "\ttotal_count = %d \n"
            "\tresp_time = %d \n"
            "\tseqno = %d \n"
            "\ttimeout_count = %d \n"
            "\tbytes = %d \n"
            "\ttotal_bytes = %d \n"
            "\ttotal_time = %d \n"
            "\tping_err = %d \n",
            pingResponse->total_count, pingResponse->resp_time, pingResponse->seqno,
            pingResponse->timeout_count, pingResponse->bytes, pingResponse->total_bytes,
            pingResponse->total_time, pingResponse->ping_err);*/
		
      }
    }
  }

  // Return to main caller
  esp_schedule();
}