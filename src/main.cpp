//platformio run -t upload --upload-port 192.168.0.41
//uploading by OTA command

#include <Arduino.h>
#include <helper.h>
#include <artnetESP32/ArtnetESP32.h>

#define UNIVERSES_COUNT 4 //4
#define START_UNIVERSE 36 ///////////////////////////    UNIVERSE AND LAST IN IP

// network ip config
IPAddress ipaddr = IPAddress(192,168,0,START_UNIVERSE);
IPAddress gateway = IPAddress(192,168,0,101);
IPAddress subnet = IPAddress(255,255,255,0);

// artnet config
uint16_t pixelsPerUni = UNIVERSE_SIZE; //
uint8_t startUniverse = START_UNIVERSE; //
uint8_t universesCount = UNIVERSES_COUNT; //
uint16_t pixelCount = pixelsPerUni * universesCount;

// create arrs
uint8_t uniData[512];
uint8_t headerData[18]; //artnetHeader

long lostPackets = 0;
// long lastPacketTime = 0;

//for syncTime
long lastPacketTime = 0;

WiFiUDP udp;

// if use WIFI
void connectWiFi() {
  WiFi.mode(WIFI_STA);

    Serial.printf("Connecting ");
    WiFi.begin("udp", "esp18650");

    while (WiFi.status() != WL_CONNECTED) {
      Serial.println(WiFi.status());

        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

// if use LAN
void beginLan() {
  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  ETH.config(ipaddr, gateway, subnet);
}

void setup() {
  Serial.begin(115200);
  delay(10);
  //connectWiFi();
  beginLan();
  udp.begin(6454);

  // testlong();
  OTA_Func();
}

uint32_t unisCount = 0;
void readUdp() {
  if(udp.parsePacket()) {
    udp.read(headerData, 18);
    int uniSize = (headerData[16] << 8) + headerData[17];
    uint8_t universe = headerData[14];
    if(universe >= startUniverse && universe < (startUniverse + universesCount) && uniSize > 500) {
      printf("univ: %d, time: %lumcs\n", universe, micros() - lastPacketTime);
    }
    udp.flush();
  }
}


void loop() {
  //MyLan
  ArduinoOTA.handle();
  readUdp();
}

  //OTA - Flashing over Air (WIFI or LAN)
void OTA_Func() {
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  }