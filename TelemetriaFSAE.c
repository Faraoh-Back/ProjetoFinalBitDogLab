#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "TelemetriaFSAE.h"

// Variáveis Globais (DEFINIÇÃO)
volatile bool modo_temperatura = true;   // Modo de exibição padrão
char mensagem_recebida[64] = "Aguardando dados...";  // Buffer para mensagens
float temperatura_atual = 0.0f;          // Temperatura atual em °C
struct tcp_pcb *server_pcb = NULL;     // PCB (Protocol Control Block) do servidor

int main() {
    // Inicializa a comunicação padrão de entrada e saída (UART para debug)
    stdio_init_all();
    
    // Configuração inicial dos periféricos: ADC, LEDs, I2C e display OLED
    init_hardware();

    // Tenta conectar ao Wi-Fi com as credenciais definidas
    if(cyw43_arch_init()) {
        printf("Erro ao inicializar Wi-Fi\n");
        return 1;
    }

    // Define o modo de operação Wi-Fi (Station Mode)
    cyw43_arch_enable_sta_mode();

    // Estabelece conexão com a rede Wi-Fi (timeout de 10 segundos)
    printf("Conectando a %s...\n", WIFI_SSID);
    if(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha na conexão\n");
        return 1;
    }
    printf("Conectado!\n");

    // Inicia o servidor TCP na porta definida para receber comandos
    iniciar_servidor_tcp();
    printf("Servidor TCP iniciado na porta %d\n", TCP_PORT);

    // Loop principal de operação do sistema
    while(true) {
        // Atualiza a temperatura e exibe no OLED
        temperatura_atual = ler_temperatura();
        atualizar_display();
        
        // Mantém a conexão Wi-Fi ativa (polling da stack lwIP)
        cyw43_arch_poll();
        
        // Retorna ao modo de exibição de temperatura após 30 segundos sem comandos
        static absolute_time_t last_cmd_time;
        if(!modo_temperatura && absolute_time_diff_us(last_cmd_time, get_absolute_time()) > 30000000) {
            modo_temperatura = true;
            set_led_color(0, 1, 0); // LED verde (modo padrão)
        }
        
        sleep_ms(1000); // Intervalo de atualização de 1 segundo
    }

    // Desinicializa o hardware Wi-Fi (não alcançado em loop infinito)
    cyw43_arch_deinit();
    return 0;
}