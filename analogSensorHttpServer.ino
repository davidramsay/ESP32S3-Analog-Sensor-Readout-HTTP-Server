// ESP32-S3 Arduino sketch
// WiFi credentials via Serial, simple HTTP API returning analog value on A0 in JSON.

#include <WiFi.h>
#include <WebServer.h>

#define ANALOG_PIN A0                  // Change to the actual GPIO if your board doesn't define A0
#define HTTP_PORT 80
#define WIFI_CONNECT_TIMEOUT_MS 30000  // 30 seconds

WebServer server(HTTP_PORT);

String readLineFromSerial(const char* prompt) {
  Serial.print(prompt);
  String input;
  // Read until newline; ignore carriage returns
  while (true) {
    while (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n') {
        // Trim whitespace
        input.trim();
        return input;
      } else if (c != '\r') {
        input += c;
      }
    }
    delay(10);
  }
}

// Simple handler: returns {"value": <analogRead(A0)>}
void handleAdc() {
  int value = analogRead(ANALOG_PIN);

  // Prepare JSON
  String json = String("{\"value\": ") + String(value) + "}";

  // Add basic headers (JSON + CORS)
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Content-Type", "application/json");
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "application/json", "{\"error\": \"not found\"}");
}

void setup() {
  Serial.begin(115200);
  // Wait for Serial to be ready (useful when powered via USB)
  while (!Serial) { delay(10); }

  Serial.println("\nESP32-S3 WiFi + API Server");
  Serial.println("Enter WiFi credentials. Type each line and press Enter.");

  String ssid     = readLineFromSerial("SSID: ");
  String password = readLineFromSerial("Password: ");

  if (ssid.length() == 0) {
    Serial.println("SSID cannot be empty. Restarting...");
    delay(2000);
    ESP.restart();
  }

  Serial.printf("Connecting to SSID \"%s\" ...\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait for connection with timeout
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection timed out. Check credentials and reset.");
    // Optionally restart to retry
    // ESP.restart();
    return;
  }

  Serial.println("WiFi connected!");
  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());

  // Configure ADC (optional: set resolution/attenuation if needed)
  // analogReadResolution(12);               // ESP32 default is 12-bit
  // analogSetAttenuation(ADC_11db);         // Wider input voltage range

  // Set up HTTP routes
  server.on("/api/adc", HTTP_GET, handleAdc);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.printf("HTTP server started on port %d\n", HTTP_PORT);
  Serial.println("Try: GET http://<above_ip>/api/adc");
}

void loop() {
  // Handle incoming HTTP requests
  server.handleClient();
}
