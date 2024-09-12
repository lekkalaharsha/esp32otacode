#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

const char* ssid = "New";
const char* password = "12345678";

// URL of the firmware binary hosted on GitHub (raw link)
const char* firmwareUrl = "https://raw.githubusercontent.com/lekkalaharsha/esp32otacode/main/otachatgpt.ino.esp32.bin";

// LED Pin
#define LED_PIN 2

void setup() {
  Serial.begin(115200);

  // Setup the LED pin as output
  pinMode(LED_PIN, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Start the OTA update process
  checkForUpdate();
}

void loop() {
  // Blink the LED to show the system is working
  digitalWrite(LED_PIN, HIGH);  // LED ON
  delay(500);
  digitalWrite(LED_PIN, LOW);   // LED OFF
  delay(500);
  Serial.print("old version");
}

void checkForUpdate() {
  Serial.println("Checking for firmware update...");

  HTTPClient http;
  http.begin(firmwareUrl);  // Specify the URL
  int httpCode = http.GET();  // Make the request

  // Debugging: HTTP response code
  Serial.printf("HTTP response code: %d\n", httpCode);

  if (httpCode == 200) {  // Check for valid response
    int contentLength = http.getSize();

    // Debugging: content length
    Serial.printf("Content length: %d\n", contentLength);

    if (contentLength > 0) {
      bool canBegin = Update.begin(contentLength);
      if (canBegin) {
        Serial.println("Begin OTA update...");
        WiFiClient *stream = http.getStreamPtr();

        // Create a buffer to hold incoming data
        uint8_t buff[512] = { 0 };  // Use a larger buffer (512 bytes)
        int written = 0;

        // Stop blinking while updating
        digitalWrite(LED_PIN, LOW);

        // Read data from the stream
        while (http.connected() && (written < contentLength)) {
          size_t available = stream->available();

          if (available) {
            int bytesRead = stream->readBytes(buff, ((available > sizeof(buff)) ? sizeof(buff) : available));
            written += bytesRead;

            // Write the buffer to the update process
            if (Update.write(buff, bytesRead) != bytesRead) {
              Serial.println("Error writing bytes to update!");
              Update.printError(Serial);
              break;
            }

            // Debugging: Show progress
            Serial.printf("Written: %d/%d bytes\n", written, contentLength);
          }
          delay(1);  // Give some breathing room to the system
        }

        // Finalize the update process
        if (Update.end()) {
          if (Update.isFinished()) {
            Serial.println("OTA update successful! Restarting...");
            ESP.restart();  // Restart the device after successful update
          } else {
            Serial.println("OTA update not finished.");
          }
        } else {
          Serial.println("OTA update failed.");
          Update.printError(Serial);
        }
      } else {
        // Debugging: Not enough space or unable to begin update
        Serial.println("Not enough space or unable to begin update.");
      }
    } else {
      // Debugging: Invalid content length or no update
      Serial.println("Invalid content length or no new update available.");
    }
  } else {
    // Debugging: Failed to fetch firmware
    Serial.printf("Failed to fetch firmware. HTTP code: %d\n", httpCode);
  }

  http.end();
}
