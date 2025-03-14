
🌱 GrowMonitor 🌱

O GrowMonitor é um sistema de monitoramento ambiental integrado para cultivos, por enquanto com capacidade para 01 grow.
Ele utiliza sensores para medir temperatura interna, temperatura externa e umidade, exibindo os dados em tempo real em um display LCD. 
Além disso, envia notificações e permite o controle remoto via Telegram e Blynk, atualizando os registros também em planilhas Google.
Com uma interface web intuitiva, o GrowMonitor possibilita configurar alertas e gerenciar dispositivos, como bombas de água, oferecendo um controle completo do ambiente de cultivo.


🚀 Melhorias Planejadas 🚀


Comando no telegram para resetar o sistema
Tela de boas vindas no LCD com o número da versão do firmware


:::::::::::: GrowMonitor Versão VS2 ::::::::::::

🟢 Monitoramento & Otimização
    ✅ (Feito) Integração de múltiplos sensores de umidade do solo 🌱
    ✅ (Feito) Inclusão de sensor de umidade do solo secundário (S12) para medições comparativas 🌾
    ✅ (Feito) Conversão aprimorada dos valores analógicos dos sensores de solo para porcentagens precisas 📊
    ✅ (Feito) Implementação de sistema de atualização OTA (Over-The-Air) 🔄
    ✅ (Feito) Suporte ao sistema de arquivos LittleFS para armazenamento de dados 🗄️
    ✅ (Feito) Monitor Serial mais detalhado e robusto 📟
    ✅ (Feito) Alertas customizáveis para umidade do solo além de temperatura 🌡️
    ✅ (Feito) Reconexão automática aprimorada com Wi-Fi e Blynk 📡
    ✅ (Feito) Comunicação com o Telegram otimizada 🤖
    ✅ (Feito) Envio de alertas ao Telegram com dados completos
    ✅ (Feito) Envio de gráficos via QuickChart 📈
    ✅ (Feito) Envio de dados ao Google Sheets via requisição HTTP POST ☁️
    🔄 Próximos Passos:
    ⏳ Ajuste dinâmico do intervalo de medição
    🔽 Redução automática da frequência de medições quando estável
    🔼 Aumento da frequência quando há mudanças bruscas nos dados 📊
    📌 Detecção e recuperação automática de falhas em sensores, notificando no Telegram
    ⚡ Aprimoramento do sistema OTA com segurança reforçada

🟡 Melhorias na Interface & Controle
    📲 Blynk
    ✅ (Feito) Dashboard atualizado com novos sensores
    ✅ (Feito) Gráficos de histórico de medições expandidos
    🔄 Próximos Passos:
    🎛️ Adicionar botões extras para controle avançado

    📟 LCD
    ✅ (Feito) Atualização para LCD 20x4
    ✅ (Feito) Exibição alternada automática entre diferentes informações
    ✅ (Feito) Melhor visibilidade das medições e alertas
    🔄 Próximos Passos:
    🖥️ Melhorias no layout do LCD para maior clareza

    🌐 Web Interface
    ✅ (Feito) Página Web com exibição dos dados em tempo real
    ✅ (Feito) Implementação de gráficos interativos usando Chart.js 📊
    ✅ (Feito) Controle da bomba de água via interface web 🚰
    ✅ (Feito) Configuração de alertas e limites pela interface 🔧
    ✅ (Feito) Atualização em tempo real usando AJAX/WebSockets 🔄
    ✅ (Feito) Upload de favicon personalizado 🖼️
    🔄 Próximos Passos:
    🎨 Aprimorar o design para maior responsividade
    📋 Implementar histórico detalhado das medições

    🤖 Telegram
    ✅ (Feito) Integração avançada com novos comandos interativos
    ✅ (Feito) Envio de gráficos interativos via QuickChart
    ✅ (Feito) Comandos para configurar limites de temperatura e umidade do solo
    ✅ (Feito) Comando /grafico para visualização do histórico
    ✅ (Feito) Notificações em tempo real com formatação avançada
    🔄 Próximos Passos:
    📌 Adição do comando /status para status completo do sistema
    🚨 Implementação de botão de emergência para desligar dispositivos


🟠 Expansão de Funcionalidades
    ✅ (Feito) Integração com sistema OTA para atualizações sem fio 📶
    ✅ (Feito) Implementação do sistema de arquivos LittleFS para armazenamento 🗂️
    ✅ (Feito) Controle avançado de bomba de água com feedback visual 🚰
    ✅ (Feito) Sincronização de dados com Google Sheets ☁️
    🔄 Próximos Passos:
    🌍 Integração com previsão do tempo online 🌤️
    📈 Coleta de dados climáticos externos para comparação
    🌱 Adição de novos sensores:
    🌾 Sensor de umidade do solo adicional
    💡 Sensor de luminosidade
    🌫️ Sensor de CO₂
    🌍 Aplicativo para celular
    📡 Conectividade MQTT para integração com Home Assistant ou Node-RED
    💾 Banco de Dados Local (SD Card ou SPIFFS) para dados offline









:::::::::::: GrowMonitor Versão vs1.9.9 ::::::::::::

🟢 Monitoramento & Otimização
    ✅ (Feito) Monitor Serial mais informativo e agradável 📟
    ✅ (Feito) Correção do comando /medir no Telegram 📩
    ✅ (Feito) Melhor reconexão automática com Wi-Fi e Blynk 📡
    ✅ (Feito) Comunicação com o Telegram otimizada 🤖
    ✅ (Feito) Alertas inteligentes via Telegram
    ✅ (Feito) Configuração de limites de temperatura e umidade
    ✅ (Feito) Envio de alertas apenas quando necessário 📢
    ✅ (Feito) Acrescentar novo sensor de solo
    ⏲️ Acrescentar novo sensor ambiente
    🔄 Próximos Passos:
    ⏳ Ajuste dinâmico do intervalo de medição
    🔽 Redução automática da frequência de medições quando os valores estão estáveis.
    🔼 Aumento da frequência quando há mudanças bruscas nos dados 📊.
    🔄 Aprimorar ainda mais a reconexão automática com Wi-Fi e Blynk, garantindo melhor estabilidade em falhas de rede.
    📌 Detecção e recuperação automática de falhas em sensores, notificando no Telegram se um sensor parar de responder.
    
🟡 Melhorias na Interface & Controle
    📲 Blynk
    ✅ (Feito) Dashboard no Blynk mais completo
    ✅ (Feito) Gráficos de histórico de medições
    🔄 Próximos Passos:
    🎛️ Adicionar botões extras para controle remoto avançado do sistema (exemplo: definir parâmetros sem precisar do Telegram).
    
    📟 LCD
    ✅ (Feito) Mudança no Tipo de LCD agora com um 20x4
    🔄 Próximos Passos:
    🖥️ Exibição alternada automaticamente entre diferentes informações (exemplo: alternar entre temperatura, umidade, status da bomba).
    🔎 Melhor visibilidade das medições no display, melhorando o contraste e espaçamento.
    ⏲️ Mudar LCD

    🌐 Web Interface
    ✅ (Feito) Página Web para exibição das medições
    ✅ (Feito) Interface acessível via navegador 📱💻
    ✅ (Feito) Configuração de alertas e limites pela interface 🔧
    ✅ (Feito) Controle da bomba pela interface web
    🔄 Próximos Passos:
    🎨 Melhorar o design da página para um layout mais intuitivo e responsivo.
    🔄 Implementar atualização em tempo real (AJAX/WebSockets) para evitar necessidade de recarregar a página.
    📊 Criar gráficos na própria interface web em vez de depender apenas do Image-Charts.
    
    🤖 Telegram
    ✅ (Feito) Menu interativo 📲
    ✅ (Feito) Comando /grafico para visualizar histórico de temperatura/umidade 📊
    🔄 Próximos Passos:
    📌 Botão de emergência para reset do sistema diretamente no Telegram.
    📢 Comando /status para exibir todos os dados da última medição em uma única mensagem.

🟠 Expansão de Funcionalidades
    ✅ (Feito) ☁️ Sincronização com Google Sheets
    ✅ (Feito) Armazenamento das medições na nuvem 📊
    ✅ (Feito) Suporte e integração com GitHub
    ✅ (Feito) Controle da bomba de água 🚰
    🔄 Próximos Passos:
    🌍 Integração com previsão do tempo online 🌤️
    Coletar dados climáticos externos para comparação com as medições locais.
    Ajustar a rega automaticamente de acordo com a umidade e a previsão do tempo.
    🌱 Adição de sensores extras para expandir a funcionalidade:
    🌾 Sensor de umidade do solo → Melhor controle para cultivo inteligente.
    💡 Sensor de luminosidade → Monitoramento do nível de luz na estufa ou ambiente.
    🌫️ Sensor de CO₂ → Para ambientes fechados e controle de qualidade do ar.
    📡 Conectividade MQTT para integração com sistemas de automação como Home Assistant ou Node-RED.
    📈 Banco de Dados Local (SD Card ou ESP32 SPIFFS) para armazenar medições offline caso a conexão falhe.













:::::::::::: GrowMonitor Versão vs1.9.2 ::::::::::::

🟢 Monitoramento & Otimização
    ✅ (Feito) Monitor Serial mais informativo e agradável 📟
    ✅ (Feito) Correção do comando /medir no Telegram 📩
    ✅ (Feito) Melhor reconexão automática com Wi-Fi e Blynk 📡
    ✅ (Feito) Comunicação com o Telegram otimizada 🤖
    ✅ (Feito) Alertas inteligentes via Telegram
    ✅ (Feito) Configuração de limites de temperatura e umidade
    ✅ (Feito) Envio de alertas apenas quando necessário 📢
    🔄 Próximos Passos:
    ⏳ Ajuste dinâmico do intervalo de medição
    🔽 Redução automática da frequência de medições quando os valores estão estáveis.
    🔼 Aumento da frequência quando há mudanças bruscas nos dados 📊.
    🔄 Aprimorar ainda mais a reconexão automática com Wi-Fi e Blynk, garantindo melhor estabilidade em falhas de rede.
    📌 Detecção e recuperação automática de falhas em sensores, notificando no Telegram se um sensor parar de responder.
    
🟡 Melhorias na Interface & Controle
    📲 Blynk
    ✅ (Feito) Dashboard no Blynk mais completo
    ✅ (Feito) Gráficos de histórico de medições
    🔄 Próximos Passos:
    🎛️ Adicionar botões extras para controle remoto avançado do sistema (exemplo: definir parâmetros sem precisar do Telegram).
    
    📟 LCD
    ✅ (Feito) LCD otimizado em relação à versão anterior
    🔄 Próximos Passos:
    🖥️ Exibição alternada automaticamente entre diferentes informações (exemplo: alternar entre temperatura, umidade, status da bomba).
    🔎 Melhor visibilidade das medições no display, melhorando o contraste e espaçamento.

    🌐 Web Interface
    ✅ (Feito) Página Web para exibição das medições
    ✅ (Feito) Interface acessível via navegador 📱💻
    ✅ (Feito) Configuração de alertas e limites pela interface 🔧
    ✅ (Feito) Controle da bomba pela interface web
    🔄 Próximos Passos:
    🎨 Melhorar o design da página para um layout mais intuitivo e responsivo.
    🔄 Implementar atualização em tempo real (AJAX/WebSockets) para evitar necessidade de recarregar a página.
    📊 Criar gráficos na própria interface web em vez de depender apenas do Image-Charts.
    
    🤖 Telegram
    ✅ (Feito) Menu interativo 📲
    ✅ (Feito) Comando /grafico para visualizar histórico de temperatura/umidade 📊
    🔄 Próximos Passos:
    📌 Botão de emergência para reset do sistema diretamente no Telegram.
    📢 Comando /status para exibir todos os dados da última medição em uma única mensagem.

🟠 Expansão de Funcionalidades
    ✅ (Feito) ☁️ Sincronização com Google Sheets
    ✅ (Feito) Armazenamento das medições na nuvem 📊
    ✅ (Feito) Suporte e integração com GitHub
    ✅ (Feito) Controle da bomba de água 🚰
    🔄 Próximos Passos:
    🌍 Integração com previsão do tempo online 🌤️
    Coletar dados climáticos externos para comparação com as medições locais.
    Ajustar a rega automaticamente de acordo com a umidade e a previsão do tempo.
    🌱 Adição de sensores extras para expandir a funcionalidade:
    🌾 Sensor de umidade do solo → Melhor controle para cultivo inteligente.
    💡 Sensor de luminosidade → Monitoramento do nível de luz na estufa ou ambiente.
    🌫️ Sensor de CO₂ → Para ambientes fechados e controle de qualidade do ar.
    📡 Conectividade MQTT para integração com sistemas de automação como Home Assistant ou Node-RED.
    📈 Banco de Dados Local (SD Card ou ESP32 SPIFFS) para armazenar medições offline caso a conexão falhe.







::: GrowMonitor Versão vs1.9 :::

🔹 Melhorias no Monitoramento
✅ (Já feito) Monitor Serial mais informativo e agradável 📟
✅ (Já feito) Correção do comando /medir no Telegram 📩
✅ (Já feito) Melhor reconexão automática com Wi-Fi e Blynk 📡
✅ (Já feito) Comunicação com o Telegram otimizada 🤖
✅ (Já feito) Alertas inteligentes via Telegram
✅ (Já feito) Configuração de limites de temperatura e umidade
✅ (Já feito)     Envio de alertas apenas quando necessário 📢
    ⏳ Ajuste dinâmico do intervalo de medição
    Redução automática da frequência de medições quando estável 🔄
    🔔Aumento quando há mudanças bruscas 📊
    Aprimorar reconexão automática com Wi-Fi e Blynk 📡

🔹 Melhorias na Interface e Controle
Blynk
✅ (Já feito) 📲 Dashboard no Blynk mais completo
✅ (Já feito)     📈 Gráficos de histórico de medições
    🎛️ Botões para controle remoto do sistema
LCD
🎛️ Tela LCD aprimorada
✅ (Já feito)    LCD otimizado em relação a versão anterior
    Exibição alternada automaticamente entre informações
    Melhor visibilidade das medições 🌡️
Web
✅ (Já feito) 🌐 🌟 Nova: Página Web para Exibição das Medições
✅ (Já feito)     Interface acessível via navegador 📱💻
✅ (Já feito)     Exibição de temperatura, umidade e horário da última medição
✅ (Já feito)     Configuração de alertas e limites diretamente pela interface 🔧
✅ (Já feito)     Ligar/desligar bomba pela interface web
    Melhoria no design da página web para um layout mais intuitivo 🎨
    Atualização em tempo real usando AJAX/WebSockets 🔄
Telegram
✅ (Já feito)     Inserir menu
✅ (Já feito)        Gráfico para visualizar histórico de temperatura/umidade 📊

🔹 Expansão de Funcionalidades
🌍 Integração com previsão do tempo online
    Dados externos para comparação com medições locais 🌤️
✅ (Já feito) ☁️ Sincronização com Google Sheets
✅ (Já feito) Armazenamento das medições na nuvem 📊
🌿 Adição de sensores extras
    Sensor de umidade do solo para aplicação em cultivo inteligente 🌾
✅ (Já feito) Bomba de água 🚰
    Sensor de luminosidade









::: GrowMonitor Versão 1.8 :::

🔹 Melhorias no Monitoramento
✅ (Já feito) Monitor Serial mais informativo e agradável 📟
✅ (Já feito) Correção do comando /medir no Telegram 📩
✅ (Já feito) Melhor reconexão automática com Wi-Fi e Blynk 📡
✅ (Já feito) Comunicação com o Telegram otimizada 🤖
✅ (Já feito) Alertas inteligentes via Telegram
✅ (Já feito) Configuração de limites de temperatura e umidade
✅ (Já feito)     Envio de alertas apenas quando necessário 📢
    ⏳ Ajuste dinâmico do intervalo de medição
    Redução automática da frequência de medições quando estável 🔄
    🔔Aumento quando há mudanças bruscas 📊

🔹 Melhorias na Interface e Controle
✅ (Já feito) 📲 Dashboard no Blynk mais completo
✅ (Já feito)     📈 Gráficos de histórico de medições
    🎛️ Botões para controle remoto do sistema
🎛️ Tela LCD aprimorada
    Exibição alternada automaticamente entre informações
    Melhor visibilidade das medições 🌡️
✅ (Já feito)    LCD otimizado em relação a versão anterior
✅ (Já feito) 🌐 🌟 Nova: Página Web para Exibição das Medições
✅ (Já feito)     Interface acessível via navegador 📱💻
✅ (Já feito)     Exibição de temperatura, umidade e horário da última medição
✅ (Já feito)     Configuração de alertas e limites diretamente pela interface 🔧
    Atualização em tempo real usando AJAX/WebSockets 🔄

🔹 Expansão de Funcionalidades
🌍 Integração com previsão do tempo online
    Dados externos para comparação com medições locais 🌤️
✅ (Já feito) ☁️ Sincronização com Google Sheets
✅ (Já feito) Armazenamento das medições na nuvem 📊
🌿 Adição de sensores extras
    Sensor de umidade do solo para aplicação em cultivo inteligente 🌾
    Bomba de água 🚰
    Sensor de luminosidade



::: GrowMonitor Versão 1.7 :::

🔹 Melhorias no Monitoramento
✅ (Já feito) Monitor Serial mais informativo e agradável 📟
✅ (Já feito) Correção do comando /medir no Telegram 📩
✅ (Já feito) Melhor reconexão automática com Wi-Fi e Blynk 📡
✅ (Já feito) Comunicação com o Telegram otimizada 🤖
✅ (Já feito) Alertas inteligentes via Telegram
✅ (Já feito) Configuração de limites de temperatura e umidade
✅ (Já feito)    Envio de alertas apenas quando necessário 📢
    ⏳ Ajuste dinâmico do intervalo de medição
    Redução automática da frequência de medições quando estável 🔄
    🔔Aumento quando há mudanças bruscas 📊

🔹 Melhorias na Interface e Controle
📲 Dashboard no Blynk mais completo
    📈 Gráficos de histórico de medições
    🎛️ Botões para controle remoto do sistema
🎛️ Tela LCD aprimorada
    Exibição alternada automaticamente entre informações
    Melhor visibilidade das medições 🌡️
✅ (Já feito) 🌐 🌟 Nova: Página Web para Exibição das Medições
✅ (Já feito)     Interface acessível via navegador 📱💻
✅ (Já feito)     Exibição de temperatura, umidade e horário da última medição
✅ (Já feito)     Configuração de alertas e limites diretamente pela interface 🔧
    Atualização em tempo real usando AJAX/WebSockets 🔄

🔹 Expansão de Funcionalidades
🌍 Integração com previsão do tempo online
    Dados externos para comparação com medições locais 🌤️
✅ (Já feito) ☁️ Sincronização com Google Sheets
✅ (Já feito) Armazenamento das medições na nuvem 📊
🌿 Adição de sensores extras
    Sensor de umidade do solo para aplicação em cultivo inteligente 🌾
    Bomba de água 🚰
    Sensor de luminosidade

::: GrowMonitor Versão 1.6 :::

🔹 Melhorias no Monitoramento
✅ (Já feito) Monitor Serial mais informativo e agradável 📟
✅ (Já feito) Correção do comando /medir no Telegram 📩
✅ (Já feito) Melhor reconexão automática com Wi-Fi e Blynk 📡
✅ (Já feito) Comunicação com o Telegram otimizada 🤖
✅ (Já feito) Alertas inteligentes via Telegram
✅ (Já feito) Configuração de limites de temperatura e umidade
✅ (Já feito)     Envio de alertas apenas quando necessário 📢
    ⏳ Ajuste dinâmico do intervalo de medição
    Redução automática da frequência de medições quando estável 🔄
    🔔Aumento quando há mudanças bruscas 📊

🔹 Melhorias na Interface e Controle
📲 Dashboard no Blynk mais completo
    📈 Gráficos de histórico de medições
    🎛️ Botões para controle remoto do sistema
🎛️ Tela LCD aprimorada
    Exibição alternada automaticamente entre informações
    Melhor visibilidade das medições 🌡️
🌐 🌟 Nova: Página Web para Exibição das Medições
    Interface acessível via navegador 📱💻
    Exibição de temperatura, umidade e horário da última medição
    Configuração de alertas e limites diretamente pela interface 🔧
    Atualização em tempo real usando AJAX/WebSockets 🔄

🔹 Expansão de Funcionalidades
🌍 Integração com previsão do tempo online
    Dados externos para comparação com medições locais 🌤️
✅ (Já feito) ☁️ Sincronização com Google Sheets
✅ (Já feito) Armazenamento das medições na nuvem 📊
🌿 Adição de sensores extras
    Sensor de umidade do solo para aplicação em cultivo inteligente 🌾
    Bomba de água 🚰
    Sensor de luminosidade






::: GrowMonitor Versão 1.5 :::

🔹 Melhorias no Monitoramento
✅ (Já feito) Monitor Serial mais informativo e agradável 📟
✅ (Já feito) Correção do comando /medir no Telegram 📩
✅ (Já feito) Melhor reconexão automática com Wi-Fi e Blynk 📡
✅ (Já feito) Comunicação com o Telegram otimizada 🤖
    Alertas inteligentes via Telegram
    Configuração de limites de temperatura e umidade
    Envio de alertas apenas quando necessário 📢
    ⏳ Ajuste dinâmico do intervalo de medição
    Redução automática da frequência de medições quando estável 🔄
    🔔Aumento quando há mudanças bruscas 📊

🔹 Melhorias na Interface e Controle
📲 Dashboard no Blynk mais completo
    📈 Gráficos de histórico de medições
    🎛️ Botões para controle remoto do sistema
🎛️ Tela LCD aprimorada
    Exibição alternada automaticamente entre informações
    Melhor visibilidade das medições 🌡️
🌐 🌟 Nova: Página Web para Exibição das Medições
    Interface acessível via navegador 📱💻
    Exibição de temperatura, umidade e horário da última medição
    Configuração de alertas e limites diretamente pela interface 🔧
    Atualização em tempo real usando AJAX/WebSockets 🔄

🔹 Expansão de Funcionalidades
🌍 Integração com previsão do tempo online
    Dados externos para comparação com medições locais 🌤️
☁️ Sincronização com Google Sheets
    Armazenamento das medições na nuvem 📊
🌿 Adição de sensores extras
    Sensor de umidade do solo para aplicação em cultivo inteligente 🌾
    Bomba de água 🚰
    Sensor de luminosidade