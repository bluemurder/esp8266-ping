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

#include <Pinger.h>
#include <ESP8266WiFi.h>

void setup()
{  
  // Begin serial connection at 9600 baud
  Serial.begin(9600);
  
  // Connect to WiFi
  bool stationConnected = WiFi.begin(
  "GuaglioWifi",
  "testtesttest");

  if(!stationConnected)
  {
    Serial.println("Error, unable to connect specified WiFi network.");
  }
} 

void loop()
{
  Pinger pinger;

  // Ping Google DNS
  int responseTime = pinger.Ping(IPAddress(8, 8, 8, 8));

  // Print it
  if(responseTime == -1)
  {
    Serial.println("Error pinging IP 8.8.8.8");
  }
  else
  {
    Serial.print("Echo response received from 8.8.8.8 in ");
    Serial.print(responseTime);
    Serial.print("ms.\n");
  }

  // Wait a while
  delay(1000);
}