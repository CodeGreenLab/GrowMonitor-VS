/******************************************************************
 * GrowMonitor VS – Sistema de Monitoramento com LCD 20x4, 
 * conexão via Wi‑Fi, Blynk, Telegram, envio de dados para Google Sheets,
 * controle de bomba via relé e geração de gráficos.
 *
 * Desenvolvido por: CodeGreenLab
 
 ******************************************************************/

// ---------------------------------------------------------------
// INCLUSÃO DE BIBLIOTECAS
// ---------------------------------------------------------------
#include "secrets.h"
#include <Arduino.h>
#include <Wire.h>
//#include <hd44780.h>
//#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <LiquidCrystal_I2C.h>
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


// ---------------------------------------------------------------
// DEFINIÇÕES E CONFIGURAÇÕES DE HARDWARE
// ---------------------------------------------------------------

// Instancia o servidor web na porta 80
WebServer server(80);




// --- Configuração do Sensor DHT11 ---
#define DHTPIN 4                            // Pino de dados do DHT11
#define DHTTYPE DHT11                       // Tipo de sensor: DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- Configuração do Sensor DS18B20 ---
#define ONE_WIRE_BUS 19                     // Pino de dados do sensor DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


//hd44780_I2Cexp lcd;
// --- Configuração do LCD 20x4 via I2C ---
/*
  O endereço do LCD geralmente é 0x27. 
  O LCD possui 20 colunas e 4 linhas.
*/
LiquidCrystal_I2C lcd(0x27, 20, 4);

// --- Configuração dos LEDs indicativos ---
#define LED_VERDE 33                        // LED verde para indicar operação (ex.: medição em andamento)
#define LED_VERMELHO 32                     // LED vermelho para indicar inatividade ou erro


// --- Configuração do Relé para controle da bomba ---
#define RELE_BOMBA 26                       // Pino do ESP32 conectado ao IN1 do módulo relé

#define SOIL_SENSOR_PIN 34  // Pino analógico para o sensor de umidade do solo



// ---------------------------------------------------------------
// CONFIGURAÇÃO DE CONEXÃO E TEMPO
// ---------------------------------------------------------------

// Configuração NTP para obter a hora atual
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);  // Offset de -10800 segundos (UTC-3), atualiza a cada 60 segundos

// ---------------------------------------------------------------
// CONFIGURAÇÃO PARA TELEGRAM
// ---------------------------------------------------------------
std::unique_ptr<WiFiClientSecure> telegramClient;  // Cliente seguro para conexões HTTPS

// ---------------------------------------------------------------
// VARIÁVEIS GLOBAIS E ESTRUTURAS
// ---------------------------------------------------------------
unsigned long ultimaExecucao = 0;                   // Armazena o tempo (em ms) da última medição
unsigned long ultimaVerificacaoTelegram = 0;        // Armazena o tempo da última verificação de comandos no Telegram
const unsigned long intervaloMedicao = 300000;      // Intervalo entre medições (300.000 ms = 300 s)
const unsigned long intervaloVerificacaoTelegram = 1000; // Intervalo de verificação de comandos do Telegram (1 s)
bool bombaLigada = false;                           // Estado atual da bomba (ligada/desligada)
float limiteUmidadeSoloAlerta = 35.0;  // Padrão: 35%


// Estrutura para armazenar uma medição
struct Medicao {
  String tempo;
  float temperaturaInterna;
  float temperaturaExterna;
  float umidade;         // Umidade do ar (DHT11)
  float umidadeSolo;     // NOVO: Umidade do solo
};

// Número máximo de medições a serem armazenadas para o gráfico
const int MAX_MEDICOES = 50;  
Medicao historico[MAX_MEDICOES];  
int indiceMedicao = 0;

// ** NOVO: Limite de temperatura para alerta (padrão: 28°C) **
float limiteTemperaturaAlerta = 28.0;

// Variáveis para armazenar a última medição para exibição
float ultimaTempInterna = 0.0;
float ultimaTempExterna = 0.0;
float ultimaUmidadeExterna = 0.0;
String ultimaHoraMedicao = "";
float ultimaUmidadeSolo = 0.0;



// ---------------------------------------------------------------
// DECLARAÇÃO DAS FUNÇÕES
// ---------------------------------------------------------------
void realizarMedicao(bool forcarEnvioTelegram = false);
void enviarMensagemTelegram(const String& msg, bool usarMarkdown, String modo);
void verificarMensagensTelegram();
void ligarBomba();
void desligarBomba();
void configurarComandosTelegram();  // Configura comandos via API do Telegram
String urlEncode(String s);         // Função para codificar URL (para links)
String gerarLinkGrafico();          // Gera URL do gráfico via QuickChart
void enviarGraficoTelegram();       // Envia link do gráfico via Telegram


// ---------------------------------------------------------------
// FUNÇÕES DE CONEXÃO BLYNK
// ---------------------------------------------------------------
void conectarBlynk() {
  if (!Blynk.connected()) {
    Serial.println("🔄 Reconectando ao Blynk...");
    Blynk.connect();
  }
}

// Função de controle do botão no Blynk (V8)
// Quando o botão é pressionado (valor 1), forçamos uma medição imediata.
BLYNK_WRITE(V8) {
  if (param.asInt() == 1) {
    realizarMedicao(true);
  }
}


// ---------------------------------------------------------------
// FUNÇÃO PARA CODIFICAÇÃO DE URL
// ---------------------------------------------------------------
String urlEncode(String s) {
  String encoded;
  for (int i = 0; i < (int)s.length(); i++){
    char c = s[i];
    // Se o caractere é alfanumérico ou um dos permitidos, mantenha-o
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      // Caso contrário, codifique-o em formato hexadecimal
      encoded += "%" + String((uint8_t)c, HEX);
    }
  }
  return encoded;
}


// ---------------------------------------------------------------
// FUNÇÃO: Gerar Link do Gráfico via QuickChart
// ---------------------------------------------------------------
String gerarLinkGrafico() {
  // Constrói listas de dados e rótulos para o gráfico
  String labels = "";
  String dataTI = "";
  String dataTE = "";
  String dataUE = "";
  String dataSolo = "";
  for (int i = 0; i < indiceMedicao; i++) {
    // Rótulos com os horários das medições
    labels += "\"" + historico[i].tempo + "\"";
    // Dados das medições com 1 casa decimal
    dataTI += String(historico[i].temperaturaInterna, 1);
    dataTE += String(historico[i].temperaturaExterna, 1);
    dataUE += String(historico[i].umidade, 1);
    dataSolo += String(historico[i].umidadeSolo, 1);
    if (i < indiceMedicao - 1) {
      labels += ",";
      dataTI += ",";
      dataTE += ",";
      dataUE += ",";
      dataSolo += ","; // <-- ADICIONE ESTA LINHA
    }
  }

  // Cria o objeto JSON para os datasets
  String config = "{";
  config += "\"type\":\"line\",";
  config += "\"data\":{";
  config += "\"labels\":[" + labels + "],";
  config += "\"datasets\":[";
  config += "{\"label\":\"Temp. Interna\",\"data\":[" + dataTI + "],\"borderColor\":\"red\",\"fill\":false},";
  config += "{\"label\":\"Temp. Externa\",\"data\":[" + dataTE + "],\"borderColor\":\"blue\",\"fill\":false},";
  config += "{\"label\":\"Umidade\",\"data\":[" + dataUE + "],\"borderColor\":\"green\",\"fill\":false},"; // <-- Adicione a vírgula aqui
  config += "{\"label\":\"Umidade do Solo\",\"data\":[" + dataSolo + "],\"borderColor\":\"brown\",\"fill\":false}";
  config += "]";
  config += "},";
  config += "\"options\":{";
  config += "\"title\":{\"display\":true,\"text\":\"GrowMonitor - Medições\"}";
  config += "}";
  config += "}";


  // Encode a configuração e monta a URL do QuickChart
  String url = "https://quickchart.io/chart?c=" + urlEncode(config);
  return url;
}




// ---------------------------------------------------------------
// FUNÇÃO: Enviar Gráfico via Telegram
// ---------------------------------------------------------------
void enviarGraficoTelegram() {
  Serial.println("📊 Gerando link do gráfico...");
  String link = gerarLinkGrafico();
  Serial.println("✅ Link do gráfico gerado: " + link);

  // Cria mensagem HTML com link clicável
  String mensagem = "<a href=\"" + link + "\">Clique aqui para visualizar o gráfico</a>";
  // Envia mensagem com parse_mode HTML para que o link seja clicável
  enviarMensagemTelegram(mensagem, false, "HTML");
}



// ---------------------------------------------------------------
// FUNÇÕES: Ligar/Desligar Bomba
// ---------------------------------------------------------------
void ligarBomba() {
  Serial.println("🔌 Tentando ligar a bomba...");
  digitalWrite(RELE_BOMBA, HIGH); // Liga o relé (ativa a bomba)
  Serial.println("💧 Bomba LIGADA! (GPIO26: " + String(digitalRead(RELE_BOMBA)) + ")");
  enviarMensagemTelegram("💧 A bomba foi LIGADA!", false, "MarkdownV2");
}

void desligarBomba() {
  Serial.println("🔌 Tentando desligar a bomba...");
  digitalWrite(RELE_BOMBA, LOW);  // Desliga o relé (desativa a bomba)
  Serial.println("💧 Bomba DESLIGADA! (GPIO26: " + String(digitalRead(RELE_BOMBA)) + ")");
  enviarMensagemTelegram("💧 A bomba foi DESLIGADA!", false, "MarkdownV2");
}


// ---------------------------------------------------------------
// FUNÇÃO: Configurar Comandos do Telegram via API
// ---------------------------------------------------------------
void configurarComandosTelegram() {
  // Reinicia o cliente seguro e ignora a verificação de certificado
  telegramClient.reset(new WiFiClientSecure);
  telegramClient->setInsecure();

  Serial.println("🔧 Configurando comandos do Telegram via setMyCommands (POST)...");

  // JSON atualizado com os comandos disponíveis
  String jsonBody = F("{\"commands\":[" 
    "{\"command\":\"start\",\"description\":\"Inicia o bot e exibe informações\"},"
    "{\"command\":\"medir\",\"description\":\"Realiza uma medição agora\"},"
    "{\"command\":\"alertatemperatura\",\"description\":\"Define alerta de temperatura\"},"
    "{\"command\":\"alertaumidade\",\"description\":\"Define alerta de umidade do solo\"},"
    "{\"command\":\"bombaligar\",\"description\":\"Liga a bomba d'água\"},"
    "{\"command\":\"bombadesligar\",\"description\":\"Desliga a bomba d'água\"},"
    "{\"command\":\"grafico\",\"description\":\"Exibe gráfico das últimas medições\"},"
    "{\"command\":\"help\",\"description\":\"Exibe a lista de comandos disponíveis\"}"
  "]}");

  // Tenta conectar ao servidor da API do Telegram
  if (telegramClient->connect("api.telegram.org", 443)) {
    // Monta a requisição HTTP POST para configurar os comandos
    String request = String("POST /bot") + botToken + "/setMyCommands HTTP/1.1\r\n" +
                     "Host: api.telegram.org\r\n" +
                     "Content-Type: application/json\r\n" +
                     "Connection: close\r\n" +
                     "Content-Length: " + jsonBody.length() + "\r\n\r\n" +
                     jsonBody;
    telegramClient->print(request);
    // (Opcional) Lê a resposta para debug
    while (telegramClient->connected() || telegramClient->available()) {
      if (telegramClient->available()) {
        String line = telegramClient->readStringUntil('\n');
        //Serial.println(line);
      }
    }
    telegramClient->stop();
    Serial.println("✅ Comandos configurados via API do Telegram (POST).");
  } else {
    Serial.println("❌ Erro ao conectar ao Telegram para configurar comandos.");
  }
}

// ---------------------------------------------------------------
// FUNÇÕES: Handlers do Servidor Web
// ---------------------------------------------------------------

// Handler para a rota "/bomba" – alterna o estado da bomba
void handleBomba() {
  bombaLigada = !bombaLigada;  // Inverte o estado atual da bomba
  if (bombaLigada) {
    ligarBomba();
  } else {
    desligarBomba();
  }
  // Envia resposta HTML com redirecionamento
  server.send(200, "text/html", "<meta http-equiv='refresh' content='1;url=/' />"
                                "<h2>Bomba " + String(bombaLigada ? "LIGADA" : "DESLIGADA") + "</h2>"
                                "<p>Redirecionando...</p>");
}



// Handler para a rota "/salvar" – atualiza o limite de temperatura
void handleSave() {
  if (server.hasArg("temp")) {
    String tempStr = server.arg("temp");
    float novoLimite = tempStr.toFloat();
    if (novoLimite > 0) {
      limiteTemperaturaAlerta = novoLimite;
      Serial.println("✅ Novo limite de temperatura: " + String(limiteTemperaturaAlerta, 1));
      // Notifica via Telegram
      String msgTelegram = "⚙️ O limite de temperatura foi atualizado para: " + String(limiteTemperaturaAlerta, 1) + "°C";
      enviarMensagemTelegram(msgTelegram, false, "MarkdownV2");
    }
  }

  if (server.hasArg("umid")) {
    String umidStr = server.arg("umid");
    float novoLimiteUmidade = umidStr.toFloat();
    if (novoLimiteUmidade >= 0 && novoLimiteUmidade <= 100) {
      limiteUmidadeSoloAlerta = novoLimiteUmidade;
      Serial.println("✅ Novo limite de umidade do solo: " + String(limiteUmidadeSoloAlerta, 1) + "%");
      enviarMensagemTelegram("⚙️ Novo limite de umidade do solo: " + String(limiteUmidadeSoloAlerta, 1) + "%", false, "MarkdownV2");
    }
  }

  // Envia uma página de confirmação para o navegador
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Limite Salvo</title></head><body>";
  page += "<h1>Configuração Atualizada!</h1>";
  page += "<p>Novo limite de temperatura: " + String(limiteTemperaturaAlerta, 1) + " °C</p>";
  page += "<p>Novo limite de umidade do solo: " + String(limiteUmidadeSoloAlerta, 1) + " %</p>";
  page += "<p><a href='/'>Voltar</a></p>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}


// Handler para a rota raiz "/" – exibe status do sistema e formulários de controle
void handleRoot() {
  String page = "<!DOCTYPE html><html lang='pt-BR'><head>";
  page += "<meta charset='UTF-8'><title>GrowMonitor WebServer</title>";

  // Favicon embutido como SVG com o emoji 🌱
  page += "<link rel='icon' type='image/svg+xml' href='data:image/svg+xml,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\"><text y=\".9em\" font-size=\"90\">🌱</text></svg>'>";

  page += "</head><body>";
  
  page += "<h1>GrowMonitor - Status</h1>";
  page += "<p><strong>Temperatura Interna:</strong> " + String(ultimaTempInterna, 1) + " °C</p>";
  page += "<p><strong>Temperatura Externa:</strong> " + String(ultimaTempExterna, 1) + " °C</p>";
  page += "<p><strong>Umidade Externa:</strong> " + String(ultimaUmidadeExterna, 1) + " %</p>";
  page += "<p><strong>Última Medição:</strong> " + ultimaHoraMedicao + "</p>";

  // Exibe status e controle da bomba
  page += "<hr><h2>Controle da Bomba de Água</h2>";
  page += "<p><strong>Status da Bomba:</strong> " + String(bombaLigada ? "<span style='color:green;'>LIGADA</span>" : "<span style='color:red;'>DESLIGADA</span>") + "</p>";
  page += "<form action='/bomba' method='GET'><input type='submit' value='" + String(bombaLigada ? "Desligar" : "Ligar") + " Bomba'></form>";

  // Formulário para configurar o limite de temperatura
  page += "<hr><h2>Configurar Limites</h2>";
  page += "<form action='/salvar' method='GET'>";
  page += "Novo limite de temperatura (°C): <input type='number' step='0.1' name='temp' value='" + String(limiteTemperaturaAlerta, 1) + "'><br>";
  page += "Novo limite de umidade do solo (%): <input type='number' step='0.1' name='umid' value='" + String(limiteUmidadeSoloAlerta, 1) + "'><br>";
  page += "<input type='submit' value='Salvar'></form>";  
  page += "</body></html>";

  server.send(200, "text/html", page);
}


// ---------------------------------------------------------------
// FUNÇÃO: Configurar Hardware e Conexões (Setup)
// ---------------------------------------------------------------
void setup() {
  // Inicializa o monitor serial
  Serial.begin(115200);
  delay(1000);

  // Configura o pino do relé (bomba) como saída e inicia desligado
  pinMode(RELE_BOMBA, OUTPUT);
  digitalWrite(RELE_BOMBA, LOW);

  Serial.println("\n=============================");
  Serial.println("🌱 Iniciando GrowMonitor...");
  Serial.println("=============================\n");

  // Inicializa a comunicação I2C nos pinos SDA=21, SCL=22
  Wire.begin(21, 22);

  // Inicializa o LCD 20x4 com endereço 0x27
  lcd.begin(20, 4, 0x27);
  Serial.println("✅ LCD 20x4 inicializado com sucesso!");
  lcd.backlight();
  lcd.clear();
  lcd.print("Iniciando...");

  pinMode(SOIL_SENSOR_PIN, INPUT);


  // Inicializa os sensores
  dht.begin();
  sensors.begin();
  Serial.println("\n✅ Sensores iniciados");

  // Conecta à rede Wi-Fi
  Serial.print("🔌 Conectando ao Wi-Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi conectado!");

  // Configura os comandos do Telegram
  configurarComandosTelegram();

  // Inicia a conexão com o Blynk e o cliente NTP
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timeClient.begin();

  // Configura os LEDs de indicação
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);

  // Configura as rotas do servidor web
  server.on("/", handleRoot);
  server.on("/salvar", handleSave);
  server.on("/bomba", handleBomba);  // Rota para controle da bomba

  // Inicia o servidor web
  server.begin();
  Serial.println("🌐 Servidor web iniciado. Acesse: http://" + WiFi.localIP().toString());

  // Atualiza o LCD para indicar que o sistema está pronto
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema pronto!");
  lcd.setCursor(0, 1);
  lcd.print("Aguardando...");
  Serial.println("✅ Sistema pronto! Aguardando medições...");
}

// ---------------------------------------------------------------
// FUNÇÃO: Realizar Medição e Atualizar Sistema
// ---------------------------------------------------------------
void realizarMedicao(bool forcarEnvioTelegram) {
  Serial.println("\n📡 Iniciando nova medição...");
  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(LED_VERMELHO, LOW);

  // Solicita e lê os dados dos sensores
  sensors.requestTemperatures();
  float temperaturaInterna = sensors.getTempCByIndex(0);
  float temperaturaExterna = dht.readTemperature();
  float umidadeExterna = dht.readHumidity();

  // Leitura do sensor de umidade do solo (valor analógico de 0 a 4095)
  // Converte para porcentagem (0 a 100%); ajuste os limites conforme sua calibração.
  int rawSoil = analogRead(SOIL_SENSOR_PIN);
  // Conversão adaptada: 3121 -> 0% (solo seco) e 1390 -> 100% (solo úmido)
  float umidadeSolo = 100.0 * (3121 - rawSoil) / (3121 - 1390);
  // Garante que o valor fique entre 0% e 100%
  if (umidadeSolo < 0) umidadeSolo = 0;
  if (umidadeSolo > 100) umidadeSolo = 100;
  

  // Verifica se os dados lidos são válidos
  if (temperaturaInterna == DEVICE_DISCONNECTED_C || isnan(temperaturaExterna) || isnan(umidadeExterna)) {
    Serial.println("❌ Erro: Falha na leitura dos sensores!");
    lcd.clear();
    lcd.print("Erro sensores!");
    delay(2000);
    return;
  }

  // Atualiza o tempo atual via NTP
  timeClient.update();
  String horaAtual = timeClient.getFormattedTime();

  // Armazena a medição no histórico para o gráfico
  if (indiceMedicao < MAX_MEDICOES) {
    historico[indiceMedicao].tempo = horaAtual;
    historico[indiceMedicao].temperaturaInterna = temperaturaInterna;
    historico[indiceMedicao].temperaturaExterna = temperaturaExterna;
    historico[indiceMedicao].umidade = umidadeExterna;
    historico[indiceMedicao].umidadeSolo = umidadeSolo;

    indiceMedicao++;
  } else {
    // Se o histórico estiver cheio, desloca os dados para manter os mais recentes
    for (int i = 1; i < MAX_MEDICOES; i++) {
      historico[i - 1] = historico[i];
    }
    historico[MAX_MEDICOES - 1].tempo = horaAtual;
    historico[MAX_MEDICOES - 1].temperaturaInterna = temperaturaInterna;
    historico[MAX_MEDICOES - 1].temperaturaExterna = temperaturaExterna;
    historico[MAX_MEDICOES - 1].umidade = umidadeExterna;
  }

  // ⚠️ Verifica condições de alerta e adiciona mensagens de aviso
  String alerta = "";
  if (temperaturaInterna > limiteTemperaturaAlerta) {
    alerta += "🚨 Alerta: Temperatura alta (" + String(temperaturaInterna, 1) + "°C)\n";
  }
  if (umidadeExterna < 20.0) {
    alerta += "🚨 Alerta: Umidade baixa (" + String(umidadeExterna, 1) + "%)\n";
  }
  if (umidadeSolo < limiteUmidadeSoloAlerta) {
    alerta += "🚨 Alerta: Solo seco! Umidade em " + String(umidadeSolo, 1) + "%\n";
  }


  // Atualiza o monitor serial com os dados da medição e alertas
  String mensagemSerial = "🌡️ TI: " + String(temperaturaInterna, 1) + "°C | " +
                          "🌡️ TE: " + String(temperaturaExterna, 1) + "°C | " +
                          "💧 UE: " + String(umidadeExterna, 1) + "% | " +
                          "🌱 Solo: " + String(umidadeSolo, 1) + "% | " +
                          "🕒 Hora: " + horaAtual + "\n" + alerta;
      Serial.println(mensagemSerial);


  // Atualiza o LCD 20x4 para exibir os dados:
  lcd.clear();
  // Linha 0: Temperaturas interna e externa
  lcd.setCursor(0, 0);
  lcd.printf("TI:%4.1fC TE:%4.1fC", temperaturaInterna, temperaturaExterna);
  // Linha 1: Umidade do ar e do solo
  lcd.setCursor(0, 1);
  lcd.printf("UE:%4.1f%% So:%4.1f%%", umidadeExterna, umidadeSolo);
  // Linha 2: Horário (hh:mm)
  lcd.setCursor(0, 2);
  lcd.printf("Hora: %s", horaAtual.substring(0, 5).c_str());
  // Linha 3: Indica alerta ou status normal
  lcd.setCursor(0, 3);
  // **Linha 3: Indica alerta ou status normal**
  lcd.setCursor(0, 3);
  if (alerta.length() > 0) {
    lcd.print("ALERTA! Verificar.");
  } else {
    lcd.print("Tudo OK");
  }
  

  // Atualiza os dados enviados via Blynk (v0: TI, v1: TE, v2: UE, v4: Hora)
  conectarBlynk();
  Blynk.virtualWrite(V0, temperaturaInterna);
  Blynk.virtualWrite(V1, temperaturaExterna);
  Blynk.virtualWrite(V2, umidadeExterna);
  Blynk.virtualWrite(V4, horaAtual);

  // Se for para enviar via Telegram (botão pressionado ou tempo decorrido)
  if (forcarEnvioTelegram || millis() - ultimaExecucao >= intervaloMedicao) {
    String mensagemTelegram = "🌡️ Temperatura Interna: " + String(temperaturaInterna, 1) + "°C\n" +
                              "🌡️ Temperatura Externa: " + String(temperaturaExterna, 1) + "°C\n" +
                              "💧 Umidade Externa: " + String(umidadeExterna, 1) + "%\n" +
                              "🌱 Umidade do Solo: " + String(umidadeSolo, 1) + "%\n" +
                              "🕒 Hora: " + horaAtual + "\n" +
                                alerta;
    enviarMensagemTelegram(mensagemTelegram, false, "MarkdownV2");
    ultimaExecucao = millis();
  }

  // Envia os dados para o Google Sheets via requisição HTTP POST
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(scriptURL); 
    http.addHeader("Content-Type", "application/json");

    // Monta o JSON com os campos esperados pelo Apps Script:
    // "temperatura" (TE), "umidade" (UE) e "temperatura_sensor" (TI)
    String postData = "{\"temperatura\":" + String(temperaturaExterna) +
                      ",\"umidade\":" + String(umidadeExterna) +
                      ",\"temperatura_sensor\":" + String(temperaturaInterna) +
                      ",\"umidade_solo\":" + String(umidadeSolo) + "}";
    int httpResponseCode = http.POST(postData);
    if (httpResponseCode > 0) {
      Serial.println("🌐 Dados enviados ao Google Sheets com sucesso!");
      //String response = http.getString(); // (Opcional) para debug
    } else {
      Serial.print("❌ Erro ao enviar ao Google Sheets. Código HTTP: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("⚠️ Wi-Fi desconectado. Não foi possível enviar ao Google Sheets.");
  }

  // Atualiza as variáveis globais com os últimos valores medidos
  ultimaTempInterna = temperaturaInterna;
  ultimaTempExterna = temperaturaExterna;
  ultimaUmidadeExterna = umidadeExterna;
  ultimaHoraMedicao = horaAtual;
  ultimaUmidadeSolo = umidadeSolo;

  // Desliga os LEDs indicativos
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);
}

// ---------------------------------------------------------------
// FUNÇÃO: Enviar Mensagem via Telegram
// ---------------------------------------------------------------
void enviarMensagemTelegram(const String& mensagemIn, bool usarMarkdown, String modo) {
  // Reinicia o cliente seguro e ignora a verificação de certificado SSL
  telegramClient.reset(new WiFiClientSecure);
  telegramClient->setInsecure();

  // Cria uma cópia da mensagem para manipulação
  String msgProcessada = mensagemIn;

  // Se o modo for MarkdownV2, realiza o escape de caracteres especiais
  if (modo == "MarkdownV2") {
    msgProcessada.replace("_",  "\\_");
    msgProcessada.replace("*",  "\\*");
    msgProcessada.replace("[",  "\\[");
    msgProcessada.replace("]",  "\\]");
    msgProcessada.replace("(",  "\\(");
    msgProcessada.replace(")",  "\\)");
    msgProcessada.replace("~",  "\\~");
    msgProcessada.replace("`",  "\\`");
    msgProcessada.replace(">",  "\\>");
    msgProcessada.replace("#",  "\\#");
    msgProcessada.replace("+",  "\\+");
    msgProcessada.replace("-",  "\\-");
    msgProcessada.replace("=",  "\\=");
    msgProcessada.replace("|",  "\\|");
    msgProcessada.replace("{",  "\\{");
    msgProcessada.replace("}",  "\\}");
    msgProcessada.replace(".",  "\\.");
    msgProcessada.replace("!",  "\\!");
  }
  // Se o modo for HTML, codifica a mensagem via URL encoding
  else if (modo == "HTML") {
    msgProcessada = urlEncode(msgProcessada);
  }

  // Tenta conectar ao servidor da API do Telegram (HTTPS na porta 443)
  if (telegramClient->connect("api.telegram.org", 443)) {
    Serial.println("📡 Enviando mensagem ao Telegram...");

    // Define o parse mode conforme o modo desejado
    String parseMode = "";
    if (modo == "MarkdownV2") {
      parseMode = "&parse_mode=MarkdownV2";
    } else if (modo == "HTML") {
      parseMode = "&parse_mode=HTML";
    }

    // Monta a URL final para o GET request
    String url = "/bot" + String(botToken) +
                 "/sendMessage?chat_id=" + String(chatID) +
                 "&text=" + msgProcessada + parseMode;
    
    // Se não for HTML, substitui espaços e quebras de linha para compatibilidade
    if (modo != "HTML") {
      url.replace(" ", "%20");
      url.replace("\n", "%0A");
    }

    // Envia a requisição HTTP
    telegramClient->print(String("GET ") + url + " HTTP/1.1\r\n" +
                            "Host: api.telegram.org\r\n" +
                            "Connection: close\r\n\r\n");

    Serial.println("✅ Mensagem enviada! Aguardando resposta...");

    // Aguarda e lê a resposta do Telegram (opcional para debug)
    String response = "";
    while (telegramClient->connected() || telegramClient->available()) {
      if (telegramClient->available()) {
        response += telegramClient->readStringUntil('\n');
      }
    }
    // Se houver erro na resposta, imprime aviso
    if (response.indexOf("\"ok\":false") != -1) {
      Serial.println("❌ Erro ao enviar mensagem! Verifique o formato ou tokens inválidos.");
    }
    telegramClient->stop();
  } else {
    Serial.println("❌ Erro ao conectar ao Telegram.");
  }
}

// ---------------------------------------------------------------
// FUNÇÃO: Verificar Mensagens e Comandos do Telegram
// ---------------------------------------------------------------
void verificarMensagensTelegram() {
  // Verifica se o intervalo mínimo para checagem passou
  if (millis() - ultimaVerificacaoTelegram < intervaloVerificacaoTelegram) {
    return;
  }
  ultimaVerificacaoTelegram = millis();

  // Reinicia o cliente seguro
  telegramClient.reset(new WiFiClientSecure);
  telegramClient->setInsecure();

  // Tenta conectar ao Telegram
  if (!telegramClient->connect("api.telegram.org", 443)) {
    Serial.println("❌ Erro ao conectar ao Telegram.");
    return;
  }

  Serial.println("\n🔍 Verificando mensagens no Telegram...");

  // Usa um offset para evitar ler as mesmas mensagens novamente
  static long ultimaMensagemID = 0;
  String url = "/bot" + String(botToken) + "/getUpdates?offset=" + String(ultimaMensagemID + 1);
  telegramClient->print(String("GET ") + url + " HTTP/1.1\r\n" +
                        "Host: api.telegram.org\r\n" +
                        "Connection: close\r\n\r\n");

  // Lê a resposta completa
  String resposta = "";
  while (telegramClient->connected() || telegramClient->available()) {
    if (telegramClient->available()) {
      resposta += telegramClient->readStringUntil('\n');
    }
  }

  Serial.println("📩 Resposta recebida do Telegram");
  // (Opcional) Para depuração, descomente a linha abaixo:
  // Serial.println(resposta);

  // Atualiza o offset com base no update_id mais recente
  int idPos = resposta.indexOf("\"update_id\":");
  if (idPos != -1) {
    int idStart = idPos + 12;
    int idEnd = resposta.indexOf(",", idStart);
    ultimaMensagemID = resposta.substring(idStart, idEnd).toInt();
  }

  // Processa os comandos recebidos
  if (resposta.indexOf("\"text\":\"/medir\"") >= 0) {
    Serial.println("✅ Comando /medir detectado! Iniciando medição...");
    realizarMedicao(true);
  }
  else if (resposta.indexOf("\"text\":\"/start\"") >= 0) {
    Serial.println("✅ Comando /start detectado!");
    enviarMensagemTelegram("Oi! Eu sou o GrowMonitor Bot.\nUse /help para ver os comandos possíveis.", false, "MarkdownV2");
  }
  else if (resposta.indexOf("\"text\":\"/help\"") >= 0) {
    Serial.println("✅ Comando /help detectado!");
    String mensagem = "📖 Lista de Comandos:\n\n"
                      "🌡️ Monitoramento:\n"
                      "• /medir - Faz uma medição agora\n"
                      "• /grafico - Exibe gráfico das últimas medições\n\n"
                      "⚙️ Configurações:\n"
                      "• /alertatemperatura XX - Altera alerta de temperatura (exemplo: /alertatemperatura 28)\n"
                      "• /alertaumidade XX - Altera alerta de umidade do solo (exemplo: /alertaumidade 35)\n\n"
                      "💧 Bomba d'água:\n"
                      "• /bombaligar - Liga a bomba\n"
                      "• /bombadesligar - Desliga a bomba\n\n"
                      "📌 Outros:\n"
                      "• /start - Exibe informações do bot\n"
                      "• /help - Exibe esta lista\n";
    enviarMensagemTelegram(mensagem, false, "MarkdownV2");
  }


  // Comandos para controle da bomba
  if (resposta.indexOf("\"text\":\"/bombaligar\"") >= 0) {
    Serial.println("✅ Comando /bombaligar detectado!");
    ligarBomba();
  }
  else if (resposta.indexOf("\"text\":\"/bombadesligar\"") >= 0) {
    Serial.println("✅ Comando /bombadesligar detectado!");
    desligarBomba();
  }

  // Comando para gerar gráfico
  if (resposta.indexOf("\"text\":\"/grafico\"") >= 0) {
    Serial.println("✅ Comando /grafico detectado! Gerando gráfico...");
    enviarGraficoTelegram();
  }

  // Comando para alterar o limite de temperatura do alerta
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
        enviarMensagemTelegram("⚙️ Novo limite de temperatura configurado: " + String(limiteTemperaturaAlerta) + "°C", false, "MarkdownV2");
      } else {
        Serial.println("❌ Comando inválido. Use /alertatemperatura XX (onde XX é um número)");
        enviarMensagemTelegram("❌ Comando inválido! Use: /alertatemperatura XX (exemplo: /alertatemperatura 30)", false, "MarkdownV2");
      }
    }
  }

// 🛠️ Correção: Alterar o limite de umidade do solo via Telegram
int alertaUmidadePos = resposta.indexOf("\"text\":\"/alertaumidade ");
if (alertaUmidadePos != -1) {
    int start = alertaUmidadePos + 23; // Ajuste conforme o comando
    int end = resposta.indexOf("\"", start);
    if (end != -1) {
        String novoLimite = resposta.substring(start, end);
        float novoValor = novoLimite.toFloat();
        if (novoValor >= 0 && novoValor <= 100) {
            limiteUmidadeSoloAlerta = novoValor;
            Serial.println("✅ Novo limite de umidade do solo: " + String(limiteUmidadeSoloAlerta) + "%");
            enviarMensagemTelegram("⚙️ Novo limite de umidade configurado: " + String(limiteUmidadeSoloAlerta) + "%", false, "MarkdownV2");
        } else {
            Serial.println("❌ Valor inválido para umidade do solo!");
            enviarMensagemTelegram("❌ Comando inválido! Use: /alertaumidade XX (exemplo: /alertaumidade 30)", false, "MarkdownV2");
        }
    }
}

  else {
    Serial.println("⚠️ Nenhum comando reconhecido.");
  }

  telegramClient->stop();
}

// ---------------------------------------------------------------
// FUNÇÃO PRINCIPAL LOOP
// ---------------------------------------------------------------
void loop() {
  Blynk.run();                // Processa tarefas do Blynk
  conectarBlynk();            // Verifica e reconecta ao Blynk se necessário
  // Realiza medição se o intervalo passou
  if (millis() - ultimaExecucao >= intervaloMedicao) {
    realizarMedicao();
  }
  verificarMensagensTelegram();  // Checa comandos do Telegram
  server.handleClient();         // Processa requisições do servidor web
}