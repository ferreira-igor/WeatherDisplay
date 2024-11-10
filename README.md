# WeatherDisplay

An ESP8266 device that connects to a WiFi network, fetches location data based on the IP address, retrieves weather information from the Open-Meteo API, and displays the current time, temperature, and humidity on the OLED screen.

## Hardware Requirements

- Lolin D1 Mini (ESP8266)
- Wemos OLED 0.66" Shield

## Using the Device

### 1. Connecting to WiFi

- Upon powering on, the device will attempt to connect to a previously saved WiFi network. If it cannot connect, it will create an access point named "WeatherDisplay".
- Connect your phone or computer to this WiFi network.
- A captive portal should appear. Select your home WiFi network and enter the password.

### 2. Viewing Weather Information

- The OLED display will show the current time, temperature, and humidity in a loop.
- The data is updated every 15 minutes.
