#include <AsyncHTTPRequest_Generic.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <PNGdec.h> 
#include "webpages.h"
#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 256

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 23

#define FIRMWARE_VERSION "v0.0.1"

PNG png;

const String default_ssid = "SSID";
const String default_wifipassword = "PASSWORD";
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const int default_webserverporthttp = 80;
const int PANEL_HEIGHT = 16;  // height of LED panel
const int PANEL_WIDTH = 16;   // width of LED panel

// configuration structure
struct Config {
  String ssid;               // wifi ssid
  String wifipassword;       // wifi password
  String httpuser;           // username to access web admin
  String httppassword;       // password to access web admin
  int webserverporthttp;     // http port number for web admin
};

// variables
Config config;                        // configuration
bool shouldReboot = false;            // schedule a reboot
AsyncWebServer *server;               // initialise webserver

// Define the array of leds
CRGB leds[NUM_LEDS];

uint32_t ledpic[PANEL_WIDTH*PANEL_HEIGHT];  // storing picture information for LED panel (16x16, 24bit per pixel)
uint32_t leddefault[PANEL_WIDTH*PANEL_HEIGHT] = // initial values for LED panel
{
0x2196f3, 0x2196f3, 0x2196f3, 0x2196f3, 0x2196f3, 0xffffff, 0xffffff, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x4caf50, 0x4caf50, 0x4caf50, 0x4caf50, 0x4caf50,
0x2196f3, 0x2196f3, 0x2196f3, 0xffffff, 0xffffff, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x4caf50, 0x4caf50, 0x4caf50,
0x2196f3, 0x2196f3, 0xffffff, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x4caf50, 0x4caf50,
0x2196f3, 0xffffff, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x4caf50,
0x2196f3, 0xffffff, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x4caf50,
0xffffff, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b,
0xffffff, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b,
0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b,
0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b,
0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xff9800,
0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0xffeb3b, 0xffeb3b, 0xff9800,
0xb71c1c, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0x000000, 0xffeb3b, 0xffeb3b, 0xff9800, 0x9c27b0,
0xb71c1c, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0x000000, 0x000000, 0x000000, 0x000000, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xff9800, 0x9c27b0,
0xb71c1c, 0xb71c1c, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xff9800, 0x9c27b0, 0x9c27b0,
0xb71c1c, 0xb71c1c, 0xb71c1c, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xff9800, 0xff9800, 0x9c27b0, 0x9c27b0, 0x9c27b0,
0xb71c1c, 0xb71c1c, 0xb71c1c, 0xb71c1c, 0xb71c1c, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xffeb3b, 0xff9800, 0xff9800, 0x9c27b0, 0x9c27b0, 0x9c27b0, 0x9c27b0, 0x9c27b0
};

// function defaults
String listFiles(bool ishtml = false);

void setup() {
  // init LED panel
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical
  FastLED.clear();
  FastLED.setBrightness(32);
  
  Serial.begin(115200);

  Serial.print("Firmware: "); Serial.println(FIRMWARE_VERSION);

  Serial.println("Booting ...");

  Serial.println("Mounting SPIFFS ...");
  if (!SPIFFS.begin(true)) {
    // if you have not used SPIFFS before on a ESP32, it will show this error.
    // after a reboot SPIFFS will be configured and will happily work.
    Serial.println("ERROR: Cannot mount SPIFFS, Rebooting");
    rebootESP("ERROR: Cannot mount SPIFFS, Rebooting");
  }

  Serial.print("SPIFFS Free: "); Serial.println(humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes())));
  Serial.print("SPIFFS Used: "); Serial.println(humanReadableSize(SPIFFS.usedBytes()));
  Serial.print("SPIFFS Total: "); Serial.println(humanReadableSize(SPIFFS.totalBytes()));

  Serial.println(listFiles());

  Serial.println("Loading Configuration ...");

  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.httpuser = default_httpuser;
  config.httppassword = default_httppassword;
  config.webserverporthttp = default_webserverporthttp;

  Serial.print("\nConnecting to Wifi: ");
  WiFi.begin(config.ssid.c_str(), config.wifipassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n\nNetwork Configuration:");
  Serial.println("----------------------");
  Serial.print("         SSID: "); Serial.println(WiFi.SSID());
  Serial.print("  Wifi Status: "); Serial.println(WiFi.status());
  Serial.print("Wifi Strength: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  Serial.print("          MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("           IP: "); Serial.println(WiFi.localIP());
  Serial.print("       Subnet: "); Serial.println(WiFi.subnetMask());
  Serial.print("      Gateway: "); Serial.println(WiFi.gatewayIP());
  Serial.print("        DNS 1: "); Serial.println(WiFi.dnsIP(0));
  Serial.print("        DNS 2: "); Serial.println(WiFi.dnsIP(1));
  Serial.print("        DNS 3: "); Serial.println(WiFi.dnsIP(2));
  Serial.println();

  // configure web server
  Serial.println("Configuring Webserver ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  // startup web server
  Serial.println("Starting Webserver ...");
  server->begin();

  printArray(leddefault, NUM_LEDS, 5000);
}

void loop() {
  // reboot if we've told it to reboot
  if (shouldReboot) {
    rebootESP("Web Admin Initiated Reboot");
  }
  readFiles();
  
}

void printArray(uint32_t Panel[], int SizeOfPanel, int DelayTime) {
  for (int i = 0; i < SizeOfPanel; i++) {
    // Check if odd or even row
    int n = i;
    int r = i / 16;
    // 0..2..4.. is reverse
    // 1..3..5.. is normal (forward)
    if ( r % 2 == 0 ){ // reverse
      n = r * 16 + 15 - (i % 16);
    } 
    
    leds[n] = Panel[i];
  }
  FastLED.show();
  delay(DelayTime);
}

// Functions to access a file on the SD card
File myfile;

void * myOpen(const char *filename, int32_t *size) {
  Serial.printf("Attempting to open %s\n", filename);
  myfile = SPIFFS.open(filename);
  *size = myfile.size();
  return &myfile;
}
void myClose(void *handle) {
  if (myfile) myfile.close();
}
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!myfile) return 0;
  return myfile.read(buffer, length);
}
int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!myfile) return 0;
  return myfile.seek(position);
}

// Function to draw pixels to the display
void PNGDraw(PNGDRAW *pDraw) {
uint16_t usPixels[PANEL_WIDTH];

  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);

  // Convert to RGB888 (true color)
  for(int i = 0; i < PANEL_WIDTH; i++) {
    uint8_t r = ((((usPixels[i] >> 11) & 0x1F) * 527) + 23) >> 6;
    uint8_t g = ((((usPixels[i] >> 5) & 0x3F) * 259) + 33) >> 6;
    uint8_t b = (((usPixels[i] & 0x1F) * 527) + 23) >> 6;

    uint32_t RGB888 = r << 16 | g << 8 | b;

    // write into LED array
    ledpic[ i + PANEL_WIDTH*pDraw->y ] = RGB888;
  }
}

void readFiles() {
  int rc;
  int filenum = 0;
  Serial.println("Reading Files from SPIFFS");
  File root = SPIFFS.open("/");
  File foundfile = root.openNextFile();
  // 
  while (foundfile) {
    if (foundfile.isDirectory() == false) {
      Serial.print("Filename :" + String(foundfile.name()) + "\n");
      const char *name = foundfile.name();
      const int len = strlen(name);
      if (len > 3 && strcmp(name + len - 3, "png") == 0) {
        // it's a PNG ;-)
        rc = png.open((const char *)name, myOpen, myClose, myRead, mySeek, PNGDraw);
        if (rc == PNG_SUCCESS) {
          Serial.printf("image number %d\n", filenum);
          Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d, buffer size: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType(), png.getBufferSize());
          // dump color values from PNG
          rc = png.decode(NULL, 0);
          // close file and increase counter
          png.close();
          filenum++;

          // now display on LED matrix
          /*
          // dump led matrix array
          Serial.println("Dump LED matrix array");
          for(int y = 0; y < PANEL_HEIGHT; y++) {
            for(int x = 0; x < PANEL_WIDTH; x++) {
              Serial.printf("0x%06X, ",ledpic[x+PANEL_WIDTH*y]);
            }
            Serial.println();
          }
          */
          printArray(ledpic, NUM_LEDS, 5000);
        }
      }
    }
    
    foundfile = root.openNextFile();
  }
  foundfile.close();
  root.close();
}

void rebootESP(String message) {
  Serial.print("Rebooting ESP32: "); Serial.println(message);
  ESP.restart();
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  Serial.println("Listing files stored on SPIFFS");
  File root = SPIFFS.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    } else {
      returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
