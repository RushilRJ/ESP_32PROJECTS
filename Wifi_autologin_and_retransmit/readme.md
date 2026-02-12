ESP32-S3 WiFi Bridge with Automatic Captive Portal Login
A custom WiFi repeater/bridge built on the ESP32-S3 microcontroller that connects to a captive-portal-protected network (such as a university WiFi) and creates a private access point for IoT devices (e.g., smart speakers, bulbs, sensors) that cannot handle login pages themselves.
The bridge automatically detects the captive portal, extracts the dynamic session parameters, and performs the full multi-step login (credentials → policy acceptance → continue) so connected devices get seamless internet access.
Ideal for dorm rooms, hostels, or any environment with a login-required WiFi network.
Features

Dual-mode WiFi (STA + AP): Connects to the restricted network as a client while providing a private AP.
NAT routing: Shares the upstream internet connection with devices on the private AP.
Automatic captive portal handling: Detects redirects, parses dynamic tokens, and submits the exact form data required by Cisco ISE-style portals.
Three-step login support: Credentials submission, Acceptable Use Policy acceptance, and final "Continue".
Robust reconnection: Auto-reconnects and re-logs in if the upstream link drops.
Lightweight & low-power: Optimized for the ESP32-S3 with PSRAM; limits max clients to preserve stability.
Detailed serial logging: Easy troubleshooting via Serial Monitor.
