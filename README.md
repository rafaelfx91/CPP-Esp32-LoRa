# LoRaWAN Sniffer Portátil – ESP32 + E220-900T22D (915 MHz – Brasil)

![Banner](https://img.shields.io/badge/LoRaWAN-Sniffer-915MHz-blue) ![ESP32](https://img.shields.io/badge/ESP32-DOIT%20DevKit%20V1-green) ![License](https://img.shields.io/github/license/tuunome/seu-repo-name)

Um sniffer LoRaWAN completo, barato e 100 % funcional para a frequência brasileira (915 MHz).  
Capta pacotes reais de redes **The Things Network (TTN)**, **Helium**, redes privadas, sensores IoT, etc.

### Funcionalidades atuais
- Recebe **todos** os pacotes LoRaWAN na faixa 902-928 MHz  
- Mostra em tempo real no Serial Monitor:  
  → Número do pacote  
  → RSSI (força do sinal) + barra visual  
  → SNR  
  → Tamanho do payload  
  → Payload completo em HEX  
- Configuração WOR desabilitada (captura 100 % dos pacotes)  
- Código super comentado e organizado (pronto para estudo ou expansão)

### Hardware necessário
| Componente               | Modelo / Observação                  | Preço médio (2025) |
|--------------------------|---------------------------------------|--------------------|
| ESP32                    | DOIT DevKit V1 (ESP-WROOM-32)         | R$ 35–50          |
| Módulo LoRa              | Ebyte E220-900T22D (LLCC68 – 915 MHz) | R$ 55–70          |
| Antena 915 MHz (opcional)| SMA ou U.FL → SMA                     | R$ 10–25          |
| Display (futuro)         | OLED SH1106 1.3" (I2C)                | R$ 30–45          |

### Conexão (E220 → ESP32)
| E220-900T22D | → | ESP32 DOIT DevKit       |
|--------------|---|-------------------------|
| VCC          | → | 3.3 V                   |
| GND          | → | GND                     |
| TXD          | → | GPIO17 (TX2)            |
| RXD          | → | GPIO16 (RX2)            |
| M0           | → | GND (fio direto)        |
| M1           | → | GND (fio direto)        |
| AUX          | → | deixar solto            |

**Importante:** Desconecte os fios TXD/RXD do E220 antes de fazer upload do código!

### Como usar
1. Clone o repositório
2. Abra no Arduino IDE
3. Selecione a placa **"DOIT ESP32 DevKit V1"**
4. Porta COM correta
5. **Desconecte TXD e RXD do E220**
6. Faça upload
7. **Conecte novamente TXD e RXD**
8. Abra o Monitor Serial em **115200 baud**

Pronto! Você vai ver todos os pacotes LoRaWAN da sua região em tempo real.

### Próximos passos (já em desenvolvimento)
- Versão com OLED SH1106 1.3" (barra de sinal, contador gigante, etc)
- Decodificador automático de pacotes Helium e TTN
- Modo portátil com bateria 18650

### Licença
MIT © 2025 – [Seu Nome ou Nick]

Sinta-se à vontade para fazer fork, melhorar e usar em projetos comerciais ou pessoais!

---
**Feito com carinho no Brasil**  
Se curtiu, deixa uma estrela!  
