/*
 * Por: Wilton Lacerda Silva
 *    Comunicação serial com I2C
 *  
 * Uso da interface I2C para comunicação com o Display OLED
 * 
 * Estudo da biblioteca ssd1306 com PicoW na Placa BitDogLab.
 *  
 * Este programa escreve uma mensagem no display OLED.
 * 
 * 
*/


#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/uart.h"
#include "hardware/pio.h" // Biblioteca para manipulação do PIO (Programável I/O)
#include "hardware/clocks.h" // Biblioteca para manipulação de clocks no Pico
#include "hardware/adc.h" // Biblioteca para controle do ADC (Conversor Analógico-Digital)
#include "pico/bootrom.h" // Biblioteca para acesso a funções de boot
#include "pio_matrix.pio.h" 
#include <math.h>  

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define Led_pin_red 13

#define Led_pin_azul 12   // Define o pino GPIO para o LED 
#define Led_pin_verde 11   // Define o pino GPIO para o LED

char comando;

#define Botao_A 5
#define Botao_B 6

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define NUM_PIXELS 25
#define OUT_PIN 7
static void gpio_irq_handler(uint gpio, uint32_t events);
static volatile uint32_t ultimo_evento_A = 0;
static volatile uint32_t ultimo_evento_B = 0;
char comando;
uint32_t matrix_rgb(double b, double r, double g) {
  unsigned char R = r * 255, G = g * 255, B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

double letras[10][25] = {
  {1, 1, 1, 1, 1,  
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1}, // Representação do número 0

  {0, 0, 1, 0, 0,
   0, 0, 1, 1, 0,
   1, 0, 1, 0, 0,
   0, 0, 1, 0, 0,
   1, 1, 1, 1, 1}, // Representação do número 1

  {1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   0, 1, 1, 1, 1,
   0, 0, 0, 0, 1,
   1, 1, 1, 1, 1}, // Representação do número 2

  {1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 1}, // Representação do número 3

  {1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   0, 0, 0, 0, 1}, // Representação do número 4

  {1, 1, 1, 1, 1,
   0, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 1}, // Representação do número 5

  {1, 1, 1, 1, 1,  
   0, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1}, // Representação do número 6

  {1, 1, 1, 1, 1,  
   1, 0, 0, 0, 0,
   0, 0, 0, 1, 0,
   0, 0, 1, 0, 0,
   0, 1, 0, 0, 0}, // Representação do número 7

  {1, 1, 1, 1, 1, 
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1}, // Representação do número 8

  {1, 1, 1, 1, 1,  
   1, 0, 0, 0, 1,
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 1}  // Representação do número 9
};
void inicializar_UART();
void animacao_1(letra);
void att_led();

PIO pio = pio0; 
uint offset;
uint sm;
uint32_t valor_led;

bool estado_led_azul =false;
bool estado_led_verde =false;



int main()
{
  gpio_init(Led_pin_verde);
  gpio_set_dir(Led_pin_verde, GPIO_OUT);
  gpio_init(Led_pin_azul);
  gpio_set_dir(Led_pin_azul, GPIO_OUT);

  // Configuração dos botões como entrada com pull-up
  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);
  gpio_init(Botao_B);
  gpio_set_dir(Botao_B, GPIO_IN);
  gpio_pull_up(Botao_B);

  // I2C Initialisation. Using it at 400Khz.
  stdio_init_all();
  inicializar_UART();
  i2c_init(I2C_PORT, 400 * 1000);


  uint offset = pio_add_program(pio, &pio_matrix_program);
  uint sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, OUT_PIN);

  gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  while (true)
  {
    
    comando = getchar();
    printf("O comando é: %c\n",comando);
    
    att_led();
    // Atualiza o conteúdo do display com animações

    switch (comando){
      case '0':
          animacao_1(0);
          break;
      case '1':
          animacao_1(1);
          break;
      case '2':
          animacao_1(2);
          break;
      case '3':
          animacao_1(3);
          break;
      case '4':
          animacao_1(4);
          break;
      case '5':
          animacao_1(5);
          break;
      case '6':
          animacao_1(6);
          break;
      case '7':
          animacao_1(7);
          break;
      case '8':
          animacao_1(8);
          break;
      case '9':
          animacao_1(9);
          break;
      default:
          break;
  }
  



  }
}

void att_led(){
  bool cor = true;
  i2c_init(I2C_PORT, 400 * 1000);
  cor = !cor;
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  ssd1306_t ssd; // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  ssd1306_fill(&ssd, !cor); // Limpa o display
  ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
  char str[2] = {comando, '\0'}; // Converte o caractere em string
  ssd1306_draw_string(&ssd, "Recebido", 8, 10); // Desenha uma string
  ssd1306_draw_string(&ssd, str,80, 10); // Exibe o comando no display   
  if (estado_led_azul == false){
    ssd1306_draw_string(&ssd, "Led azul OFF",8,25); // Desenha uma string
  }

  if (estado_led_azul == true){
    ssd1306_draw_string(&ssd, "Led azul ON",8,25); // Desenha uma string
  }
  if (estado_led_verde == false){
    ssd1306_draw_string(&ssd, "Led verde OFF",8,35); // Desenha uma string
  }

  if (estado_led_verde == true){
    ssd1306_draw_string(&ssd, "Led verde ON",8,35); // Desenha uma string
  }

  ssd1306_draw_string(&ssd, "By Gabriel",8,50);
  ssd1306_send_data(&ssd); // Atualiza o display

}
void inicializar_UART(){
  uart_init(UART_ID, BAUD_RATE);
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}


void animacao_1(int letra){
  for (int i = 0; i < 25; i++) {
      valor_led = matrix_rgb(letras[letra][24-i], 0, 0.0);
      pio_sm_put_blocking(pio, sm, valor_led);
  }
}

void gpio_irq_handler(uint gpio, uint32_t events) {
  uint32_t current_time_a = to_us_since_boot(get_absolute_time());
  uint32_t current_time_b = to_us_since_boot(get_absolute_time());
  if (gpio == Botao_A) {
      if (current_time_a- ultimo_evento_A > 200000) {
        ultimo_evento_A = current_time_a;
        if (estado_led_verde == false){
           gpio_put(Led_pin_verde, 1);   
           estado_led_verde = true;
           att_led();
           printf("Led verde ligado\n");
        }  
      } else {
          gpio_put(Led_pin_verde, 0);
          estado_led_verde = false;
          att_led();
          printf("Led verde desligado\n");
      }
          

      }  

  else if (gpio == Botao_B && current_time_b - ultimo_evento_B > 200000) {

      ultimo_evento_B = current_time_b;
      if (estado_led_azul == false){
        gpio_put(Led_pin_azul, 1);   
        estado_led_azul = true;
        att_led();
        printf("Led azul ligado\n");
     }  
      } else {
          gpio_put(Led_pin_azul, 0);
          estado_led_azul=false;
          att_led();
          printf("Led azul desligado\n");
      }
  }
