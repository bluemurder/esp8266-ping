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

#include <Esp.h>
#include "Pinger.h"
#include "ESP8266WiFi.h" // needed for WiFi.hostByName()

extern "C"
{
  #include <lwip/icmp.h> // needed for icmp packet definitions
  #include <lwip/inet_chksum.h> // needed for inet_chksum()
  #include <lwip/sys.h> // needed for sys_now()
}

//////////////////////////////////////////////////////////////////////////////
// Constructor
Pinger::Pinger()
{
  // Default icmp echo id field for packets generated here
  m_packetId = 0xDEAD;

  // Null pointer to enable safe memory usage
  m_IcmpProtocolControlBlock = nullptr;

  // Empty user defined callback references
  m_onReceive = nullptr;
  m_onEnd = nullptr;

  // Zero echo requests for now
  m_requestsToSend = 0;

  // A valid size of an icmp echo request can be 40 bytes: 8 bytes for the 
  // icmp echo header and 32 data bytes.
  m_echoPayloadLen = 32;
}

//////////////////////////////////////////////////////////////////////////////
// Destructor
Pinger::~Pinger()
{
  ClearPcb();
}

//////////////////////////////////////////////////////////////////////////////
// Set callback to run everytime a ping response is received
void Pinger::OnReceive(PingerCallback callback)
{
  m_onReceive = callback;
}

//////////////////////////////////////////////////////////////////////////////
// Set callback to run when a group of ping requests is run
void Pinger::OnEnd(PingerCallback callback)
{
  m_onEnd = callback;
}

//////////////////////////////////////////////////////////////////////////////
// Ping an IP address a number of times, with specified timeout.
// Return false if an error occurs
bool Pinger::Ping(IPAddress ip, u32_t requests, u32_t timeout)
{
  // If zero packets to send or countdown not expired yet, exit
  if(requests == 0 || m_requestsToSend != 0)
  {
    return false;
  }

  // If never assigned yet, create protocol control block data
  // and assign callback to execute when ICMP packet received
  if(m_IcmpProtocolControlBlock == nullptr)
  {
    // Create new ICMP detection data
    m_IcmpProtocolControlBlock = raw_new(IP_PROTO_ICMP);
    if(m_IcmpProtocolControlBlock == nullptr)
    {
      return false;
    }

    // When LWIP detects a packet corresponding to specified protocol control
    // block, the PingReceivedStatic callback is executed
    raw_recv(
      m_IcmpProtocolControlBlock,
      PingReceivedStatic,
      (void *)this);

    // Selects the local interfaces where detection will be made.
    // In this case, all local interfaces
    raw_bind(m_IcmpProtocolControlBlock, IP_ADDR_ANY);
  }

  // Reset response
  m_pingResponse.Reset();

  // Assign initial values to response structure
  m_pingResponse.DestIPAddress = ip;
  m_pingResponse.EchoRequestTimeout = timeout;

  // Assign initial values to present class members
  m_requestsToSend = requests;
  m_firstRequestTimestamp = sys_now();

  // Build icmp echo request and send it
  BuildAndSendPacket();

  return true;
}

//////////////////////////////////////////////////////////////////////////////
// Ping a hostname a number of times, with specified timeout.
// Return false if an error occurs
bool Pinger::Ping(const String & hostname, u32_t requests, u32_t timeout)
{
  // Evaluate if possible to resolve hostname
  IPAddress ip;
  if(WiFi.hostByName(hostname.c_str(), ip) == false)
  {
    // Unable to resolve hostname
    return false;
  }

  // If yes, start ping routine
  bool pingSucceeded = Ping(ip, requests, timeout);

  // Update response
  m_pingResponse.DestHostname = hostname;

  return pingSucceeded;
}

//////////////////////////////////////////////////////////////////////////////
// Sets the ID of echo request packets. Useful to filter echo responses
// when multiple istances of present class are used.
void Pinger::SetPacketsId(u16_t id)
{
  m_packetId = id;
}

//////////////////////////////////////////////////////////////////////////////
// Gets the ID used to mark every echo request packet.
u16_t Pinger::GetPacketsId()
{
  return m_packetId;
}

//////////////////////////////////////////////////////////////////////////////
// Sets echo payload length, in bytes
void Pinger::SetEchoPayloadLength(u16_t len)
{
  m_echoPayloadLen = len;
}

//////////////////////////////////////////////////////////////////////////////
// Gets echo payload length, in bytes
u16_t Pinger::SetEchoPayloadLength()
{
  return m_echoPayloadLen;
}

//////////////////////////////////////////////////////////////////////////////
// Stops the current ping sequence.
void Pinger::StopPingSequence()
{
  m_requestsToSend = 0;
}

//////////////////////////////////////////////////////////////////////////////
// LWIP callback run when a ping response is received (static wrapper)
u8_t Pinger::PingReceivedStatic(
  void * pinger,
  raw_pcb * pcb,
  pbuf * packetBuffer,
  const ip_addr_t * addr)
{
  // Check parameters
  if(
    pinger == nullptr ||
    pcb == nullptr ||
    packetBuffer == nullptr ||
    addr == nullptr)
  {
    // 0 is returned to raw_recv. In this way the packet will be matched 
    // against further PCBs and/or forwarded to other protocol layers.
    return 0;
  }

  return ((Pinger *)pinger)->PingReceived(packetBuffer, addr);
}

//////////////////////////////////////////////////////////////////////////////
// LWIP callback run when a ping response is received
u8_t Pinger::PingReceived(pbuf * packetBuffer, const ip_addr_t * addr)
{
  // Check parameters
  if(packetBuffer == nullptr || addr == nullptr)
  {
    // Not free the packet, and return zero. The packet will be matched against
    // further PCBs and/or forwarded to other protocol layers.
    return 0;
  }
  
  // Save IPv4 header structure to read ttl value
  struct ip_hdr * ip = (struct ip_hdr *)packetBuffer->payload;
  if(ip == nullptr)
  {
    // Not free the packet, and return zero. The packet will be matched against
    // further PCBs and/or forwarded to other protocol layers.
    return 0;
  }
  
  // Move the ->payload pointer skipping the IPv4 header of the packet with 
  // pbuf_header function. If such function fails, it returns nonzero
  if (pbuf_header(packetBuffer, -PBUF_IP_HLEN) != 0)
  {  
    // Not free the packet, and return zero. The packet will be matched against
    // further PCBs and/or forwarded to other protocol layers.
    return 0;
  }

  // After the IPv4 header, one can access the icmp echo header
  struct icmp_echo_hdr * echoResponseHeader = 
    (struct icmp_echo_hdr *)packetBuffer->payload;
  if(echoResponseHeader == nullptr)
  {
    // Restore original position of ->payload pointer
    pbuf_header(packetBuffer, PBUF_IP_HLEN);
  
    // Not free the packet, and return zero. The packet will be matched against
    // further PCBs and/or forwarded to other protocol layers.
    return 0;
  }
  
  // Check echo response header validity
  if ((echoResponseHeader->id != m_packetId) ||
      (echoResponseHeader->seqno != htons(m_pingResponse.SequenceNumber)) ||
      (echoResponseHeader->type != ICMP_ER))
  {
    // Restore original position of ->payload pointer
    pbuf_header(packetBuffer, PBUF_IP_HLEN);
  
    // Not free the packet, and return zero. The packet will be matched against
    // further PCBs and/or forwarded to other protocol layers.
    return 0;
  }

  // Packet is valid, so read data from echo response
  
  // Set flags and counters
  m_pingResponse.ReceivedResponse = true;
  ++(m_pingResponse.TotalReceivedResponses);
  
  // Read packet ttl
  m_pingResponse.TimeToLive = ip->_ttl;

  // Detect mac address if possible
  const ip_addr_t * unused_ipaddr;
  etharp_find_addr(NULL, addr, &m_pingResponse.DestMacAddress, &unused_ipaddr);

  // Current response time
  m_pingResponse.ResponseTime = sys_now() - m_requestTimestamp;
  
  // Maximum response time
  if(m_pingResponse.ResponseTime > m_pingResponse.MaxResponseTime)
  {
    m_pingResponse.MaxResponseTime = m_pingResponse.ResponseTime;
  }
  
  // Minimum response time
  if(m_pingResponse.ResponseTime < m_pingResponse.MinResponseTime)
  {
    m_pingResponse.MinResponseTime = m_pingResponse.ResponseTime;
  }
  
  // Evaluate average response time
  m_pingResponse.AvgResponseTime += m_pingResponse.ResponseTime;
  if(m_requestsToSend == 0)
  {
    m_pingResponse.AvgResponseTime /= m_pingResponse.TotalReceivedResponses;
  }

  // If user defined a onReceive event, call such callback with the help of 
  // the ESP8266 timer with a 1 ms timeout. This trick allows to call the event
  // callback asynchronously
  if (m_onReceive != nullptr)
  {
    os_timer_disarm(&m_fakeTimer);
    os_timer_setfn(
      &m_fakeTimer,
      (os_timer_func_t *)ReceivedResponseCallback,
      (void *)this);
    os_timer_arm(&m_fakeTimer, 1, 0);
  }

  // Eat the packet by calling pbuf_free() and returning non-zero.
  // The packet will not be passed to other raw PCBs or other protocol layers.
  pbuf_free(packetBuffer);
  return 1;
}

//////////////////////////////////////////////////////////////////////////////
// Timer callback run when an Echo request timeout event occurs (static wrapper)
void Pinger::TimeoutCallback(void * pinger)
{
    return ((Pinger *)pinger)->RequestTimeoutOccurred();
}

//////////////////////////////////////////////////////////////////////////////
// Timer callback run when an Echo request timeout event occurs
void Pinger::RequestTimeoutOccurred()
{
  // Disarm request timeout timer
  os_timer_disarm(&m_requestTimeoutTimer);

  // If timeout expired without receiving any response, call onReceive event 
  // callback
  if(m_pingResponse.ReceivedResponse == false)
  {
    if(m_onReceive != nullptr)
    {
      bool result = m_onReceive(m_pingResponse);

      // If event returned false, stop ping sequence
      if(result == false)
      {
        StopPingSequence();
      }
    }
  }
  
  if(m_requestsToSend != 0)
  {
    BuildAndSendPacket();
  }
  else
  {
    // Evaluate total time spent since first request
    m_pingResponse.TotalPingingTime = sys_now() - m_firstRequestTimestamp;

    // Evaluate statistics on response time
    if(m_pingResponse.TotalReceivedResponses == 0)
    {
      m_pingResponse.AvgResponseTime = 0;
      m_pingResponse.MinResponseTime = 0;
      m_pingResponse.MaxResponseTime = 0;
    }

    // Call the end ping requests callback if defined
    if(m_onEnd != nullptr)
    {
      m_onEnd(m_pingResponse);
    }

    // Clear protocol control block
    ClearPcb();
  }
}

//////////////////////////////////////////////////////////////////////////////
// Timer callback run when an Echo response is received
void Pinger::ReceivedResponseCallback(void * pinger)
{
  Pinger &host = *(Pinger *)pinger;
  os_timer_disarm(&host.m_fakeTimer);
  if (host.m_onReceive != nullptr)
  {
    bool result = host.m_onReceive(host.m_pingResponse);

    // If event returned false, stop ping sequence
    if(result == false)
    {
      host.StopPingSequence();
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Compose echo request packet and sends it
void Pinger::BuildAndSendPacket()
{
  // Init response fields for current request
  m_pingResponse.ReceivedResponse = false;
  m_pingResponse.EchoMessageSize = m_echoPayloadLen + 
    sizeof(struct icmp_echo_hdr);
  
  // Allocate packet buffer structure. Buffer memory is allocated as one 
  // large chunk. This includes protocol headers as well.
  struct pbuf * packetBuffer = pbuf_alloc(
    PBUF_IP,
    m_pingResponse.EchoMessageSize,
    PBUF_RAM);
  if(packetBuffer == nullptr)
  {
    return;
  }
  
  // Check if packet buffer correctly created
  if((packetBuffer->len != packetBuffer->tot_len) ||
    (packetBuffer->next != nullptr))
  {
    // Free packet buffer memory and exit
    pbuf_free(packetBuffer);
    return;
  }

  // Build echo request packet
  struct icmp_echo_hdr * echoRequestHeader =
    (struct icmp_echo_hdr *)packetBuffer->payload;
  ICMPH_TYPE_SET(echoRequestHeader, ICMP_ECHO);
  ICMPH_CODE_SET(echoRequestHeader, 0);
  echoRequestHeader->chksum = 0;
  echoRequestHeader->id = m_packetId;
  ++(m_pingResponse.SequenceNumber);
  if (m_pingResponse.SequenceNumber == 0x7fff)
  {
    m_pingResponse.SequenceNumber = 0;
  }
  echoRequestHeader->seqno = htons(m_pingResponse.SequenceNumber);

  size_t icmpHeaderLen = sizeof(struct icmp_echo_hdr);
  size_t icmpDataLen = m_pingResponse.EchoMessageSize - icmpHeaderLen;

  // Just after icmp echo request header, append payload bytes to reach
  // the specified packed dimension
  char dataByte = 0x61; // 'a' character
  for(size_t i = 0; i < icmpDataLen; i++)
  {
    ((char*)echoRequestHeader)[icmpHeaderLen + i] = dataByte;
    ++dataByte;
    if(dataByte > 0x77) // 'w' character
    {
      dataByte = 0x61;
    }
  }

  // Evaluate and set packet checksum
  echoRequestHeader->chksum = inet_chksum(echoRequestHeader,
    m_pingResponse.EchoMessageSize);

  // Finally, send the packet and register timestamp
  ip_addr_t destIPAddress;
  destIPAddress.addr = m_pingResponse.DestIPAddress;
  raw_sendto(m_IcmpProtocolControlBlock, packetBuffer, &destIPAddress);
  m_requestTimestamp = sys_now();
  
  // Free packet buffer memory
  pbuf_free(packetBuffer);
  
  // Update counters
  ++(m_pingResponse.TotalSentRequests);
  --m_requestsToSend;
  
  // Start countdown for waiting response
  os_timer_disarm(&m_requestTimeoutTimer);
  os_timer_setfn(
    &m_requestTimeoutTimer,
    (os_timer_func_t *)TimeoutCallback,
    (void *)this);
  os_timer_arm(&m_requestTimeoutTimer, m_pingResponse.EchoRequestTimeout, 0);
}

//////////////////////////////////////////////////////////////////////////////
// Clear protocol control block
void Pinger::ClearPcb()
{
  if(m_IcmpProtocolControlBlock != nullptr)
  {
    // The PCB is removed from the list of RAW PCB's and the data structure
    // is freed from memory.
    raw_remove(m_IcmpProtocolControlBlock);
    m_IcmpProtocolControlBlock = nullptr;
  }
}

