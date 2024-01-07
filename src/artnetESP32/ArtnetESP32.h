/*The MIT License (MIT)

Copyright (c) 2014 Nathanaël Lécaudé
https://github.com/natcl/Artnet, http://forum.pjrc.com/threads/24688-Artnet-to-OctoWS2811

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef ARTNETESP32_H
#define ARTNETESP32_H
//#define CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM  64
#include <Arduino.h>
//#include "FastLED.h"
#if defined(ARDUINO_SAMD_ZERO)
#include <WiFi101.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
//#include "WifiUdp.h"
#elif defined(ESP32)
#include <WiFi.h>
#include "UdpArtnet.h"
//#include "WifiUdp.h"
#else
#include <Ethernet.h>
#include <EthernetUdp.h>
#endif
#include <Udp.h>
// UDP specific

#define ART_NET_PORT 6454
// Opcodes
#define ART_POLL 0x2000
#define ART_POLL_REPLY 0x2100
#define ART_DMX 0x5000
#define ART_SYNC 0x5200
// Buffers
#define MAX_BUFFER_ARTNET 800
// Packet
#define ART_NET_ID "Art-Net\0"
#define ART_DMX_START 18

#include "freertos/queue.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "esp_task_wdt.h"
#include "FS.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#define BUFFER_SIZE 800

//static QueueHandle_t _artnet_queue;
static TaskHandle_t recordArtnetHandle;
static TaskHandle_t readFromSDHandle;
static TaskHandle_t userArtnetHandle;

struct artnet_reply_s
{
  uint8_t id[8];
  uint16_t opCode;
  uint8_t ip[4];
  uint16_t port;
  uint8_t verH;
  uint8_t ver;
  uint8_t subH;
  uint8_t sub;
  uint8_t oemH;
  uint8_t oem;
  uint8_t ubea;
  uint8_t status;
  uint8_t etsaman[2];
  uint8_t shortname[18];
  uint8_t longname[64];
  uint8_t nodereport[64];
  uint8_t numbportsH;
  uint8_t numbports;
  uint8_t porttypes[4]; //max of 4 ports per node
  uint8_t goodinput[4];
  uint8_t goodoutput[4];
  uint8_t swin[4];
  uint8_t swout[4];
  uint8_t swvideo;
  uint8_t swmacro;
  uint8_t swremote;
  uint8_t sp1;
  uint8_t sp2;
  uint8_t sp3;
  uint8_t style;
  uint8_t mac[6];
  uint8_t bindip[4];
  uint8_t bindindex;
  uint8_t status2;
  uint8_t filler[26];
} __attribute__((packed));

class ArtnetESP32
{
public:
  ArtnetESP32();
  TaskHandle_t artnetAfterFrameHandle;

  void (*frameCallback)();
  void (*frameRecordCallback)();
  void (*readFromSDCallback)();
  static void afterFrameTask(void *pvParameters);
  static void recordArtnetTask(void *pvParameters);
  static void readFromSDTask(void *pvParameters);
  File recordfile;
  void startArtnetrecord(File file);
  bool stopRecord = true;
  void stopArtnetRecord();
  // uint16_t readFrame();
  uint16_t readFrameRecord();
  uint16_t buffer_read = 0;
  uint16_t buffer_play = 0;
  uint32_t timenow = 0;
  uint8_t *getFrameReadSD();
  uint32_t totalrecording = 0;
  // void getFrameandRecord();
  uint8_t *ledsbuffer = NULL;
  void setLedsBuffer(uint8_t *leds);
  SemaphoreHandle_t Artnet_Semaphore2 = xSemaphoreCreateBinary();
  uint16_t last_size = 0;
  uint32_t getsync();
  uint32_t recordStartTime;
  uint32_t recordEndTime;
  int startuniverse;
  bool firstframe = false;
  bool readNextFrameAndWait(File playingfile);
  bool running = false;
  uint8_t *getframe(int framenumber);
  uint8_t *getframe();
  //void getframe2(uint8_t* leds);
  void getframe(uint8_t *leds);
  void getBufferFrame(uint8_t *leds);
  void getFrameForRecord(uint8_t *leds);
  uint8_t *getframeread(int buffer);
  uint32_t nbframeread;
  uint32_t frameslues = 0;
  uint32_t lostframes = 0;
  void resetsync();
  //void setframe(CRGB * frame);
  void begin(byte mac[], byte ip[]);
  void begin();
  void begin(uint16_t nbPixels, uint16_t nbPixelsPerUniverses);
  void begin(uint16_t nbPixels, uint16_t nbPixelsPerUniverses, int starunivers);
  void setBroadcast(byte bc[]);
  uint16_t read();
  uint16_t read2();

  uint16_t readFrame()
  {
    uint16_t result;
    result = read3();
    if (result == 1 and artnetAfterFrameHandle)
      xTaskNotifyGive(artnetAfterFrameHandle);
    return result;
  }
  uint16_t read3()
  {
    struct sockaddr_in si_other;
    int slen = sizeof(si_other), len;
    long timef = 0;

    //timef=millis();
    incomingUniverse = 99;
    uint32_t decal = nbPixelsPerUniverse * 3 + ART_DMX_START;
    uint32_t decal2 = nbNeededUniverses * decal;
    uint8_t *offset, *offset2;
    // bool resetframe=true;
    offset2 = artnetleds1 + currentframenumber * decal2;
  er:

#ifndef ARTNET_NO_TIMEOUT
    timef = millis();
#endif
    offset = offset2;

    while (incomingUniverse != startuniverse)
    {
#ifndef ARTNET_NO_TIMEOUT
      if (millis() - timef > 1000)
      {
        Serial.println("Time out fired");
        return 0;
      }
#endif
      //MSG_DONTWAIT
      if ((len = recvfrom(Udp.udp_server, offset, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *)&si_other, (socklen_t *)&slen)) > 0) //1460
      {

        incomingUniverse = offset[14];
#ifdef ARTNET_DEBUG
        printf("Universe : %d length:%d \n", incomingUniverse, len);
#endif
      }
    }
#ifdef ARTNET_DEBUG
        printf("Passed\n");
#endif
    for (int uni = startuniverse + 1; uni < nbNeededUniverses + startuniverse; uni++)
    {
      offset += decal;
      //recvfrom(Udp.udp_server, offset, BUFFER_SIZE,MSG_DONTWAIT, (struct sockaddr *)&si_other, (socklen_t *)&slen) <= 0
     //recv(Udp.udp_server, offset, BUFFER_SIZE,MSG_DONTWAIT)<=0
      while ( recvfrom(Udp.udp_server, offset, BUFFER_SIZE,MSG_DONTWAIT, (struct sockaddr *)&si_other, (socklen_t *)&slen) <= 0)
      {
        //incomingUniverse = *(offset + 14);

#ifndef ARTNET_NO_TIMEOUT
        if (millis() - timef > 1000)
        {
          Serial.println("Time out fired");
          return 0;
        }

#endif
      }
      incomingUniverse = *(offset + 14);
      if (incomingUniverse != uni)
      {
#ifdef ARTNET_DEBUG
        printf("Universe : %d length:%d expected universer:%d\n", incomingUniverse, len, uni);
#endif
        lostframes++;
        // resetframe=false;
        goto er;
      }
    }

    currentframenumber = (currentframenumber + 1) % 2;
    frameslues++;
    return 1;
  }

  uint16_t read4();
  uint16_t readWithoutWaiting();
  uint16_t read2(TaskHandle_t task);
  uint16_t read(TaskHandle_t task);
  void printPacketHeader();
  void printPacketContent();
  void stop();

  uint32_t sync = 0;
  uint32_t syncmax = 0;
  uint32_t sync2 = 0;
  uint32_t syncmax2 = 0;
  uint32_t elaspe[2];
  uint32_t start_time = 0;
  uint32_t current_time = 0;
  uint32_t getElaspseTime();
  // Return a pointer to the start of the DMX data
  inline uint8_t *getDmxFrame(void)
  {
    return artnetPacket + ART_DMX_START;
  }

  inline uint16_t getOpcode(void)
  {
    return opcode;
  }

  inline uint8_t getSequence(void)
  {
    return sequence;
  }

  inline uint16_t getUniverse(void)
  {
    return incomingUniverse;
  }

  inline uint16_t getLength(void)
  {
    return dmxDataLength;
  }

  inline IPAddress getRemoteIP(void)
  {
    return remoteIP;
  }

  inline void setArtDmxCallback(void (*fptr)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data, IPAddress remoteIP))
  {
    artDmxCallback = fptr;
  }

  inline void setArtSyncCallback(void (*fptr)(IPAddress remoteIP))
  {
    artSyncCallback = fptr;
  }

  inline void setFrameCallback(void (*fptr)())
  {
    frameCallback = fptr;
  }

  inline void setframeRecordCallback(void (*fptr)())
  {
    frameRecordCallback = fptr;
  }

  inline void setreadFromSDCallback(void (*fptr)())
  {
    readFromSDCallback = fptr;
  }

private:
  uint8_t node_ip_address[4];

  uint8_t id[8];
#if defined(ARDUINO_SAMD_ZERO) || defined(ESP8266) || defined(ESP32)
  WiFiUDPArtnet Udp;
  // WiFiUDP Udp;
#else
  EthernetUDP Udp;
#endif
  struct artnet_reply_s ArtPollReply;

  uint16_t nbPixelsPerUniverse;
  uint16_t nbPixels;
  uint16_t nbNeededUniverses;
  uint8_t *artnetleds1;
  uint8_t *artnetleds2;
  uint8_t *artnetleds3;
  uint8_t *currentframe;
  uint8_t *frames[10];
  uint8_t artnetPacket[MAX_BUFFER_ARTNET];
  uint16_t packetSize;
  IPAddress broadcast;
  uint16_t opcode;
  uint8_t sequence;
  uint16_t incomingUniverse;
  uint16_t dmxDataLength;
  uint8_t currentframenumber;
  uint8_t buffernum;
  uint8_t readbuffer;
  IPAddress remoteIP;
  void (*artDmxCallback)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data, IPAddress remoteIP);
  void (*artSyncCallback)(IPAddress remoteIP);
};

#endif
