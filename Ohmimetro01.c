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

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 // GPIO para o voltímetro
#define Botao_A 5  // GPIO para botão A


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

char* cores[] = {
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


float encontrar_valor_E24 (float resistencia){
  
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

void gerar_cores(float valor_E24){
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
#include "pico/bootrom.h"
#define botaoB 6
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

  float tensao;
  char str_x[5]; // Buffer para armazenar a string
  char str_y[5]; // Buffer para armazenar a string

  bool cor = true;
  while (true)
  {
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    float soma = 0.0f;
    for (int i = 0; i < 500; i++)
    {
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma / 500.0f;

    // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
    float valor_E24 = encontrar_valor_E24(R_x); 
    gerar_cores(valor_E24);

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

    //Escreve as cores
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
    sleep_ms(3000);
  }
}