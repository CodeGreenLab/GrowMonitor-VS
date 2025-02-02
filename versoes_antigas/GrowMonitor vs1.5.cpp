
#include "secrets.h"
#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClientSecure.h>

// Configuração do DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configuração do DS18B20
#define ONE_WIRE_BUS 19
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Configuração do LCD
hd44780_I2Cexp lcd;

// Configuração dos LEDs
#define LED_VERDE 33
#define LED_VERMELHO 32

// Configuração NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);

// Configuração do Telegram
std::unique_ptr<WiFiClientSecure> telegramClient;

// Variáveis Globais
String ultimaMensagemSerial = "";
unsigned long ultimaExecucao = 0;
unsigned long ultimaVerificacaoTelegram = 0;
const unsigned long intervaloMedicao = 30000; // 30 segundos
const unsigned long intervaloVerificacaoTelegram = 2000; // 2 segundos

void realizarMedicao(bool forcarEnvioTelegram = false);
void enviarMensagemTelegram(const String& mensagem);
void verificarMensagensTelegram();

// ** Reconectar ao Blynk se necessário **
void conectarBlynk() {
  if (!Blynk.connected()) {
    Serial.println("🔄 Reconectando ao Blynk...");
    Blynk.connect();
  }
}

// ** Controle via botão no Blynk **
BLYNK_WRITE(V2) {
  realizarMedicao(true);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=============================");
  Serial.println("🌱 Iniciando GrowMonitor...");
  Serial.println("=============================\n");

  if (lcd.begin(16, 2)) {
    Serial.println("❌ Erro: Não foi possível inicializar o LCD!");
    while (1);
  }
  lcd.backlight();
  lcd.print("Iniciando...");

  dht.begin();
  sensors.begin();

  Serial.print("🔌 Conectando ao Wi-Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi conectado!");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timeClient.begin();

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);

  Serial.println("✅ Sistema pronto! Aguardando medições...");
}

void realizarMedicao(bool forcarEnvioTelegram) {
  Serial.println("\n📡 Iniciando nova medição...");
  
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(LED_VERMELHO, LOW);

  sensors.requestTemperatures();
  float temperaturaInterna = sensors.getTempCByIndex(0);
  float temperaturaExterna = dht.readTemperature();
  float umidadeExterna = dht.readHumidity();

  if (temperaturaInterna == DEVICE_DISCONNECTED_C || isnan(temperaturaExterna) || isnan(umidadeExterna)) {
    Serial.println("❌ Erro: Falha na leitura dos sensores!");
    lcd.clear();
    lcd.print("Erro sensores!");
    delay(2000);
    return;
  }

  timeClient.update();
  String horaAtual = timeClient.getFormattedTime();

  // ** Atualiza o Serial **
  String mensagemSerial = "🌡️ TI: " + String(temperaturaInterna, 1) + "°C | "
                          "🌡️ TE: " + String(temperaturaExterna, 1) + "°C | "
                          "💧 UE: " + String(umidadeExterna, 1) + "% | "
                          "🕒 Hora: " + horaAtual;

  Serial.println(mensagemSerial);

  // ** Atualiza o LCD **
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("TI:%.1f TE:%.1f", temperaturaInterna, temperaturaExterna);
  lcd.setCursor(0, 1);
  lcd.printf("UE:%.1f Hora:%s", umidadeExterna, horaAtual.substring(0, 5).c_str());

  // ** Atualiza o Blynk **
  conectarBlynk();
  String dados = "TI: " + String(temperaturaInterna, 1) + "°C, TE: " + 
                 String(temperaturaExterna, 1) + "°C, UE: " + 
                 String(umidadeExterna, 1) + "%";
  Blynk.virtualWrite(V0, dados);
  Blynk.virtualWrite(V1, horaAtual);

  // ** Envia ao Telegram se necessário **
  if (forcarEnvioTelegram || millis() - ultimaExecucao >= intervaloMedicao) {
    

    String mensagemTelegram = "🌡️ Temperatura Interna: " + String(temperaturaInterna, 1) + "°C\n" +
                              "🌡️ Temperatura Externa: " + String(temperaturaExterna, 1) + "°C\n" +
                              "💧 Umidade Externa: " + String(umidadeExterna, 1) + "%\n" +
                              "🕒 Hora: " + horaAtual;

    enviarMensagemTelegram(mensagemTelegram);
    ultimaExecucao = millis();
  } else {
    Serial.println("⏳ Aguardando tempo mínimo antes de enviar nova mensagem ao Telegram...");
  }

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);
}

void enviarMensagemTelegram(const String& mensagem) {
  telegramClient.reset(new WiFiClientSecure);
  telegramClient->setInsecure();

  if (telegramClient->connect("api.telegram.org", 443)) {
    Serial.println("📡 Enviando mensagem ao Telegram...");

    String url = "/bot" + String(botToken) + "/sendMessage?chat_id=" + String(chatID) + "&text=" + mensagem;
    url.replace(" ", "%20");
    url.replace("\n", "%0A");

    telegramClient->print(String("GET ") + url + " HTTP/1.1\r\n" +
                          "Host: api.telegram.org\r\n" +
                          "Connection: close\r\n\r\n");

    Serial.println("✅ Mensagem enviada!");
    telegramClient->stop();
  }
}

void verificarMensagensTelegram() {
  if (millis() - ultimaVerificacaoTelegram < intervaloVerificacaoTelegram) {
    return;
  }
  ultimaVerificacaoTelegram = millis();

  telegramClient.reset(new WiFiClientSecure);
  telegramClient->setInsecure();

  if (!telegramClient->connect("api.telegram.org", 443)) {
    Serial.println("❌ Erro ao conectar ao Telegram.");
    return;
  }

  Serial.println("\n🔍 Verificando mensagens no Telegram...");

  static long ultimaMensagemID = 0;
  String url = "/bot" + String(botToken) + "/getUpdates?offset=" + String(ultimaMensagemID + 1);

  telegramClient->print(String("GET ") + url + " HTTP/1.1\r\n" +
                        "Host: api.telegram.org\r\n" +
                        "Connection: close\r\n\r\n");

  String resposta = "";
  while (telegramClient->connected() || telegramClient->available()) {
    if (telegramClient->available()) {
      resposta += telegramClient->readStringUntil('\n');
    }
  }

  Serial.println("📩 Resposta recebida do Telegram");
  //Serial.println(resposta);

  // 🔹 Buscar o update_id mais recente para evitar mensagens repetidas
  int idPos = resposta.indexOf("\"update_id\":");
  if (idPos != -1) {
    int idStart = idPos + 12;
    int idEnd = resposta.indexOf(",", idStart);
    ultimaMensagemID = resposta.substring(idStart, idEnd).toInt();
  }

  // 🔹 Verificar se o comando "/medir" está na resposta do Telegram
  if (resposta.indexOf("\"text\":\"/medir\"") >= 0) {
    Serial.println("✅ Comando /medir detectado! Iniciando medição...");
    realizarMedicao(true);
  } else {
    Serial.println("⚠️ Nenhum comando /medir encontrado.");
  }

  telegramClient->stop();
}


void loop() {
  Blynk.run();
  conectarBlynk();
  if (millis() - ultimaExecucao >= intervaloMedicao) {
    realizarMedicao();
  }
  verificarMensagensTelegram();
}
