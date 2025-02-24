// TelemetriaFSAE.h
#ifndef TELEMETRIA_FSAE_H
#define TELEMETRIA_FSAE_H

// Configurações de Hardware
// ------------------------------
/**
 * Define os pinos e parâmetros críticos para comunicação, sensores e LEDs.
 * - I2C: Comunicação com o display OLED.
 * - LEDs RGB: Controle de status visual.
 * - ADC: Leitura do sensor de temperatura GE-2096.
 * - Wi-Fi: Credenciais de rede para conexão.
 * - Amostragem: Número de leituras do ADC para média móvel.
 * - Segurança: Limite máximo de temperatura permitida.
 */
#define I2C_SDA_PIN 14      // Pino I2C para dados (SDA)
#define I2C_SCL_PIN 15      // Pino I2C para clock (SCL)
#define LED_RED_PIN 13      // Pino do LED vermelho
#define LED_GREEN_PIN 11    // Pino do LED verde
#define LED_BLUE_PIN 12     // Pino do LED azul
#define TEMP_ADC_PIN 28     // Pino ADC para leitura de temperatura
#define WIFI_SSID "Galaxy A71 3708"  // SSID da rede Wi-Fi
#define WIFI_PASS "ufms0428"         // Senha da rede Wi-Fi
#define TCP_PORT 8080       // Porta TCP do servidor
#define NUM_SAMPLES 10       // Número de amostras para média móvel
#define MAX_TEMP 150.0f      // Temperatura máxima permitida (em °C)

// Declaração Variáveis Globais
// ------------------------------
/**
 * Estados e dados compartilhados entre módulos.
 * - `modo_temperatura`: Controla a exibição no OLED (temperatura ou mensagem).
 * - `mensagem_recebida`: Armazena a última mensagem recebida via TCP.
 * - `temperatura_atual`: Valor atualizado da temperatura.
 * - `server_pcb`: Estrutura de controle do servidor TCP.
 */
extern volatile bool modo_temperatura;   // Modo de exibição padrão
extern char mensagem_recebida[64];  // Buffer para mensagens
extern float temperatura_atual;          // Temperatura atual em °C
extern struct tcp_pcb *server_pcb;       // PCB (Protocol Control Block) do servidor

// Protótipos de Funções
// ------------------------------

/**
 * Define a cor do LED RGB conforme os valores booleanos fornecidos.
 * 
 * @param red    Ativa/desativa o LED vermelho.
 * @param green  Ativa/desativa o LED verde.
 * @param blue   Ativa/desativa o LED azul.
 */
void set_led_color(bool red, bool green, bool blue);

/**
 * Configura todos os periféricos: ADC, LEDs, I2C e display OLED.
 * @note Inicializações críticas sem verificações de erro explícitas.
 */
void init_hardware();

/**
 * Lê a temperatura do sensor via ADC, aplica média móvel e converte para °C.
 * @return Temperatura em Celsius, limitada a MAX_TEMP.
 * @warning Fórmula de conversão simplificada (ajustar conforme datasheet).
 */
float ler_temperatura();

/**
 * Atualiza o display OLED com a temperatura ou mensagem recebida.
 * @note Alterna entre modos conforme `modo_temperatura`.
 */
void atualizar_display();

/**
 * Envia a temperatura atual via conexão TCP em formato legível.
 * @param tpcb Estrutura de controle da conexão TCP.
 * @warning Dados copiados para buffer interno (flag TCP_WRITE_FLAG_COPY).
 */
void enviar_dados_tcp(struct tcp_pcb *tpcb);

/**
 * Callback para tratamento de dados recebidos via TCP.
 * @param arg   Argumento genérico.
 * @param tpcb  Estrutura de controle da conexão.
 * @param p     Buffer de dados recebidos.
 * @param err   Código de erro (se aplicável).
 * @return Código de erro do LWIP.
 * @note Atualiza `mensagem_recebida` e responde com temperatura.
 */
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

/**
 * Callback para aceitar novas conexões TCP.
 * @param arg     Argumento genérico.
 * @param newpcb  Nova estrutura de controle da conexão.
 * @param err     Código de erro (se aplicável).
 * @return Código de erro do LWIP.
 * @note Configura `tcp_recv_callback` para a conexão aceita.
 */
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);

/**
 * Inicia o servidor TCP na porta definida (TCP_PORT).
 * @note Utiliza LWIP para criação e gerenciamento do socket.
 */
void iniciar_servidor_tcp();

#endif // TELEMETRIA_FSAE_H