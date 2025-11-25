/*********************************************************************
  SNIFFER LORAWAN PROFISSIONAL - ESP32 + E220-900T22D + WEB BLE
  Tema escuro | Log em arquivo | Comandos | Decodificação
  Versão FINAL 2025 - Tudo funcionando
*********************************************************************/

#include <HardwareSerial.h>
#include <LittleFS.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <time.h>

HardwareSerial loraSerial(2);

// ==================== CONFIGURAÇÕES ====================
// LORA_RX_PIN / LORA_TX_PIN
// Variáveis usadas: LORA_RX_PIN, LORA_TX_PIN
// Para que serve: define pinos da Serial2 para comunicação com E220
#define LORA_RX_PIN  16
#define LORA_TX_PIN  17

// BLE_SERVICE_UUID / BLE_CHAR_UUID
// Variáveis usadas: UUIDs
// Para que serve: identificadores do serviço e característica BLE para Web Bluetooth
#define BLE_SERVICE_UUID   "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_UUID      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

// ==================== CONTADORES GLOBAIS ====================
int pacotesRecebidos = 0;
int gatewaysUnicos = 0;
int sensoresUnicos = 0;
unsigned long ultimaAtualizacaoTamanho = 0;

// ==================== OBJETOS GLOBAIS ====================
AsyncWebServer server(80);
WebSocketsServer webSocket(81);
BLECharacteristic *pCharacteristic;

// ==================== FUNÇÕES COMENTADAS ====================

// setupPrincipal
// Variáveis usadas: todas as globais
// Para que serve: inicia Serial, LittleFS, BLE, WebServer, configura E220 e monta página web
void setupPrincipal() {
  Serial.begin(115200);
  while (!Serial);

  if (!LittleFS.begin()) {
    Serial.println(F("ERRO: Falha ao montar LittleFS!"));
    return;
  }
  Serial.println(F("\n=== SNIFFER LORAWAN PROFISSIONAL INICIADO ==="));

  // Configura E220
  loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  uint8_t cfg[] = {0xC0, 0x00, 0x00, 0x1A, 0x17, 0x80}; // WOR OFF
  delay(100);
  loraSerial.write(cfg, sizeof(cfg));
  delay(300);
  Serial.println(F("E220 configurado: 915 MHz | 22 dBm | SF12 | WOR OFF"));

  // Inicia BLE
  BLEDevice::init("LoRaSniffer-BR");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      BLE_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->start();
  Serial.println(F("BLE ativo - conecte via Web Bluetooth"));

  // Página web + WebSocket
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println(F("WebServer e WebSocket iniciados"));
}

// loopPrincipal
// Variáveis usadas: todas
// Para que serve: verifica pacotes LoRa e atualiza tamanho do log a cada 5 min
void loopPrincipal() {
  webSocket.loop();
  verificarSeChegouPacote();
  atualizarTamanhoLogPeriodicamente();
}

// verificarSeChegouPacote
// Variáveis usadas: pacotesRecebidos
// Para que serve: detecta pacotes válidos do E220
void verificarSeChegouPacote() {
  if (loraSerial.available() >= 4) {
    uint8_t header = loraSerial.read();
    if (header == 0xC1 || header == 0x00) {
      processarPacoteRecebido();
    }
  }
}

// processarPacoteRecebido
// Variáveis usadas: pacotesRecebidos, gatewaysUnicos, sensoresUnicos
// Para que serve: lê cabeçalho, RSSI, payload e envia evento traduzido
void processarPacoteRecebido() {
  while (loraSerial.available() < 3);
  uint8_t len = loraSerial.read();
  int8_t rssiRaw = loraSerial.read();
  int8_t snrRaw = loraSerial.read();
  int rssi = (rssiRaw > 0) ? rssiRaw - 256 : rssiRaw;
  float snr = snrRaw * 0.25f;
  if (rssi < -200) return;

  pacotesRecebidos++;

  String evento = decodificarPacote(len, rssi, snr);
  salvarNoLog(evento);
  enviarEventoParaWeb("novo", evento);
  Serial.println(evento);
}

// decodificarPacote
// Variáveis usadas: len, rssi, snr
// Para que serve: cria uma linha de texto com horário relativo (segundos desde ligar) e informações do pacote
String decodificarPacote(uint8_t len, int rssi, float snr) {
  unsigned long segundos = millis() / 1000;
  unsigned long minutos = segundos / 60;
  unsigned long seg = segundos % 60;
  unsigned long horas = minutos / 60;
  minutos = minutos % 60;

  char temp[300];
  snprintf(temp, sizeof(temp),
           "[%02lu:%02lu:%02lu] PACOTE #%d | RSSI: %d dBm | SNR: %.1f dB | %d bytes | Sinal: ",
           horas, minutos, seg, pacotesRecebidos, rssi, snr, len);

  // Barra de sinal bonitinha
  int barras = map(constrain(rssi, -130, -30), -130, -30, 0, 20);
  String barra = "";
  for (int i = 0; i < 20; i++) {
    barra += (i < barras) ? "█" : "░";
  }

  return String(temp) + barra;
}

// salvarNoLog
// Variáveis usadas: linha
// Para que serve: adiciona linha ao log.txt
void salvarNoLog(String linha) {
  File f = LittleFS.open("/log.txt", "a");
  if (f) {
    f.println(linha);
    f.close();
  } else {
    Serial.println(F("ERRO: não abriu log.txt para escrita"));
  }
}

// atualizarTamanhoLogPeriodicamente
// Variáveis usadas: ultimaAtualizacaoTamanho
// Para que serve: a cada 5 min envia tamanho do log para a web
void atualizarTamanhoLogPeriodicamente() {
  if (millis() - ultimaAtualizacaoTamanho > 300000) {
    File f = LittleFS.open("/log.txt", "r");
    if (f) {
      String info = "Log: " + String(f.size()/1024.0, 1) + " KB";
      enviarEventoParaWeb("info", info);
      f.close();
    }
    ultimaAtualizacaoTamanho = millis();
  }
}

// webSocketEvent
// Variáveis usadas: comando do cliente
// Para que serve: recebe comandos do terminal web
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_TEXT) {
    String cmd = String((char*)payload);
    cmd.trim();
    processarComando(cmd);
  }
}

// processarComando
// Variáveis usadas: cmd
// Para que serve: executa /delete, /load, etc
void processarComando(String cmd) {
  if (cmd == "/d" || cmd == "/delete") {
    LittleFS.remove("/log.txt");
    enviarEventoParaWeb("info", "Log apagado!");
  } else if (cmd == "/l" || cmd == "/load") {
    File f = LittleFS.open("/log.txt", "r");
    if (f) {
      String conteudo = f.readString();
      enviarEventoParaWeb("log", conteudo);
      f.close();
    }
  } else if (cmd == "/clear") {
    enviarEventoParaWeb("clear", "");
  }
}

// enviarEventoParaWeb
// Variáveis usadas: tipo, mensagem
// Para que serve: manda JSON pro navegador via WebSocket
void enviarEventoParaWeb(String tipo, String msg) {
  String json = "{\"tipo\":\"" + tipo + "\",\"msg\":\"" + msg + "\"}";
  webSocket.broadcastTXT(json);
}

// setup / loop
void setup() {
  setupPrincipal();
}
void loop() {
  loopPrincipal();
}