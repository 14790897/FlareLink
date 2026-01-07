// ESP32-C3 UART trigger: receive five 0xFF bytes -> latch GPIO12 HIGH
#include <Arduino.h>

constexpr int UART_TX_PIN = 0;   // GPIO0 as UART TX
constexpr int UART_RX_PIN = 1;   // GPIO1 as UART RX
constexpr int OUTPUT_PIN  = 12;  // GPIO12 to drive HIGH
constexpr uint32_t UART_BAUD = 9600;

void setup() {
  // Initialize USB Serial for logging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32-C3 UART Trigger Started ===");
  
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);
  Serial.printf("GPIO%d initialized as OUTPUT (LOW)\n", OUTPUT_PIN);

  // Initialize UART on specified pins
  Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.printf("UART initialized: TX=GPIO%d, RX=GPIO%d, Baud=%d\n", 
                UART_TX_PIN, UART_RX_PIN, UART_BAUD);
  Serial.println("Waiting for 5x 0xFF trigger...\n");
}

void loop() {
  static uint8_t consecutiveFF = 0;
  static bool outputActive = false;
  static unsigned long outputStartTime = 0;
  static unsigned long lastDebugTime = 0;
  const unsigned long OUTPUT_DURATION = 2000; // 2 seconds in milliseconds

  // Check if output is active and should be turned off
  if (outputActive) {
    // Clear any incoming UART data during output phase to prevent interference
    while (Serial1.available() > 0) {
      Serial1.read(); // Discard data
    }
    
    unsigned long elapsed = millis() - outputStartTime;
    if (elapsed >= OUTPUT_DURATION) {
      digitalWrite(OUTPUT_PIN, LOW);
      outputActive = false;
      consecutiveFF = 0; // Reset counter for next trigger
      Serial.printf("[OUTPUT] GPIO12 -> LOW (elapsed=%lums, ready for next trigger)\n\n", elapsed);
    } else {
      // Keep pin HIGH during the entire duration
      digitalWrite(OUTPUT_PIN, HIGH);
      
      // Periodic status update while HIGH
      if (millis() - lastDebugTime >= 500) {
        Serial.printf("[STATUS] GPIO12=HIGH, elapsed=%lums/%lums\n", elapsed, OUTPUT_DURATION);
        lastDebugTime = millis();
      }
    }
    delay(1);
    return;
  }

  // Read incoming UART data
  while (Serial1.available() > 0) {
    int byteIn = Serial1.read();
    if (byteIn < 0) break;

    if (static_cast<uint8_t>(byteIn) == 0xFF) {
      consecutiveFF++;
      Serial.printf("[RX] 0xFF (count: %d/5)\n", consecutiveFF);
      if (consecutiveFF >= 5) {
        digitalWrite(OUTPUT_PIN, HIGH);
        outputActive = true;
        outputStartTime = millis();
        lastDebugTime = millis();
        consecutiveFF = 0; // Reset to avoid retriggering immediately
        Serial.printf("[TRIGGER] 5x 0xFF detected! GPIO12 -> HIGH for %lums\n\n", OUTPUT_DURATION);
      }
    } else {
      if (consecutiveFF > 0) {
        Serial.printf("[RX] 0x%02X (reset counter)\n", static_cast<uint8_t>(byteIn));
      }
      consecutiveFF = 0;
    }
  }

  // Small yield to allow other tasks
  delay(1);
}