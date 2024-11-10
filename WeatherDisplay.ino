/*
Name: WeatherDisplay
Description: An ESP8266 device that connects to a WiFi network, fetches location data based on the IP address, retrieves weather information from the Open-Meteo API, and displays the current time, temperature, and humidity on the OLED screen.
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>

#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager/releases/tag/v2.0.17
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson/releases/tag/v6.21.5
#include <SSD1306Wire.h>  // https://github.com/ThingPulse/esp8266-oled-ssd1306/releases/tag/4.6.1
#include <OLEDDisplayUi.h>
#include <ezTime.h>  // https://github.com/ropg/ezTime/releases/tag/0.8.3

// Pin Configuration
#define PIN_I2C_SDA 4
#define PIN_I2C_SCL 5

// Name of the access point for WiFiManager
const char *ap_name = "WeatherDisplay";

// Global variables to store location and weather
char current_timezone[50] = "UTC0";
float current_latitude = 0.0;
float current_longitude = 0.0;

float current_temperature = 0.0;
int current_humidity = 0;

unsigned long previousMillis = 0;  // Stores the last time weather was updated
const long interval = 900000;      // Update interval set to 15 minutes (in milliseconds)

WiFiManager wm;  // WiFiManager object for managing WiFi connections
Timezone myTZ;

// OLED display setup (I2C address 0x3c, SDA, SCL pins)
SSD1306Wire display(0x3c, PIN_I2C_SDA, PIN_I2C_SCL, GEOMETRY_64_48);
OLEDDisplayUi ui(&display);

// Function to update location using IP-based geolocation API
void updateLocation() {
  Serial.println("Updating location...");

  String server = "http://ip-api.com/json";  // API endpoint for IP-based location data

  WiFiClient client;  // WiFi client for making HTTP requests
  HTTPClient http;    // HTTP client for sending/receiving HTTP requests

  http.begin(client, server);         // Start HTTP connection
  int httpResponseCode = http.GET();  // Send GET request
  if (httpResponseCode > 0) {         // Check if request was successful
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    StaticJsonDocument<512> doc;  // JSON document to store response
    DeserializationError error = deserializeJson(doc, http.getString());
    if (error) {  // Check if JSON parsing was successful
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    } else {
      // Store location and timezone data
      strcpy(current_timezone, doc["timezone"]);
      current_latitude = doc["lat"];
      current_longitude = doc["lon"];
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);  // Print error code if request failed
  }
  http.end();     // End HTTP connection
  client.stop();  // Close WiFi client

  Serial.print("Timezone: ");
  Serial.println(current_timezone);
  Serial.print("Latitude: ");
  Serial.println(current_latitude);
  Serial.print("Logitude: ");
  Serial.println(current_longitude);
}

// Function to update weather data using Open-Meteo API
void updateWeather() {
  Serial.println("Updating weather...");

  // Construct API request URL with current location and timezone
  String server = "http://api.open-meteo.com/v1/forecast?timezone=auto";
  server += "&current=temperature_2m,relative_humidity_2m";
  server += "&latitude=";
  server += String(current_latitude);
  server += "&longitude=";
  server += String(current_longitude);

  WiFiClient client;  // WiFi client for making HTTP requests
  HTTPClient http;    // HTTP client for sending/receiving HTTP requests

  http.begin(client, server);         // Start HTTP connection
  int httpResponseCode = http.GET();  // Send GET request
  if (httpResponseCode > 0) {         // Check if request was successful
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    StaticJsonDocument<1024> doc;  // JSON document to store response
    DeserializationError error = deserializeJson(doc, http.getString());
    if (error) {  // Check if JSON parsing was successful
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    } else {
      // Store current temperature and humidity data
      JsonObject current = doc["current"];
      current_temperature = current["temperature_2m"];
      current_humidity = current["relative_humidity_2m"];
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);  // Print error code if request failed
  }
  http.end();     // End HTTP connection
  client.stop();  // Close WiFi client

  Serial.print("Temperature: ");
  Serial.println(current_temperature);
  Serial.print("Humidity: ");
  Serial.println(current_humidity);
}

// Function to draw the first frame (clock) on the OLED display
void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(32 + x, 0 + y, myTZ.dateTime("H:i"));
  display->setFont(ArialMT_Plain_10);
  display->drawString(32 + x, 24 + y, myTZ.dateTime("d/m/Y"));
}

// Function to draw the second frame (temperature) on the OLED display
void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(32 + x, 0 + y, "Temperature");  // Label for temperature
  display->setFont(ArialMT_Plain_16);
  display->drawString(32 + x, 20 + y, String(current_temperature) + " °C");  // Display temperature
}

// Function to draw the third frame (humidity) on the OLED display
void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(32 + x, 0 + y, "Humidity");  // Label for humidity
  display->setFont(ArialMT_Plain_16);
  display->drawString(32 + x, 20 + y, String(current_humidity) + "%");  // Display humidity
}

// Array of frame callback functions for the UI
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3 };
int frameCount = 3;  // Number of frames

// Initial setup function
void setup() {
  Serial.begin(115200);  // Start serial communication at 115200 baud

  WiFi.mode(WIFI_STA);  // Set WiFi mode to station

  display.init();                  // Initialize the OLED display
  display.flipScreenVertically();  // Flip the display vertically
  display.clear();                 // Clear the display
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(display.getWidth() / 2, 0, "Waiting for");  // Display "Waiting for" text
  display.setFont(ArialMT_Plain_16);
  display.drawString(display.getWidth() / 2, 24, "WiFi");  // Display "WiFi" text
  display.display();                                       // Show the display

  if (!wm.autoConnect(ap_name)) {  // Attempt to auto-connect to WiFi
    Serial.println("Failed to connect and hit timeout! Rebooting...");
    delay(5000);
    ESP.restart();  // Restart ESP8266 if connection fails
  } else {
    Serial.println("Connected!");
  }

  display.clear();  // Clear the display
  display.end();    // End the display

  updateLocation();  // Update location data
  updateWeather();   // Update weather data

  waitForSync();
  myTZ.setLocation(current_timezone);
  Serial.println(myTZ.dateTime());

  ui.setTargetFPS(60);                   // Set UI target FPS to 60
  ui.setIndicatorPosition(BOTTOM);       // Set UI indicator position to bottom
  ui.setIndicatorDirection(LEFT_RIGHT);  // Set indicator direction
  ui.setFrameAnimation(SLIDE_LEFT);      // Set frame animation style
  ui.setFrames(frames, frameCount);      // Assign frames to the UI
  ui.init();                             // Initialize the UI
  display.flipScreenVertically();        // Flip the display vertically
}

// Main loop function
void loop() {
  events();

  int remainingTimeBudget = ui.update();  // Update the UI and get remaining time

  if (remainingTimeBudget > 0) {
    delay(remainingTimeBudget);  // Delay if time budget remains
  }

  unsigned long currentMillis = millis();  // Get current time in milliseconds

  if ((unsigned long)(currentMillis - previousMillis) >= interval) {
    previousMillis = currentMillis;  // Update previousMillis
    updateWeather();                 // Update weather data if interval passed
  }
}