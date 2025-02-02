
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
#include <HTTPClient.h>  // Para fazer requisição HTTP
#include <WebServer.h>

WebServer server(80); // Servidor HTTP na porta 80

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
const unsigned long intervaloMedicao = 60000; // 60 segundos
const unsigned long intervaloVerificacaoTelegram = 2000; // 2 segundos

// ** NOVO: Limite de temperatura para alerta (padrão: 28°C) **
float limiteTemperaturaAlerta = 28.0;

float ultimaTempInterna = 0.0;
float ultimaTempExterna = 0.0;
float ultimaUmidadeExterna = 0.0;
String ultimaHoraMedicao = "";


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
// AGORA: use V3 para o botão
BLYNK_WRITE(V8) {
  // Se for 1 (botão ativado), chama realizarMedicao(true)
  if (param.asInt() == 1) {
    realizarMedicao(true);
  }
}



void configurarComandosTelegram() {
  // Monta um cliente seguro (SSL/TLS)
  telegramClient.reset(new WiFiClientSecure);
  telegramClient->setInsecure(); // Ignora verificação de certificado

  Serial.println("🔧 Configurando comandos do Telegram via setMyCommands (POST)...");

  // Crie a string em formato JSON para o corpo do POST
  // Segue o modelo: { "commands": [ { "command": "...", "description": "..." }, ... ] }
  String jsonBody = F("{\"commands\":["
                        "{\"command\":\"start\",\"description\":\"Inicia o bot\"},"
                        "{\"command\":\"medir\",\"description\":\"Realiza medicao\"},"
                        "{\"command\":\"help\",\"description\":\"Exibe ajuda\"},"
                        "{\"command\":\"alertatemperatura\",\"description\":\"Escreva alertatemperatura xx para muda o limite da temperatura interna\"}"
                      "]}");

  // Tenta conectar no servidor da API do Telegram
  if (telegramClient->connect("api.telegram.org", 443)) {

    // Monta o cabeçalho do POST
    // Importante: usar POST /bot<token>/setMyCommands
    // Enviar Content-Length igual ao tamanho do corpo em JSON
    String request  = String("POST /bot") + botToken + "/setMyCommands HTTP/1.1\r\n" +
                     "Host: api.telegram.org\r\n" +
                     "Content-Type: application/json\r\n" +
                     "Connection: close\r\n" +
                     "Content-Length: " + jsonBody.length() + "\r\n\r\n" +
                     jsonBody; // Aqui anexamos o JSON no final

    // Envia a requisição
    telegramClient->print(request);

    // Lê a resposta (opcional, para debug)
    while (telegramClient->connected() || telegramClient->available()) {
      if (telegramClient->available()) {
        String line = telegramClient->readStringUntil('\n');
        Serial.println(line);
      }
    }

    telegramClient->stop();
    Serial.println("✅ Comandos configurados via API do Telegram (POST).");

  } else {
    Serial.println("❌ Erro ao conectar ao Telegram para configurar comandos.");
  }
}



// Função de callback para lidar com a rota "/salvar"
void handleSave() {
  if (server.hasArg("temp")) {
    String tempStr = server.arg("temp");
    float novoLimite = tempStr.toFloat();
    if (novoLimite > 0) {
      limiteTemperaturaAlerta = novoLimite;
      Serial.println("✅ Novo limite de temperatura: " + String(limiteTemperaturaAlerta, 1));

      // Enviar mensagem ao Telegram notificando a mudança
      String msgTelegram = "⚙️ O limite de temperatura foi atualizado para: " 
                           + String(limiteTemperaturaAlerta, 1) + "°C";
      enviarMensagemTelegram(msgTelegram);
    }
  }
  
  // HTML de resposta ao navegador
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<title>Limite Salvo</title></head><body>";
  page += "<h1>Limite atualizado!</h1>";
  page += "<p>Novo limite de temperatura: " + String(limiteTemperaturaAlerta, 1) + " °C</p>";
  page += "<p><a href='/'>Voltar</a></p>";
  page += "</body></html>";
  
  server.send(200, "text/html", page);
}



void handleRoot() {
  String page = "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'>";
  page += "<title>GrowMonitor WebServer</title></head><body>";

  page += "<h1>GrowMonitor - Status</h1>";
  page += "<p><strong>Temperatura Interna:</strong> " + String(ultimaTempInterna, 1) + " °C</p>";
  page += "<p><strong>Temperatura Externa:</strong> " + String(ultimaTempExterna, 1) + " °C</p>";
  page += "<p><strong>Umidade Externa:</strong> " + String(ultimaUmidadeExterna, 1) + " %</p>";
  page += "<p><strong>Última Medição:</strong> " + ultimaHoraMedicao + "</p>";

  page += "<hr>";
  page += "<h2>Configurar Limite de Temperatura</h2>";
  page += "<form action='/salvar' method='GET'>"; 
  // ou method='POST', mas então usaria server.on("/salvar", handleSave, HTTP_POST);
  
  page += "Novo limite (°C): <input type='number' step='0.1' name='temp' value='" + String(limiteTemperaturaAlerta,1) + "'>";
  page += "<input type='submit' value='Salvar'>";
  page += "</form>";

  page += "</body></html>";

  server.send(200, "text/html", page);
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
  Serial.println("\n✅ Sensores iniciados");

  Serial.print("🔌 Conectando ao Wi-Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi conectado!");

  configurarComandosTelegram();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timeClient.begin();

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);

  // Inicia a rota raiz ("/") chamando handleRoot
  server.on("/", handleRoot);
  server.on("/salvar", handleSave);


  // Inicia o servidor
  server.begin();
  Serial.println("🌐 Servidor web iniciado. Acesse: http://" + WiFi.localIP().toString());

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

  // ** Verificação de alertas **
  String alerta = "";
  if (temperaturaInterna > limiteTemperaturaAlerta) {
    alerta += "🚨 Alerta: Temperatura alta (" + String(temperaturaInterna, 1) + "°C)\n";
  }
  if (umidadeExterna < 20.0) {
    alerta += "🚨 Alerta: Umidade baixa (" + String(umidadeExterna, 1) + "%)\n";
  }

  // ** Atualiza o Serial **
  String mensagemSerial = "🌡️ TI: " + String(temperaturaInterna, 1) + "°C | "
                          "🌡️ TE: " + String(temperaturaExterna, 1) + "°C | "
                          "💧 UE: " + String(umidadeExterna, 1) + "% | "
                          "🕒 Hora: " + horaAtual + "\n" +
                              alerta;

  Serial.println(mensagemSerial);

  // ** Atualiza o LCD **
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("TI:%.1f TE:%.1f", temperaturaInterna, temperaturaExterna);
  lcd.setCursor(0, 1);
  String horaDisplay = horaAtual.substring(0, 5); // Cria uma substring "hh:mm" a partir de horaAtual (ex.: "12:34:56" → "12:34")
  lcd.printf("UE:%.1f %s", umidadeExterna, horaDisplay.c_str()); // Exibe apenas "hh:mm" no LCD

  // ** Atualiza o Blynk **
  conectarBlynk();
  
  // 1) Temperatura Interna numérico
  Blynk.virtualWrite(V0, temperaturaInterna);
  // 2) Temperatura Externa numérico
  Blynk.virtualWrite(V1, temperaturaExterna);
  // 3) Umidade Externa numérico
  Blynk.virtualWrite(V2, umidadeExterna);
  // 4) Hora da medição, usando string
  Blynk.virtualWrite(V4, horaAtual);

  // ** Envio para o Telegram **
  if (forcarEnvioTelegram || millis() - ultimaExecucao >= intervaloMedicao) {
    String mensagemTelegram = "🌡️ Temperatura Interna: " + String(temperaturaInterna, 1) + "°C\n" +
                              "🌡️ Temperatura Externa: " + String(temperaturaExterna, 1) + "°C\n" +
                              "💧 Umidade Externa: " + String(umidadeExterna, 1) + "%\n" +
                              "🕒 Hora: " + horaAtual + "\n" +
                              alerta;
    enviarMensagemTelegram(mensagemTelegram);
    ultimaExecucao = millis();
  }

// Exemplo de envio ao final de realizarMedicao(...)
if (WiFi.status() == WL_CONNECTED) {
  HTTPClient http;
  http.begin(scriptURL); 
  http.addHeader("Content-Type", "application/json");

  // Monte o JSON com os nomes EXACTAMENTE como seu script espera:
  // "temperatura", "umidade" e "temperatura_sensor"
  String postData = "{\"temperatura\":" + String(temperaturaExterna) +
                    ",\"umidade\":" + String(umidadeExterna) +
                    ",\"temperatura_sensor\":" + String(temperaturaInterna) + "}";

  // Envia via POST
  int httpResponseCode = http.POST(postData);
  if (httpResponseCode > 0) {
    Serial.println("🌐 Dados enviados ao Google Sheets com sucesso!");
    String response = http.getString();
    //Serial.println("Resposta do servidor: " + response);
  } else {
    Serial.print("❌ Erro ao enviar ao Google Sheets. Código HTTP: ");
    Serial.println(httpResponseCode);
  }
  http.end();
} else {
  Serial.println("⚠️ Wi-Fi desconectado. Não foi possível enviar ao Google Sheets.");
}

  // Atualiza as variáveis globais
  ultimaTempInterna = temperaturaInterna;
  ultimaTempExterna = temperaturaExterna;
  ultimaUmidadeExterna = umidadeExterna;
  ultimaHoraMedicao = horaAtual;

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
  // Serial.println(resposta); // DEBUG: Habilite se quiser ver a resposta completa

  // 🔹 Buscar o update_id mais recente para evitar mensagens repetidas
  int idPos = resposta.indexOf("\"update_id\":");
  if (idPos != -1) {
    int idStart = idPos + 12;
    int idEnd = resposta.indexOf(",", idStart);
    ultimaMensagemID = resposta.substring(idStart, idEnd).toInt();
  }

  // 🔹 Verifica qual comando foi recebido
  if (resposta.indexOf("\"text\":\"/medir\"") >= 0) {
    Serial.println("✅ Comando /medir detectado! Iniciando medição...");
    realizarMedicao(true);
  }
  else if (resposta.indexOf("\"text\":\"/start\"") >= 0) {
    Serial.println("✅ Comando /start detectado!");
    // Por exemplo, responder via Telegram com uma mensagem de boas-vindas
    enviarMensagemTelegram("Olá! Eu sou o GrowMonitor Bot.\n"
                           "Use /medir para obter medições imediatas ou /help para ajuda.");
  }
  else if (resposta.indexOf("\"text\":\"/help\"") >= 0) {
    Serial.println("✅ Comando /help detectado!");
    // Por exemplo, uma mensagem de ajuda com instruções
    enviarMensagemTelegram("Comandos disponíveis:\n"
                           "/start - Inicia o bot\n"
                           "/medir - Faz uma medição imediata\n"
                           "/alertatemperatura xx - Altera o alerta para temperatura XX\n"
                           "/help - Exibe esta ajuda");
  }
  
  // 🔹 Comando para alterar o limite de temperatura do alerta
  int alertaPos = resposta.indexOf("\"text\":\"/alertatemperatura ");
  if (alertaPos != -1) {
    int start = alertaPos + 26;
    int end = resposta.indexOf("\"", start);
    if (end != -1) {
      String novoLimite = resposta.substring(start, end);
      float novoValor = novoLimite.toFloat();
      if (novoValor > 0) {
        limiteTemperaturaAlerta = novoValor;
        Serial.println("✅ Novo limite de temperatura para alerta: " + String(limiteTemperaturaAlerta) + "°C");
        enviarMensagemTelegram("⚙️ Novo limite de temperatura configurado: " + String(limiteTemperaturaAlerta) + "°C");
      } else {
        Serial.println("❌ Comando inválido. Use /alertatemperatura XX (onde XX é um número)");
        enviarMensagemTelegram("❌ Comando inválido! Use: /alertatemperatura XX (exemplo: /alertatemperatura 30)");
      }
    }
  }
  
  else {
    Serial.println("⚠️ Nenhum comando reconhecido.");
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
  server.handleClient(); // Processa requisições HTTP
}
