/*********************************************************************
  E220-900T22D + DOIT ESP32 DevKit - Só escuta redes LoRa 915 MHz
  Usa Serial2 → GPIO16 e GPIO17 (NÃO usa TX0/RX0!)
  Monitor Serial 115200 baud - 2025
*********************************************************************/

#include <HardwareSerial.h>

// Serial2 do ESP32 → E220 (pinos corretos!)
HardwareSerial loraSerial(2);

// ==================== PINOS CERTOS ====================
#define LORA_RX_PIN  16   // ESP32 GPIO16 (RX2) → TXD do E220
#define LORA_TX_PIN  17   // ESP32 GPIO17 (TX2) → RXD do E220

// ==================== CONTADORES ====================
int pacotes = 0;
int rssi = -200;
float snr = -99.0;

void setup() {
  // Monitor Serial (este sim usa TX0/RX0 - é normal)
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("\n=== Sniffer LoRa 915 MHz - E220-900T22D ==="));
  Serial.println(F("Usando GPIO16 (RX) e GPIO17 (TX) - Serial2"));
  Serial.println(F("M0 e M1 ligados no GND → modo escuta"));
  Serial.println(F("Aguardando pacotes...\n"));

  // Inicia comunicação com o E220 nos pinos certos
  loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

  // Configuração única: 915 MHz, 22 dBm, SF12 (máximo alcance)
  uint8_t config[] = {0xC0, 0x00, 0x00, 0x1A, 0x17, 0x44};
  delay(100);
  loraSerial.write(config, sizeof(config));
  delay(300);

  Serial.println(F("E220 configurado com sucesso!"));
  Serial.println(F("915.0 MHz | 22 dBm | SF12 | 125 kHz"));
  Serial.println(F("========================================\n"));
}

void loop() {
  if (loraSerial.available() >= 4) {
    uint8_t header = loraSerial.read();

    // Pacote LoRa recebido
    if (header == 0xC1 || header == 0x00) {
      while (loraSerial.available() < 3);
      loraSerial.read();                      // tamanho (ignora)

      int8_t rssiRaw = loraSerial.read();     // RSSI
      int8_t snrRaw  = loraSerial.read();     // SNR

      rssi = (rssiRaw < 0) ? rssiRaw : rssiRaw - 256;
      snr  = snrRaw * 0.25;
      pacotes++;

      // Mostra imediatamente no Monitor Serial
      Serial.print(F("PACOTE #"));
      Serial.print(pacotes);
      Serial.print(F("  →  RSSI: "));
      Serial.print(rssi);
      Serial.print(F(" dBm"));
      Serial.print(F("  |  SNR: "));
      Serial.print(snr, 1);
      Serial.print(F(" dB"));

      // Barra de sinal bonitinha
      int barras = map(constrain(rssi, -130, -40), -130, -40, 0, 40);
      Serial.print(F("  ["));
      for (int i = 0; i < 40; i++) {
        Serial.print(i < barras ? "█" : "░");
      }
      Serial.println(F("]"));
    }
  }
}