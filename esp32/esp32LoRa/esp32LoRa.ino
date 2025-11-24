/*********************************************************************
  E220-900T22D Sniffer LoRaWAN 915 MHz - Versão Final com Payload
  WOR DESABILITADO → mostra TODOS os pacotes reais
  Monitor Serial 115200 baud
*********************************************************************/

#include <HardwareSerial.h>
HardwareSerial loraSerial(2);

// ==================== PINOS ====================
// LORA_RX_PIN
// Variáveis usadas: LORA_RX_PIN
// Para que serve: define o pino RX da Serial2 (GPIO16 → TXD do E220)
#define LORA_RX_PIN  16

// LORA_TX_PIN
// Variáveis usadas: LORA_TX_PIN
// Para que serve: define o pino TX da Serial2 (GPIO17 → RXD do E220)
#define LORA_TX_PIN  17

// ==================== CONTADORES GLOBAIS ====================
int pacotesRecebidos = 0;   // Conta quantos pacotes LoRaWAN foram capturados

// setupPrincipal
// Variáveis usadas: pacotesRecebidos (reset implícito), loraSerial
// Para que serve: inicia comunicação serial, configura o E220 em modo transparente total (WOR OFF) e mostra mensagem de início
void setupPrincipal() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("\n=== Sniffer LoRaWAN 915 MHz - E220-900T22D ==="));
  Serial.println(F("WOR DESABILITADO → todos os pacotes serão mostrados"));
  
  loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

  // Configuração definitiva: 915 MHz + 22 dBm + SF12 + WOR OFF
  uint8_t config[] = {0xC0, 0x00, 0x00, 0x1A, 0x17, 0x80};
  delay(100);
  loraSerial.write(config, sizeof(config));
  delay(300);

  Serial.println(F("Configuração enviada com sucesso!"));
  Serial.println(F("915.0 MHz | 22 dBm | SF12 | WOR = OFF"));
  Serial.println(F("================================================\n"));
}

// loopPrincipal
// Variáveis usadas: pacotesRecebidos
// Para que serve: fica verificando continuamente se chegou pacote LoRa e processa
void loopPrincipal() {
  verificarSeChegouPacote();
}

// verificarSeChegouPacote
// Variáveis usadas: pacotesRecebidos
// Para que serve: lê o buffer do E220 e identifica pacotes válidos (header 0xC1 ou 0x00)
void verificarSeChegouPacote() {
  if (loraSerial.available() >= 4) {
    uint8_t header = loraSerial.read();

    if (header == 0xC1 || header == 0x00) {
      processarPacoteRecebido();
    }
  }
}

// processarPacoteRecebido
// Variáveis usadas: pacotesRecebidos
// Para que serve: lê tamanho, RSSI, SNR e todo o payload, depois chama a função que imprime tudo
void processarPacoteRecebido() {
  while (loraSerial.available() < 3);           // espera o resto do cabeçalho
  uint8_t tamanhoPayload = loraSerial.read();   // tamanho em bytes
  int8_t  rssiRaw        = loraSerial.read();   // RSSI bruto
  int8_t  snrRaw         = loraSerial.read();   // SNR bruto

  // Converte RSSI corretamente
  int rssi = (rssiRaw > 0) ? rssiRaw - 256 : rssiRaw;
  float snr = snrRaw * 0.25f;

  // Ignora valores absurdos (ruído do módulo)
  if (rssi < -200) return;

  pacotesRecebidos++;
  imprimirPacoteCompleto(tamanhoPayload, rssi, snr);
}

// imprimirPacoteCompleto
// Variáveis usadas: pacotesRecebidos, tamanhoPayload, rssi, snr
// Para que serve: mostra no Serial Monitor todas as informações do pacote (número, RSSI, SNR, payload em HEX e barra de sinal)
void imprimirPacoteCompleto(uint8_t tamanho, int rssi, float snr) {
  Serial.print(F("PACOTE #"));
  Serial.print(pacotesRecebidos);
  Serial.print(F(" → RSSI: "));
  Serial.print(rssi);
  Serial.print(F(" dBm | SNR: "));
  Serial.print(snr, 1);
  Serial.print(F(" dB | "));
  Serial.print(tamanho);
  Serial.print(F(" bytes → "));

  // Payload em HEX
  for (int i = 0; i < tamanho && loraSerial.available(); i++) {
    uint8_t byte = loraSerial.read();
    if (byte < 0x10) Serial.print("0");
    Serial.print(byte, HEX);
    Serial.print(" ");
  }

  // Barra de sinal visual
  int barras = map(constrain(rssi, -130, -30), -130, -30, 0, 35);
  Serial.print(F(" ["));
  for (int i = 0; i < 35; i++) {
    Serial.print(i < barras ? "█" : "░");
  }
  Serial.println(F("]"));
}

// ==================== SETUP E LOOP ====================
void setup() {
  setupPrincipal();
}

void loop() {
  loopPrincipal();
}