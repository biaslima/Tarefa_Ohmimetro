/*
 * Por: Wilton Lacerda Silva
 *    Ohmímetro utilizando o ADC da BitDogLab
 *    Modificado por Anna Beatriz Lima
 *
 * 
 * Neste exemplo, utilizamos o ADC do RP2040 para medir a resistência de um resistor
 * desconhecido, utilizando um divisor de tensão com dois resistores.
 * O resistor conhecido é de 10k ohm e o desconhecido é o que queremos medir.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "ws2812.pio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/bootrom.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 
#define Botao_A 5  
#define botaoB 6

// Matriz de LEDs
#define led_matrix_pin 7
#define NUM_LEDS 25
#define IS_RGBW false
uint32_t leds[NUM_LEDS];

int R_conhecido = 10000;   // Resistor de 10k ohm
float R_x = 0.0;           // Resistor desconhecido
float ADC_VREF = 3.31;     // Tensão de referência do ADC
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)

//Variáveis cores
char* cor1;
char* cor2;
char* cor3;

//Valores da série E24
float baseE24[] = {
  1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0,
  2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3,
  4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1
};
 
int potencias_10[] = {
  10, 100, 1000, 10000, 100000
};

char* cores[] = { //Cores das faixas
  "Preto",    // 0
  "Marrom",   // 1
  "Vermelho", // 2
  "Laranja",  // 3
  "Amarelo",  // 4
  "Verde",    // 5
  "Azul",     // 6
  "Violeta",  // 7
  "Cinza",    // 8
  "Branco"    // 9
};

//=========================Matriz de LEDs - Funções===========================
uint8_t localizar_led_xy(uint8_t x, uint8_t y) { //Localiza o LED desejado
  return (4 - y) * 5 + x;
}

uint32_t create_color(uint8_t green, uint8_t red, uint8_t blue) { //Cria cor para o LED
  return ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
}

void update_leds(PIO pio, uint sm) { //Atualiza matriz
  for (int i = 0; i < NUM_LEDS; i++) {
      pio_sm_put_blocking(pio, sm, leds[i] << 8u);
  }
}

uint32_t cor_para_rgb(char* nome_cor) { //Conversão de cores para GRB
  if(strcmp(nome_cor, "Preto") == 0) return create_color(0, 0, 0);
  if(strcmp(nome_cor, "Marrom") == 0) return create_color(10, 26, 0);
  if(strcmp(nome_cor, "Vermelho") == 0) return create_color(0, 60, 0);
  if(strcmp(nome_cor, "Laranja") == 0) return create_color(25, 65, 0);
  if(strcmp(nome_cor, "Amarelo") == 0) return create_color(60, 60, 0);
  if(strcmp(nome_cor, "Verde") == 0) return create_color(60, 0, 0);
  if(strcmp(nome_cor, "Azul") == 0) return create_color(0, 0, 60);
  if(strcmp(nome_cor, "Violeta") == 0) return create_color(0, 48, 48);
  if(strcmp(nome_cor, "Cinza") == 0) return create_color(35, 35, 35);
  if(strcmp(nome_cor, "Branco") == 0) return create_color(60, 60, 60);
}

void exibir_faixas(uint32_t leds[NUM_LEDS], char* cor1, char* cor2, char* cor3) { //Exibição de faixas na matriz

  for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = 0;
  }
  
  leds[localizar_led_xy(2, 0)] = create_color(8,8,8); 
  for(int y = 1; y <= 3; y++) {
    leds[localizar_led_xy(0, y)] = create_color(5, 5, 5);
    leds[localizar_led_xy(4, y)] = create_color(5, 5, 5);
  }
  
  // Primeira faixa
  leds[localizar_led_xy(1, 1)] = cor_para_rgb(cor1);
  leds[localizar_led_xy(2, 1)] = cor_para_rgb(cor1);
  leds[localizar_led_xy(3, 1)] = cor_para_rgb(cor1);
  
  // Segunda faixa
  leds[localizar_led_xy(1, 2)] = cor_para_rgb(cor2);
  leds[localizar_led_xy(2, 2)] = cor_para_rgb(cor2);
  leds[localizar_led_xy(3, 2)] = cor_para_rgb(cor2);
  
  // Terceira faixa
  leds[localizar_led_xy(1, 3)] = cor_para_rgb(cor3);
  leds[localizar_led_xy(2, 3)] = cor_para_rgb(cor3);
  leds[localizar_led_xy(3, 3)] = cor_para_rgb(cor3);
  
  leds[localizar_led_xy(2, 4)] = create_color(8, 8, 8);
}

float encontrar_valor_E24 (float resistencia){ //Encontra o valor correspondente a serie E24
  
  float menor_diferenca = 100000;
  float mais_proximo = 0.0; 

  int tamanhoE24 = sizeof(baseE24) / sizeof(baseE24[0]); 
  int tamanho_potencia = sizeof(potencias_10) / sizeof(potencias_10[0]);

  for (int i = 0; i < tamanhoE24 ; i++){
    for (int j = 0; j < tamanho_potencia; j++){
      float valor_aproximado = baseE24[i] * potencias_10[j];
      float diferenca = fabs(resistencia - valor_aproximado);
      if (diferenca < menor_diferenca){
        menor_diferenca = diferenca;
        mais_proximo = valor_aproximado; 
      }
    }
    
  }
  return mais_proximo;
}

void gerar_cores(float valor_E24){ //Descobre a cor correspondente a faixa do resistor
  char str_valor[10];
  sprintf(str_valor, "%.0f", valor_E24);

  int tam = strlen(str_valor);
  int multiplicador = tam - 2;

  char digito1 = str_valor[0];
  char digito2 = str_valor[1];

  int d1 = digito1 - '0'; 
  int d2 = digito2 - '0'; 

  cor1 = cores[d1];
  cor2 = cores[d2];
  cor3 = cores[multiplicador]; 
}

// Trecho para modo BOOTSEL com botão B
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

int main()
{
  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  // Aqui termina o trecho para modo BOOTSEL com botão B

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                        // Pull up the data line
  gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
  ssd1306_t ssd;                                                // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd);                                         // Configura o display
  ssd1306_send_data(&ssd);                                      // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  // Configurações PIO
  PIO pio = pio0;
  int sm = 0;
  uint offset = pio_add_program(pio, &ws2812_program);
  printf("Loaded program at %d\n", offset);

  ws2812_program_init(pio, sm, offset, led_matrix_pin, 800000, IS_RGBW);

    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = 0; 
    }
  update_leds(pio, sm);

  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string
  char str_e24[15]; 

  bool cor = true;


  while (true)
  {
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    float soma = 0.0f;
    for (int i = 0; i < 500; i++){
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

    // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
    float valor_E24 = encontrar_valor_E24(R_x); 
    gerar_cores(valor_E24);

    if (valor_E24 < 1000) {
      sprintf(str_e24, "%.0f Ohm", valor_E24);
    } else if (valor_E24 < 1000000) {
      sprintf(str_e24, "%.1f kOhm", valor_E24 / 1000.0);
    } else {
      sprintf(str_e24, "%.2f MOhm", valor_E24 / 1000000.0);
    }

    sprintf(str_x, "%1.0f", media); // Converte o inteiro em string
    sprintf(str_y, "%1.0f", R_x);   // Converte o float em string

    //  Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor);                          // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
    ssd1306_line(&ssd, 3, 25, 123, 25, cor);           // Desenha uma linha
    ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
    ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);  // Desenha uma string
    ssd1306_draw_string(&ssd, "  Ohmimetro", 10, 28);  // Desenha uma string
    ssd1306_draw_string(&ssd, "ADC", 13, 41);          // Desenha uma string
    ssd1306_draw_string(&ssd, "Resisten.", 50, 41);    // Desenha uma string
    ssd1306_line(&ssd, 44, 37, 44, 60, cor);           // Desenha uma linha vertical
    ssd1306_draw_string(&ssd, str_x, 8, 52);           // Desenha uma string
    ssd1306_draw_string(&ssd, str_y, 59, 52);          // Desenha uma string
    ssd1306_send_data(&ssd);                           // Atualiza o display
    sleep_ms(3000);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    int text_width = strlen(str_e24) * 6; // caractere 6 pixels de largura
    int center_x = (128 - text_width) / 2;
    ssd1306_draw_string(&ssd, "Valor E24:", 29, 24);
    ssd1306_draw_string(&ssd, str_e24, center_x, 36);
    ssd1306_send_data(&ssd);
    sleep_ms(2000);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    //Escreve cores
    ssd1306_draw_string(&ssd, "Cores:", 8, 6);
    ssd1306_draw_string(&ssd, cor1, 30, 20);
    ssd1306_draw_string(&ssd, cor2, 30, 34);
    ssd1306_draw_string(&ssd, cor3, 30, 48);
    
    //Desenha reistor
    ssd1306_rect(&ssd, 18, 100, 7, 12, cor, cor); 
    ssd1306_rect(&ssd, 32, 100, 7, 12, cor, cor);     
    ssd1306_rect(&ssd, 46, 100, 7, 12, cor, cor);     
    ssd1306_line(&ssd, 103, 6, 103, 63, true);
    ssd1306_line(&ssd, 96, 13, 110, 13, true);
    ssd1306_line(&ssd, 96, 63, 110, 63, true);
    ssd1306_line(&ssd, 96, 13, 96, 17, true);
    ssd1306_line(&ssd, 110, 13, 110, 17, true);
    ssd1306_line(&ssd, 96, 58, 96, 63, true);
    ssd1306_line(&ssd, 110, 58, 110, 63, true);
    ssd1306_send_data(&ssd);

    exibir_faixas(leds, cor1, cor2, cor3);
    update_leds(pio, sm);

    sleep_ms(3000);
  }
}