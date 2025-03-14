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
#include <LiquidCrystal_I2C.h>    // Biblioteca para LCD via I2C
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>           // Para fazer requisição HTTP
#include <WebServer.h>
#include "FS.h"
#include "LittleFS.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>  // Adicione essa linha junto aos outros includes

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

// --- Configuração do LCD 20x4 via I2C ---
// O endereço do LCD geralmente é 0x27. O display possui 20 colunas e 4 linhas.
LiquidCrystal_I2C lcd(0x27, 20, 4);

// --- Configuração dos LEDs indicativos ---
#define LED_VERDE 33                        // LED verde: medição em andamento
#define LED_VERMELHO 32                     // LED vermelho: inatividade ou erro

// --- Configuração do Relé para controle da bomba ---
#define RELE_BOMBA 26                       // Pino conectado ao IN1 do módulo relé

#define SOIL_SENSOR1_PIN 34  // Pino analógico para o sensor de umidade do solo (Atual)
#define SOIL_SENSOR2_PIN 35  // Pino analógico para o sensor de umidade do solo (S12)

// Função para converter leitura analógica em porcentagem de umidade
float converterParaPorcentagem(int leitura, int seco, int umido) {
  float porcentagem = 100.0 * (seco - leitura) / (seco - umido);
  if (porcentagem < 0) porcentagem = 0;
  if (porcentagem > 100) porcentagem = 100;
  return porcentagem;
}



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

// Controle de alternância de telas no LCD
unsigned long ultimaTrocaTela = 0;
const unsigned long intervaloTrocaTela = 5000; // 5 segundos
int telaAtual = 0; // 0: Temperaturas, 1: Umidades, 2: Status e Alertas



// Variáveis globais para armazenar as leituras dos sensores de solo
float umidadeSolo1 = 0.0;  // Sensor atual
float umidadeSolo2 = 0.0;  // Sensor S12


// Estrutura para armazenar uma medição
struct Medicao {
  String tempo;
  float temperaturaInterna;
  float temperaturaExterna;
  float umidade;         // Umidade do ar (DHT11)
  float umidadeSolo1;     // NOVO: Umidade do solo
  float umidadeSolo2;    // Sensor S12 (NOVO)
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
  String dataSolo1 = "";
  String dataSolo2 = "";
  for (int i = 0; i < indiceMedicao; i++) {
    // Rótulos com os horários das medições
    labels += "\"" + historico[i].tempo + "\"";
    // Dados das medições com 1 casa decimal
    dataTI += String(historico[i].temperaturaInterna, 1);
    dataTE += String(historico[i].temperaturaExterna, 1);
    dataUE += String(historico[i].umidade, 1);
    // Dados das medições com 1 casa decimal
    dataSolo1 += String(historico[i].umidadeSolo1, 1);
    dataSolo2 += String(historico[i].umidadeSolo2, 1);  // Agora usa o histórico correto

    if (i < indiceMedicao - 1) {
      labels += ",";
      dataTI += ",";
      dataTE += ",";
      dataUE += ",";
      dataSolo1 += ",";
      dataSolo2 += ",";
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
  config += "{\"label\":\"Umidade do Solo 1\",\"data\":[" + dataSolo1 + "],\"borderColor\":\"brown\",\"fill\":false},";
  config += "{\"label\":\"Umidade do Solo 2 (S12)\",\"data\":[" + dataSolo2 + "],\"borderColor\":\"orange\",\"fill\":false}";
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


void handleFavicon() {
  File file = LittleFS.open("/favicon.png", "r");
  if (!file) {
      server.send(404, "text/plain", "Favicon não encontrado");
      return;
  }

  server.streamFile(file, "image/png");
  file.close();
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
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GrowMonitor</title>
    <link rel="icon" type="image/png" href="/favicon.png">
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0"></script>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            text-align: center;
            padding: 20px;
        }
        .container {
            max-width: 600px;
            margin: auto;
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #2e7d32;
        }
        .data {
            font-size: 18px;
            margin: 10px 0;
        }
        .button {
            display: inline-block;
            padding: 10px 20px;
            margin: 10px;
            font-size: 16px;
            color: white;
            background-color: #2e7d32;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        .button.red {
            background-color: #d32f2f;
        }
        .button:hover {
            opacity: 0.8;
        }
        canvas {
            width: 100% !important;
            height: 300px !important;
            max-width: 600px;
            border: 1px solid #ccc; /* Adiciona borda para visualização */
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌱 GrowMonitor - Status</h1>
        <p class="data">🌡️ Temperatura Interna: <span id="tempInterna">--</span> °C</p>
        <p class="data">🌡️ Temperatura Externa: <span id="tempExterna">--</span> °C</p>
        <p class="data">💧 Umidade Externa: <span id="umidadeExterna">--</span> %</p>
        <p class="data">🌱 Umidade do Solo (Sensor Atual): <span id="umidadeSolo1">--</span> %</p>
        <p class="data">🌱 Umidade do Solo (S12): <span id="umidadeSolo2">--</span> %</p>
        <p class="data">🕒 Última Medição: <span id="horaMedicao">--</span></p>
        
        <h2>Controle da Bomba de Água</h2>
        <button class="button" onclick="toggleBomba()">Alternar Bomba</button>
        <p>Status: <span id="bombaStatus">Desligada</span></p>

        <h2>Configurar Limites de Alerta</h2>
        <form action="/salvar" method="GET">
            <label for="tempAlerta">Limite de Temperatura (°C):</label>
            <input type="number" step="0.1" id="tempAlerta" name="temp" value="">
            <br>
            <label for="umidadeAlerta">Limite de Umidade do Solo (%):</label>
            <input type="number" step="0.1" id="umidadeAlerta" name="umid" value="">
            <br>
            <input class="button" type="submit" value="Salvar">
        </form>
    </div>
    
    <div class="container">
        <h2>📊 Gráficos de Monitoramento</h2>
        <canvas id="chartTemp"></canvas>
        <canvas id="chartUmidade"></canvas>
    </div>
    
<script>
    console.log("📊 Iniciando configuração dos gráficos...");

    let tempLabels = [];
    let tempInternaData = [];
    let tempExternaData = [];
    let umidadeExternaData = [];
    let umidadeSolo1Data = [];
    let umidadeSolo2Data = [];

    // Obtenção dos contextos dos gráficos
    const ctxTempElement = document.getElementById('chartTemp');
    const ctxUmidadeElement = document.getElementById('chartUmidade');

    if (!ctxTempElement || !ctxUmidadeElement) {
        console.error("❌ Não foi possível encontrar os elementos canvas.");
    }

    const ctxTemp = ctxTempElement.getContext('2d');
    const ctxUmidade = ctxUmidadeElement.getContext('2d');

    if (!ctxTemp || !ctxUmidade) {
        console.error("❌ Falha ao obter o contexto 2D dos canvases.");
    } else {
        console.log("✅ Contextos 2D obtidos com sucesso.");
    }

    // Gráfico de Temperaturas
    const chartTemp = new Chart(ctxTemp, {
        type: 'line',
        data: {
            labels: tempLabels,
            datasets: [
                { label: 'Temp. Interna (°C)', data: tempInternaData, borderColor: 'red', fill: false },
                { label: 'Temp. Externa (°C)', data: tempExternaData, borderColor: 'blue', fill: false }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: { beginAtZero: true }
            }
        }
    });

    // Gráfico de Umidade
    const chartUmidade = new Chart(ctxUmidade, {
        type: 'line',
        data: {
            labels: tempLabels,
            datasets: [
                { label: 'Umidade Externa (%)', data: umidadeExternaData, borderColor: 'green', fill: false },
                { label: 'Umidade Solo 1 (%)', data: umidadeSolo1Data, borderColor: 'brown', fill: false },
                { label: 'Umidade Solo 2 (S12) (%)', data: umidadeSolo2Data, borderColor: 'orange', fill: false }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: { beginAtZero: true }
            }
        }
    });

    // Função para buscar dados do servidor
    async function fetchData() {
        console.log("🔄 Buscando dados do servidor...");
        try {
            const response = await fetch('/dados');
            if (!response.ok) {
                console.error("Erro ao buscar dados: " + response.status);
                return;
            }
            const data = await response.json();
            console.log("✅ Dados recebidos:", data);

            // Atualiza os dados nos elementos HTML
            document.getElementById('tempInterna').innerText = data.tempInterna ?? '--';
            document.getElementById('tempExterna').innerText = data.tempExterna ?? '--';
            document.getElementById('umidadeExterna').innerText = data.umidadeExterna ?? '--';
            document.getElementById('umidadeSolo1').innerText = data.umidadeSolo1 ?? '--';
            document.getElementById('umidadeSolo2').innerText = data.umidadeSolo2 ?? '--';
            document.getElementById('horaMedicao').innerText = data.horaMedicao ?? '--';
            document.getElementById('bombaStatus').innerText = data.bombaLigada ? 'Ligada' : 'Desligada';

            atualizarGraficos(data);
        } catch (error) {
            console.error("Erro ao buscar dados:", error);
        }
    }

    // Função para atualizar os gráficos
    function atualizarGraficos(data) {
        const maxPontos = 50;

        // Adiciona novos dados
        tempLabels.push(data.horaMedicao);
        tempInternaData.push(parseFloat(data.tempInterna));
        tempExternaData.push(parseFloat(data.tempExterna));
        umidadeExternaData.push(parseFloat(data.umidadeExterna));
        umidadeSolo1Data.push(parseFloat(data.umidadeSolo1));
        umidadeSolo2Data.push(parseFloat(data.umidadeSolo2));

        // Remove dados antigos se exceder o limite
        if (tempLabels.length > maxPontos) {
            tempLabels.shift();
            tempInternaData.shift();
            tempExternaData.shift();
            umidadeExternaData.shift();
            umidadeSolo1Data.shift();
            umidadeSolo2Data.shift();
        }

        // Logs para depuração
        console.log("📊 Atualizando gráfico com os seguintes dados:");
        console.log("Horários:", tempLabels);
        console.log("Temp Interna:", tempInternaData);
        console.log("Umidade Solo 1:", umidadeSolo1Data);
        console.log("Umidade Solo 2:", umidadeSolo2Data);

        chartTemp.update();
        chartUmidade.update();
        console.log("✅ Gráficos atualizados.");
    }

    // Controle da Bomba
    function toggleBomba() {
        fetch('/bomba').then(() => fetchData());
    }

    // Atualiza os dados a cada 5 segundos
    setInterval(fetchData, 5000);
    fetchData();
</script>

</body>
</html>
  )rawliteral";

  server.send(200, "text/html", page);
}






void handleDados() {
  String json = "{";
  json += "\"tempInterna\":" + String(ultimaTempInterna, 1) + ",";
  json += "\"tempExterna\":" + String(ultimaTempExterna, 1) + ",";
  json += "\"umidadeExterna\":" + String(ultimaUmidadeExterna, 1) + ",";
  json += "\"umidadeSolo1\":" + String(umidadeSolo1, 1) + ",";  // Sensor atual
  json += "\"umidadeSolo2\":" + String(umidadeSolo2, 1) + ",";  // Sensor S12
  json += "\"horaMedicao\":\"" + ultimaHoraMedicao + "\",";
  json += "\"bombaLigada\":" + String(bombaLigada ? "true" : "false") + ",";
  json += "\"limiteTemperatura\":" + String(limiteTemperaturaAlerta, 1) + ",";
  json += "\"limiteUmidadeSolo\":" + String(limiteUmidadeSoloAlerta, 1);
  json += "}";

  server.send(200, "application/json", json);
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


  if (!LittleFS.begin(true)) {
    Serial.println("❌ Erro ao montar o sistema de arquivos!");
    return;
  }

  Serial.println("✅ Sistema de arquivos montado com sucesso!");


  // Inicializa o LCD 20x4 com endereço 0x27
  lcd.begin(20, 4, 0x27);
  Serial.println("✅ LCD 20x4 inicializado com sucesso!");
  lcd.backlight();
  lcd.clear();
  lcd.print("Iniciando...");

  pinMode(SOIL_SENSOR1_PIN, INPUT);


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

  if (!MDNS.begin("esp32")) {
    Serial.println("Erro ao iniciar mDNS!");
    while (1) { delay(1000); }
  }
  Serial.println("mDNS iniciado com sucesso. Use esp32.local para acessar o dispositivo.");

// Configura OTA
ArduinoOTA.setHostname("meu-esp32");  // Nome do dispositivo na rede
ArduinoOTA.setPassword("admin");      // Senha para atualização OTA (opcional)


// Inicialização do OTA
ArduinoOTA.onStart([]() {
  Serial.println("Iniciando atualização OTA...");
});
ArduinoOTA.onEnd([]() {
  Serial.println("Atualização OTA finalizada.");
});
ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  Serial.printf("Progresso OTA: %u%%\r", (progress * 100) / total);
});
ArduinoOTA.onError([](ota_error_t error) {
  Serial.printf("Erro OTA [%u]: ", error);
  if (error == OTA_AUTH_ERROR) Serial.println("Falha de autenticação");
  else if (error == OTA_BEGIN_ERROR) Serial.println("Falha ao iniciar OTA");
  else if (error == OTA_CONNECT_ERROR) Serial.println("Falha de conexão");
  else if (error == OTA_RECEIVE_ERROR) Serial.println("Erro na recepção");
  else if (error == OTA_END_ERROR) Serial.println("Falha ao finalizar OTA");
});
ArduinoOTA.begin();
Serial.println("OTA iniciado e pronto para atualizações.");
  
  Serial.println("OTA iniciado e pronto para atualizações.");
  

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
  server.on("/dados", handleDados);
  server.on("/favicon.png", HTTP_GET, handleFavicon);


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

  // Envia mensagem ao Telegram indicando que o sistema foi iniciado
  String mensagemInicio = "🚀 GrowMonitor iniciado com sucesso!\n"
                          "📡 Conectado ao Wi-Fi: " + String(ssid) + "\n"
                          "🕒 Sistema em operação e aguardando medições.";

  enviarMensagemTelegram(mensagemInicio, false, "MarkdownV2");


}

// ---------------------------------------------------------------
// FUNÇÃO: Realizar Medição e Atualizar Sistema
// ---------------------------------------------------------------
void realizarMedicao(bool forcarEnvioTelegram) {
  ArduinoOTA.handle();  // ✅ Adicionado aqui
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
  // Leitura dos sensores de umidade do solo
  int rawSoil1 = analogRead(SOIL_SENSOR1_PIN);  // Sensor atual
  int rawSoil2 = analogRead(SOIL_SENSOR2_PIN);  // Sensor S12
  
  Serial.println("🔍 Leituras Analógicas dos Sensores de Solo:");
  Serial.print(" - Sensor 1 (Atual): ");
  Serial.println(rawSoil1);
  Serial.print(" - Sensor 2 (S12): ");
  Serial.println(rawSoil2);



  // Conversão para porcentagem usando as médias aferidas
  umidadeSolo1 = converterParaPorcentagem(rawSoil1, 3208, 1521);  // Sensor atual
  umidadeSolo2 = converterParaPorcentagem(rawSoil2, 3716, 1979);  // Sensor S12
  
  // Garante que o valor fique entre 0% e 100%
  if (umidadeSolo1 < 0) umidadeSolo1 = 0;
  if (umidadeSolo1 > 100) umidadeSolo1 = 100;
  

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
    historico[indiceMedicao].umidadeSolo1 = umidadeSolo1;
    historico[indiceMedicao].umidadeSolo2 = umidadeSolo2;  // Armazena dados do S12
    indiceMedicao++;
  } else {
    for (int i = 1; i < MAX_MEDICOES; i++) {
      historico[i - 1] = historico[i];
    }
    historico[MAX_MEDICOES - 1].tempo = horaAtual;
    historico[MAX_MEDICOES - 1].temperaturaInterna = temperaturaInterna;
    historico[MAX_MEDICOES - 1].temperaturaExterna = temperaturaExterna;
    historico[MAX_MEDICOES - 1].umidade = umidadeExterna;
    historico[MAX_MEDICOES - 1].umidadeSolo1 = umidadeSolo1;
    historico[MAX_MEDICOES - 1].umidadeSolo2 = umidadeSolo2;  // Atualização
  }
  

  // ⚠️ Verifica condições de alerta e adiciona mensagens de aviso
  String alerta = "";
  if (temperaturaInterna > limiteTemperaturaAlerta) {
    alerta += "🚨 Alerta: Temperatura alta (" + String(temperaturaInterna, 1) + "°C)\n";
  }
  if (umidadeExterna < 20.0) {
    alerta += "🚨 Alerta: Umidade baixa (" + String(umidadeExterna, 1) + "%)\n";
  }
  if (umidadeSolo1 < limiteUmidadeSoloAlerta) {
    alerta += "🚨 Alerta: Solo seco! Sensor Atual em " + String(umidadeSolo1, 1) + "%\n";
  }
  if (umidadeSolo2 < limiteUmidadeSoloAlerta) {
    alerta += "🚨 Alerta: Solo seco! Sensor S12 em " + String(umidadeSolo2, 1) + "%\n";
  }
  

  // Atualiza o monitor serial com os dados da medição e alertas
  String mensagemSerial = "🌡️ TI: " + String(temperaturaInterna, 1) + "°C | " +
                          "🌡️ TE: " + String(temperaturaExterna, 1) + "°C | " +
                          "💧 UE: " + String(umidadeExterna, 1) + "% | " +
                          "🌱 Solo 1 (Atual): " + String(umidadeSolo1, 1) + "% | " +
                          "🌱 Solo 2 (S12): " + String(umidadeSolo2, 1) + "% | " +
                          "🕒 Hora: " + horaAtual + "\n" + alerta;
  Serial.println(mensagemSerial);


  // Atualiza os dados enviados via Blynk (v0: TI, v1: TE, v2: UE, v4: Hora)
  conectarBlynk();
  Blynk.virtualWrite(V0, temperaturaInterna);
  Blynk.virtualWrite(V1, temperaturaExterna);
  Blynk.virtualWrite(V2, umidadeExterna);
  Blynk.virtualWrite(V4, horaAtual);

  // Se for para enviar via Telegram (botão pressionado ou tempo decorrido)
  if (forcarEnvioTelegram || millis() - ultimaExecucao >= intervaloMedicao) {
      String mensagemTelegram = "🌡️ Temperatura Interna2: " + String(temperaturaInterna, 1) + "°C\n" +
      "🌡️ Temperatura Externa: " + String(temperaturaExterna, 1) + "°C\n" +
      "💧 Umidade Externa: " + String(umidadeExterna, 1) + "%\n" +
      "🌱 Umidade do Solo (Sensor Atual): " + String(umidadeSolo1, 1) + "%\n" +
      "🌱 Umidade do Solo (S12): " + String(umidadeSolo2, 1) + "%\n" +
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
                      ",\"umidade_solo\":" + String(umidadeSolo1) + "}";
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
  ultimaUmidadeSolo = umidadeSolo1;


  // Desliga os LEDs indicativos
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, HIGH);
}


void atualizarLCD() {
  lcd.clear();
  switch (telaAtual) {
    case 0:
      // Tela de Temperaturas
      lcd.setCursor(0, 0);
      lcd.print("Temp Int: " + String(ultimaTempInterna, 1) + "C");
      lcd.setCursor(0, 1);
      lcd.print("Temp Ext: " + String(ultimaTempExterna, 1) + "C");
      lcd.setCursor(0, 2);
      lcd.print("Hora: " + ultimaHoraMedicao.substring(0, 5));
      break;

    case 1:
      // Tela de Umidades
      lcd.setCursor(0, 0);
      lcd.print("Umid Ext: " + String(ultimaUmidadeExterna, 1) + "%");
      lcd.setCursor(0, 1);
      lcd.print("Solo 1: " + String(umidadeSolo1, 1) + "%");
      lcd.setCursor(0, 2);
      lcd.print("Solo S12: " + String(umidadeSolo2, 1) + "%");
      break;

    case 2:
      // Tela de Status e Alertas
      lcd.setCursor(0, 0);
      lcd.print("Bomba: " + String(bombaLigada ? "Ligada" : "Desligada"));
      lcd.setCursor(0, 1);
      if (ultimaTempInterna > limiteTemperaturaAlerta) {
        lcd.print("! Alerta: Temp Alta");
      } else if (umidadeSolo1 < limiteUmidadeSoloAlerta || umidadeSolo2 < limiteUmidadeSoloAlerta) {
        lcd.print("! Alerta: Solo Seco");
      } else {
        lcd.print("Status: Normal");
      }
      break;
  }
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
  ArduinoOTA.handle();  // ✅ Adicionado
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
  ArduinoOTA.handle();
  //Blynk.run();
  conectarBlynk();
  verificarMensagensTelegram();
  server.handleClient();

  // Alternância automática das telas no LCD
  if (millis() - ultimaTrocaTela >= intervaloTrocaTela) {
    telaAtual = (telaAtual + 1) % 3; // Alterna entre as 3 telas
    atualizarLCD();
    ultimaTrocaTela = millis();
  }

  // Realiza medição se o intervalo passou
  if (millis() - ultimaExecucao >= intervaloMedicao) {
    realizarMedicao();
  }
}
