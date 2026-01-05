
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

// FUNCOES

//USADAS PARA O LCD
void lcd_command(unsigned char cmnd);
void lcd_char(unsigned char data);
void lcd_init(void);
void lcd_string(const char *str);
void lcd_set_cursor(unsigned char row, unsigned char col);
void lcd_display_two_lines(const char *line1, const char *line2);


//USADAS PARA CONFIG DAS SENHAS
void read_key(char* senha);
void config_senha(void);
void check_senha(void);
void check_ports(void);
void displays(void);
void count_60(void);
void wait_touch(void);

// Definicao de pinos para LCD
#define LCD_RS PD2
#define LCD_EN PD3
#define LCD_D4 PD4
#define LCD_D5 PD5
#define LCD_D6 PD6
#define LCD_D7 PD7

// Definicao de pinos para displays de 7 segmentos
#define DISP1 PC0
#define DISP2 PC1

// Senha de fabrica e variaveis globais
const char senha_fabrica[5] = "A1B4";
char senha_programada[5];
char tentativa_senha[5];
volatile int estado_porta = 1; // 0 = fechada, 1 = aberta
volatile int tentativas_incorretas = 0; 



const char KEYPAD[4][4] = {  
    {'1', '2', '3', 'A'}, {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'}, {'*', '0', '#', 'D'}
};

// Funcao para varredura da tecla pressionada
char keypad_getkey() {
    for (int col = 0; col < 4; col++) {
        PORTB = (PORTB & 0xF0) | ~(1 << col); // Configura a coluna em LOW
        for (int row = 0; row < 4; row++) {
            if (!(PINB & (1 << (row + 4)))) {  // Verifica a linha
                while (!(PINB & (1 << (row + 4)))); // Espera liberar
                return KEYPAD[row][col];  // Retorna a tecla
            }
        }
    }
    return '\0';
}

// Funcao de ativacao do LCD
void lcd_command(unsigned char cmnd) {
    PORTD = (PORTD & 0x0F) | (cmnd & 0xF0);
    PORTD &= ~(1 << LCD_RS);
    PORTD |= (1 << LCD_EN);
    _delay_us(1);
    PORTD &= ~(1 << LCD_EN);
    _delay_us(200);
    PORTD = (PORTD & 0x0F) | ((cmnd << 4) & 0xF0);
    PORTD |= (1 << LCD_EN);
    _delay_us(1);
    PORTD &= ~(1 << LCD_EN);
    _delay_ms(2);
}

void lcd_char(unsigned char data) {
    PORTD = (PORTD & 0x0F) | (data & 0xF0);
    PORTD |= (1 << LCD_RS);
    PORTD |= (1 << LCD_EN);
    _delay_us(1);
    PORTD &= ~(1 << LCD_EN);
    _delay_us(200);
    PORTD = (PORTD & 0x0F) | ((data << 4) & 0xF0);
    PORTD |= (1 << LCD_EN);
    _delay_us(1);
    PORTD &= ~(1 << LCD_EN);
    _delay_ms(2);
}

void lcd_init(void) {
    DDRD |= (1 << LCD_RS) | (1 << LCD_EN);
    DDRD |= (1 << LCD_D4) | (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7);
    _delay_ms(20);
    lcd_command(0x02);
    lcd_command(0x28);
    lcd_command(0x0c);
    lcd_command(0x06);
    lcd_command(0x01);
    _delay_ms(2);
}

void lcd_string(const char *str) {
    int i;
    for (i = 0; str[i] != 0; i++) {
        lcd_char(str[i]);
    }
}

void lcd_set_cursor(unsigned char row, unsigned char col) {
    unsigned char address = (row == 0) ? 0x80 + col : 0xC0 + col;
    lcd_command(address);
}

void lcd_display_two_lines(const char *line1, const char *line2) {
    lcd_command(0x01); // Limpar display antes de mostrar novas mensagens
    _delay_ms(2);
    lcd_set_cursor(0, 0);
    lcd_string(line1);
    lcd_set_cursor(1, 0);
    lcd_string(line2);
}

// Funcao para ler a senha do teclado
void read_key(char* senha) {
    for (int i = 0; i < 4; i++) {
        char key = '\0';
        while (key == '\0') {
            key = keypad_getkey();
        }
        senha[i] = key;
    }
    senha[4] = '\0'; // Termina a string com NULL
}

// Funcao para configurar nova senha
void config_senha(void) {
    lcd_display_two_lines("NOVA SENHA", "ALFANUMERICA (4)");
    read_key(senha_programada);
    lcd_display_two_lines("CONFIRME A", "NOVA SENHA (4)");
    char confirmacao[5];
    read_key(confirmacao);
    if (strcmp(senha_programada, confirmacao) == 0) {
        lcd_display_two_lines("NOVA SENHA", "PROGRAMADA");
        _delay_ms(5000);
        lcd_display_two_lines("FECHE", "A PORTA");
        while (estado_porta == 1) {
            check_ports();
        }
        // Limpar o display apos fechar a porta
        lcd_command(0x01);
    } else {
        lcd_display_two_lines("SENHAS", "NAO COINCIDEM");
        _delay_ms(2000);
        config_senha();
    }
}

// Funcao para validar a senha
void check_senha(void) {
    lcd_display_two_lines("DIGITE", "A SENHA");
    read_key(tentativa_senha);
    if (strcmp(tentativa_senha, senha_programada) == 0 || strcmp(tentativa_senha, senha_fabrica) == 0) {
        lcd_display_two_lines("PORTA", "DESTRAVADA");
        _delay_ms(10000);
        
        int segundos = 20;
        while (segundos > 0 && estado_porta == 0) {
            check_ports();
            segundos--;
            _delay_ms(1000);
        }
        
        if (estado_porta == 0) {
            lcd_command(0x01); // Limpar o display
            wait_touch();
        } else {
            lcd_display_two_lines("DIGITE", "A SENHA");
        }
    } else {
        lcd_display_two_lines("SENHA", "INCORRETA");
        _delay_ms(10000);
        lcd_command(0x01); // Limpar o display apos 10 segundos
        tentativas_incorretas++;
        if (tentativas_incorretas >= 3) {
            count_60();
            lcd_command(0x01); 
            wait_touch();
            lcd_display_two_lines("DIGITE", "A SENHA");
            tentativas_incorretas = 0;
        } else {
            lcd_command(0x01); 
        }
    }
}

// Funcao de contagem regressiva
void count_60(void) {
    for (int i = 60; i >= 0; i--) {
        char buffer[3];
        sprintf(buffer, "%02d", i);
        lcd_display_two_lines("AGUARDE", buffer);
        _delay_ms(1000);
    }
}

// Funcao para esperar por um toque no teclado
void wait_touch(void) {
    while (keypad_getkey() == '\0') {
        // Espera por um toque no teclado
    }
}

// Funcao para verificar o estado da chave da porta
void check_ports(void) {
    if (!(PIND & (1 << PD0))) {
        estado_porta = 0; // Porta fechada
        displays();
    } else {
        estado_porta = 1; // Porta aberta
        displays();
    }
}

// Funcao para atualizar displays de 7 segmentos
void displays(void) {
    if (estado_porta == 1) {
        // Porta fechada, displays ligam o ponto decimal
        PORTC = 0x00; // Configura ambos os displays para mostrar 0  
    } else {
        // Porta aberta, displays mostram 00
        PORTC =  0b1111101; // Configura ambos os displays para ligar o ponto decimal; 
    }
}

int main(void) {
    // Configuracao inicial
    DDRB = 0x0F;
    PORTB = 0xF0;
    DDRC = 0xFF; // Configura todos os pinos de PORTC como saida para os displays de 7 segmentos
    PORTC = 0x00; // Inicializa displays mostrando 0
    DDRD &= ~(1 << PD0); // Configura PD0 como entrada para a chave da porta
    PORTD |= (1 << PD0); // Habilita pull-up em PD0
    lcd_init();
    strcpy(senha_programada, senha_fabrica); // Inicializa com a senha de fabrica

    // Iniciar com a configuracao de nova senha
    check_ports();
    if (estado_porta == 1)
    {
       config_senha(); 
    }
    

    while (1) {
        if (estado_porta == 0) {
            if (keypad_getkey() != '\0') {
                check_senha();
            }
        } else {
            lcd_command(0x01); // Limpar o display apos fechar a porta
            check_ports();
        }
    }
    return 0;
}