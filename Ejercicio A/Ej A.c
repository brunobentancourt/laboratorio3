#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define I2C_LCD_ADDR 0x27

#define LCD_BACKLIGHT 0x08
#define ENABLE        0x04
#define RS            0x01


#define HEATER_PIN PD4
#define FAN_PIN    PD3
#define DHT_PIN    PD2

volatile int punto_medio = 20;
volatile int rx_flag = 0;
volatile char rx_buffer[32];
volatile uint8_t rx_idx = 0;

int extract_number(const char* str) {
	while (*str) {
		if (*str >= '0' && *str <= '9') {
			return atoi(str);
		}
		str++;
	}
	return -1;
}

void I2C_init(void) {
	TWBR = 72;
	TWSR = 0;
}

void I2C_start(void) {
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}

void I2C_stop(void) {
	TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
	_delay_us(100);
}

void I2C_write(uint8_t data) {
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}

void LCD_I2C_write_byte(uint8_t data) {
	I2C_start();
	I2C_write(I2C_LCD_ADDR << 1);
	I2C_write(data | LCD_BACKLIGHT);
	I2C_stop();
}

void LCD_I2C_pulse(uint8_t data) {
	LCD_I2C_write_byte(data | ENABLE);
	_delay_us(1);
	LCD_I2C_write_byte(data & ~ENABLE);
	_delay_us(50);
}

void LCD_I2C_send(uint8_t value, uint8_t mode) {
	uint8_t highnib = value & 0xF0;
	uint8_t lownib = (value << 4) & 0xF0;
	LCD_I2C_pulse(highnib | mode);
	LCD_I2C_pulse(lownib | mode);
}

void LCD_command(uint8_t cmd) { LCD_I2C_send(cmd, 0); }
void LCD_char(uint8_t data)   { LCD_I2C_send(data, RS); }

void LCD_init(void) {
	I2C_init();
	_delay_ms(50);
	
	LCD_I2C_pulse(0x30); _delay_ms(5);
	LCD_I2C_pulse(0x30); _delay_us(150);
	LCD_I2C_pulse(0x30);
	LCD_I2C_pulse(0x20);

	LCD_command(0x28);
	LCD_command(0x0C);
	LCD_command(0x06);
	LCD_command(0x01);
	_delay_ms(2);
}

void LCD_set_cursor(uint8_t row, uint8_t col) {
	uint8_t addr = (row == 0) ? 0x80 + col : 0xC0 + col;
	LCD_command(addr);
}

void LCD_print(const char* str) {
	while (*str) LCD_char(*str++);
}

void UART_init(unsigned int baud) {
	unsigned int ubrr = F_CPU/16/baud - 1;
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)(ubrr);
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
}

void UART_transmit(char data) {
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

void UART_send_string(const char* str) {
	while (*str) UART_transmit(*str++);
}

ISR(USART_RX_vect) {
	char ch = UDR0;
	if (ch == '\r' || ch == '\n') {
		if (rx_idx > 0) {
			rx_buffer[rx_idx] = '\0';
			rx_flag = 1;
			rx_idx = 0;
		}
		} else if (rx_idx < sizeof(rx_buffer) - 1) {
		rx_buffer[rx_idx++] = ch;
	}
}

uint8_t DHT11_read(int *temperature) {
	uint8_t bits[5] = {0};
	uint8_t i, j;

	DDRD |= (1<<DHT_PIN);
	PORTD &= ~(1<<DHT_PIN);
	_delay_ms(18);
	PORTD |= (1<<DHT_PIN);
	_delay_us(30);

	DDRD &= ~(1<<DHT_PIN);
	_delay_us(40);

	if (PIND & (1<<DHT_PIN)) return 0;
	while (!(PIND & (1<<DHT_PIN)));
	while (PIND & (1<<DHT_PIN));

	for (j = 0; j < 5; j++) {
		for (i = 0; i < 8; i++) {
			while (!(PIND & (1<<DHT_PIN)));
			_delay_us(30);
			if (PIND & (1<<DHT_PIN)) {
				bits[j] |= (1 << (7 - i));
			}
			while (PIND & (1<<DHT_PIN));
		}
	}

	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
		*temperature = bits[2];
		return 1;
	}
	return 0;
}

void PWM_init(void) {
	DDRD |= (1<<FAN_PIN) | (1<<HEATER_PIN);
	TCCR2A = (1<<COM2B1) | (1<<WGM21) | (1<<WGM20);
	TCCR2B = (1<<CS22) | (1<<CS21) | (1<<CS20);
	OCR2B = 0;
	PORTD &= ~((1<<FAN_PIN) | (1<<HEATER_PIN));
}

void set_fan_speed(uint8_t speed_pct) {
	if (speed_pct == 0) {
		TCCR2A &= ~(1<<COM2B1);
		PORTD &= ~(1<<FAN_PIN);
		OCR2B = 0;
		} else {
		TCCR2A |= (1<<COM2B1);
		OCR2B = (uint16_t)speed_pct * 255 / 100;
	}
}

void set_heater(uint8_t state) {
	if (state) PORTD |= (1<<HEATER_PIN);
	else PORTD &= ~(1<<HEATER_PIN);
}

int main(void) {
	UART_init(9600);
	LCD_init();
	PWM_init();
	sei();

	UART_send_string("--- SISTEMA DE CONTROL CON DHT11 INICIADO ---\r\n");

	int temp = 0;
	char buffer[64];

	while (1) {
		if (rx_flag) {
			char temp_buf[32];
			cli();
			strcpy(temp_buf, (const char*)rx_buffer);
			rx_flag = 0;
			sei();

			int new_pm = extract_number(temp_buf);

			if (new_pm >= 10 && new_pm <= 35) {
				punto_medio = new_pm;
				sprintf(buffer, "PM actualizado a %d C\r\n", punto_medio);
				UART_send_string(buffer);
				} else {
				UART_send_string("ERROR: Ingrese un valor entre 10 y 35 C.\r\n");
			}
		}

		if (DHT11_read(&temp)) {
			int delta = punto_medio - 20;
			int t_calefactor_max = 15 + delta;
			int t_ideal_max      = 25 + delta;
			int t_fan_low_max    = 35 + delta;
			int t_fan_med_max    = 45 + delta;

			char accion[20];

			if (temp <= t_calefactor_max) {
				set_heater(1); set_fan_speed(0);
				strcpy(accion, "Calefactor ON");
			}
			else if (temp <= t_ideal_max) {
				set_heater(0); set_fan_speed(0);
				strcpy(accion, "Apagado (Ideal)");
			}
			else if (temp <= t_fan_low_max) {
				set_heater(0); set_fan_speed(33);
				strcpy(accion, "Ventilador BAJO");
			}
			else if (temp <= t_fan_med_max) {
				set_heater(0); set_fan_speed(66);
				strcpy(accion, "Ventilador MEDIO");
			}
			else {
				set_heater(0); set_fan_speed(100);
				strcpy(accion, "Ventilador ALTO");
			}

			LCD_command(0x01);
			_delay_ms(2);

			sprintf(buffer, "Temp: %d C", temp);
			LCD_set_cursor(0, 0);
			LCD_print(buffer);

			sprintf(buffer, "PM:%dC R:[%d-%d]", punto_medio, t_calefactor_max + 1, t_ideal_max);
			LCD_set_cursor(1, 0);
			LCD_print(buffer);

			sprintf(buffer, "%d,%d,%s\r\n", temp, punto_medio, accion);
			UART_send_string(buffer);

			} else {
			set_heater(0);
			set_fan_speed(0);
			LCD_set_cursor(0, 0);
			LCD_print("Error DHT11     ");
			UART_send_string("Error al leer sensor DHT11\r\n");
		}

		for (int i = 0; i < 50; i++) {
			_delay_ms(100);
			if (rx_flag) break;
		}
	}
}