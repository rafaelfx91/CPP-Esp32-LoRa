#include <HardwareSerial.h>
HardwareSerial loraSerial(2);

#define LORA_RX_PIN 16
#define LORA_TX_PIN 17

int pacotes = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("\n=== Sniffer LoRaWAN 915 MHz - MODO TRANSPARENTE TOTAL ==="));

  loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

  // CONFIGURAÇÃO CORRETA → DESABILITA WOR e força modo transparente 100%
  uint8_t cfg[] = {0xC0, 0x00, 0x00, 0x1A, 0x17, 0x80};
  //                                     ^^  ^^  ^^
  //                                     1A = 9600 baud
  //                                     17 = canal 23 → 915.0 MHz
  //                                     80 = 22 dBm + SF12 + WOR DESABILITADO ←←← AQUI!

  delay(100);
  loraSerial.write(cfg, sizeof(cfg));
  delay(300);
  Serial.println(F("E220 configurado → WOR OFF | 915 MHz | 22 dBm | SF12"));
  Serial.println(F("Agora vai mostrar TODOS os pacotes com payload!\n"));
}

void loop() {
  if (loraSerial.available() >= 4) {
    uint8_t header = loraSerial.read();

    if (header == 0xC1 || header == 0x00) {
      while (loraSerial.available() < 3);
      uint8_t len     = loraSerial.read();
      int8_t  rssiRaw = loraSerial.read();
      int8_t  snrRaw  = loraSerial.read();

      int rssi = (rssiRaw > 0) ? rssiRaw - 256 : rssiRaw;
      float snr = snrRaw * 0.25f;
      if (rssi < -200) return;

      pacotes++;

      Serial.print(F("PACOTE #")); Serial.print(pacotes);
      Serial.print(F(" | RSSI: ")); Serial.print(rssi); Serial.print(F(" dBm"));
      Serial.print(F(" | SNR: ")); Serial.print(snr, 1); Serial.print(F(" dB"));
      Serial.print(F(" | ")); Serial.print(len); Serial.print(F(" bytes → "));

      for (int i = 0; i < len && loraSerial.available(); i++) {
        uint8_t b = loraSerial.read();
        if (b < 0x10) Serial.print("0");
        Serial.print(b, HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}