ESP32 Website Update Notifier
Overview
This project uses an ESP32-S3 development board to monitor a specific website category (the All India Chess Federation's "Arbiters News" RSS feed) for new updates or articles. It acts as a simple IoT notifier: the onboard RGB LED changes color to indicate status—green for no changes (baseline state), red for a new update detected. This is perfect for waiting on important announcements like exam results or news posts without constantly checking the site manually.
The board connects to WiFi, fetches the RSS feed every 60 seconds (configurable), extracts the latest article link, and compares it to a hardcoded baseline. If a new post appears (link changes), the LED turns solid red permanently (even across reboots) until reset.
Built with Arduino IDE, this is a beginner-friendly IoT project focusing on networking, HTTP requests, and LED control. It's tailored for the ESP32-S3-1 N16R8 model (16MB flash, 8MB PSRAM, Type-C USB).
Features

WiFi Connectivity: Automatically connects to your network and retries on failure.
RSS Feed Monitoring: Checks the AICF Arbiters News feed for new articles.
Change Detection: Compares the latest article link to a hardcoded baseline; alerts on any newer post.
LED Indicators:
Solid Green: No new updates (matches baseline).
Solid Red: New update detected (persistent alert).
Slow Blinking Green: Refreshing/checking the feed.
Fast Blinking Red: WiFi or HTTP error.

Persistent Alert: Red state saved across reboots using non-volatile storage.
Efficient Checks: Uses conditional HTTP requests (ETag/Last-Modified) to avoid unnecessary downloads.
Customizable: Easily change the baseline link, refresh interval, or URL for other sites.

Hardware Requirements

ESP32-S3 development board (e.g., ESP32-S3-1 N16R8 with onboard RGB LED on GPIO48).
Type-C USB cable for power and programming.
No additional hardware needed—uses the built-in RGB LED.

Note: If your board's RGB LED doesn't light up, check if the "RGB" pads on the back need soldering (common on some variants).
Software Requirements

Arduino IDE (version 2.x recommended).
ESP32 board support: Add https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json in Preferences > Additional Boards Manager URLs, then install "esp32" by Espressif.
Libraries:
Adafruit NeoPixel (for RGB LED).
Built-in: WiFi, HTTPClient, Preferences.
