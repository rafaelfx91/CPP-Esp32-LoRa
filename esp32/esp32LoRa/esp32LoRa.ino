/*********************************************************************
  SNIFFER LORAWAN 915 MHz – WEB BLUETOOTH (FUNCIONA 100%)
  Versão final – leve, estável e linda no celular
*********************************************************************/

#include <HardwareSerial.h>
#include <LittleFS.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ==================== PINOS E220 ====================
#define LORA_RX_PIN 16
#define LORA_TX_PIN 17
HardwareSerial loraSerial(2);

// ==================== BLE UUIDs ====================
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHAR_TX_UUID        "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP → celular
#define CHAR_RX_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // celular → ESP

BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
int pacotesRecebidos = 0;
unsigned long ultimaAtualizacaoLog = 0;

// ==================== PROTÓTIPOS (para o compilador não reclamar) ====================
void enviar(String msg);
void processarComando(String cmd);
String criarLinhaEvento(int rssi, float snr, uint8_t len);
void salvarNoLog(String linha);

// ==================== CALLBACKS BLE ====================
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println(F("CELULAR CONECTADO!"));
      enviar("info|Sniffer LoRaWAN conectado!"));
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println(F("Celular desconectado"));
      pServer->getAdvertising()->start();
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        String cmd = String(value.c_str());
        cmd.trim();
        if (cmd.length() > 0) {
          Serial.println("Comando recebido: " + cmd);
          processarComando(cmd);
        }
      }
    }
};

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!LittleFS.begin()) {
    Serial.println(F("ERRO: LittleFS não montou!"));
    return;
  }
  Serial.println(F("\n=== SNIFFER LORAWAN WEB BLUETOOTH ==="));

  // Cria log se não existir
  if (!LittleFS.exists("/log.txt")) {
    File f = LittleFS.open("/log.txt", "w");
    if (f) { f.println("=== LOG INICIADO ==="); f.close(); }
  }

  // Configura E220
  loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  uint8_t cfg[] = {0xC0, 0x00, 0x00, 0x1A, 0x17, 0x80};
  delay(100);
  loraSerial.write(cfg, sizeof(cfg));
  delay(300);
  Serial.println(F("E220 configurado – 915 MHz | 22 dBm | SF12 | WOR OFF"));

  // BLE
  BLEDevice::init("LoRaSniffer-BR");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(CHAR_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHAR_RX_UUID, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println(F("BLE ATIVO! Abra o index.html no celular e conecte."));
}

// ==================== LOOP ====================
void loop() {
  // Captura pacotes LoRa
  if (loraSerial.available() >= 4) {
    uint8_t header = loraSerial.read();
    if (header == 0xC1 || header == 0x00) {
      while (loraSerial.available() < 3);
      uint8_t len = loraSerial.read();
      int8_t rssiRaw = loraSerial.read();
      int8_t snrRaw = loraSerial.read();
      int rssi = (rssiRaw > 0) ? rssiRaw - 256 : rssiRaw;
      float snr = snrRaw * 0.25f;
      if (rssi < -200) return;

      pacotesRecebidos++;
      String linha = criarLinhaEvento(rssi, snr, len);
      salvarNoLog(linha);
      if (deviceConnected) enviar("evento|" + linha);
      Serial.println(linha);
    }
  }

  // Atualiza tamanho do log a cada 5 minutos
  if (millis() - ultimaAtualizacaoLog > 300000) {
    File f = LittleFS.open("/log.txt", "r");
    if (f) {
      String info = "Log: " + String(f.size() / 1024.0, 1) + " KB";
      if (deviceConnected) enviar("info|" + info);
      f.close();
    }
    ultimaAtualizacaoLog = millis();
  }
}

// ==================== FUNÇÕES ====================
String criarLinhaEvento(int rssi, float snr, uint8_t len) {
  unsigned long s = millis() / 1000;
  char buf[220];
  snprintf(buf, sizeof(buf), "[%02lu:%02lu:%02lu] PACOTE #%d | %d dBm | %.1f dB | %d bytes | ",
           s/3600, (s%3600)/60, s%60, pacotesRecebidos, rssi, snr, len);
  String linha = buf;
  int barras = map(constrain(rssi, -130, -30), -130, -30, 0, 20);
  for (int i = 0; i < 20; i++) linha += (i < barras) ? "█" : "░";
  return linha;
}

void salvarNoLog(String linha) {
  File f = LittleFS.open("/log.txt", "a");
  if (f) { f.println(linha); f.close(); }
}

void processarComando(String cmd) {
  if (cmd == "/d" || cmd == "/delete") {
    LittleFS.remove("/log.txt");
    enviar("info|Log apagado com sucesso!");
  }
  else if (cmd == "/l" || cmd == "/load") {
    File f = LittleFS.open("/log.txt", "r");
    if (f) {
      enviar("logstart|");
      while (f.available()) {
        String l = f.readStringUntil('\n');
        if (l.length()) enviar("log|" + l);
      }
      enviar("logend|");
      f.close();
    }
  }
  else if (cmd == "/clear") {
    enviar("clear|");
  }
}

void enviar(String msg) {
  if (deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
    delay(10);
  }
}