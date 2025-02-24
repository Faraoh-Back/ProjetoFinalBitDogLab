#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "TelemetriaFSAE.h"


// Define a cor do LED RGB conforme os valores booleanos fornecidos (vermelho, verde, azul).
void set_led_color(bool red, bool green, bool blue) {
    gpio_put(LED_RED_PIN, red);
    gpio_put(LED_GREEN_PIN, green);
    gpio_put(LED_BLUE_PIN, blue);
}

// Define a cor do LED RGB conforme os valores booleanos fornecidos (vermelho, verde, azul).
void init_hardware() {
    // Configuração do ADC para o sensor GE-2096
    adc_init();
    adc_gpio_init(TEMP_ADC_PIN);
    adc_select_input(2);  // Canal ADC2 (GPIO28)

    // Configura LED RGB
    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    set_led_color(0, 0, 0);

    // Inicializa Display OLED
    i2c_init(i2c1, 400000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    ssd1306_init();
}

// Lê a temperatura do sensor GE-2096 via ADC, aplica média móvel e converte o valor para Celsius.
// Retorna a temperatura limitada ao valor máximo definido por MAX_TEMP.
float ler_temperatura() {
    const float conversion_factor = 3.3f / (1 << 12);
    uint32_t sum = 0;

    // Leitura com média móvel
    for(int i = 0; i < NUM_SAMPLES; i++) {
        sum += adc_read();
        sleep_us(100);
    }
    
    uint16_t raw = sum / NUM_SAMPLES;
    float voltage = raw * conversion_factor;
    
    // Fórmula de conversão (ajustar conforme datasheet)
    float temp = voltage * 100.0f;  // Exemplo: 10mV/°C
    
    // Limite de segurança
    return (temp > MAX_TEMP) ? MAX_TEMP : temp;
}

// Atualiza o display OLED exibindo a temperatura atual ou a última mensagem recebida via TCP.
// Alterna entre os modos de exibição conforme a variável `modo_temperatura`.
void atualizar_display() {
    static uint8_t buffer[ssd1306_buffer_length];
    memset(buffer, 0, sizeof(buffer));

    if(modo_temperatura) {
        char temp_str[20];
        snprintf(temp_str, sizeof(temp_str), "Temp: %.1fC", temperatura_atual);
        ssd1306_draw_string(buffer, 5, 0, temp_str);
    } else {
        ssd1306_draw_string(buffer, 5, 0, mensagem_recebida);
    }

    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    render_on_display(buffer, &area);
}

// Envia a temperatura atual via conexão TCP em formato de texto (ex: "Temperatura: 25.5°C").
void enviar_dados_tcp(struct tcp_pcb *tpcb) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Temperatura: %.1f°C\n", temperatura_atual);
    tcp_write(tpcb, buffer, strlen(buffer), TCP_WRITE_FLAG_COPY);
}

// Callback chamado ao receber dados TCP. Armazena a mensagem recebida e responde com a temperatura atual.
// Ativa o LED vermelho e desativa o modo de exibição de temperatura.
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if(p != NULL) {
        // Processa mensagem recebida
        size_t len = p->len > sizeof(mensagem_recebida)-1 ? sizeof(mensagem_recebida)-1 : p->len;
        strncpy(mensagem_recebida, p->payload, len);
        mensagem_recebida[len] = '\0';
        
        // Resposta automática com temperatura
        enviar_dados_tcp(tpcb);
        
        modo_temperatura = false;
        set_led_color(1, 0, 0); //RED
        pbuf_free(p);
    }
    return ERR_OK;
}

// Callback para aceitar novas conexões TCP. Configura a função de recebimento de dados.
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_recv_callback);
    return ERR_OK;
}


// Inicia o servidor TCP na porta definida por TCP_PORT, preparando-o para aceitar conexões.
void iniciar_servidor_tcp() {
    server_pcb = tcp_new();
    tcp_bind(server_pcb, IP_ADDR_ANY, TCP_PORT);
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, tcp_accept_callback);
}