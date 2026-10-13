#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUDRATE 9600
#define UBRR_VAL ((F_CPU / (16UL * BAUDRATE)) - 1)

volatile uint16_t setpoint = 512;
char rx_buffer[16];
uint8_t rx_index = 0;

const uint8_t step_sequence[4] = {
	(1 << PORTD2) | (1 << PORTD3),
	(1 << PORTD3) | (1 << PORTD4),
	(1 << PORTD4) | (1 << PORTD5),
	(1 << PORTD5) | (1 << PORTD2)
};

uint8_t current_step = 0;

void UART_init(void) {
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)(UBRR_VAL);
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_transmit(char data) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

void UART_send_string(const char *str) {
	while (*str) {
		UART_transmit(*str++);
	}
}

void UART_check_input(void) {
	if (UCSR0A & (1 << RXC0)) {
		char c = UDR0;
		if (c == '\n' || c == '\r') {
			rx_buffer[rx_index] = '\0';
			if (rx_index > 0) {
				int val = atoi(rx_buffer);
				if (val >= 0 && val <= 1023) {
					setpoint = (uint16_t)val;
				}
			}
			rx_index = 0;
			} else if (rx_index < sizeof(rx_buffer) - 1) {
			rx_buffer[rx_index++] = c;
		}
	}
}

void ADC_init(void) {
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_read(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

void Stepper_step(int8_t dir) {
	if (dir > 0) {
		current_step = (current_step + 1) % 4;
		} else if (dir < 0) {
		current_step = (current_step + 3) % 4;
		} else {
		PORTD &= ~((1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) | (1 << PORTD5));
		return;
	}

	PORTD = (PORTD & ~((1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) | (1 << PORTD5))) | step_sequence[current_step];
}

int main(void) {
	DDRD |= (1 << DDD2) | (1 << DDD3) | (1 << DDD4) | (1 << DDD5);

	UART_init();
	ADC_init();

	char telemetry[100];

	while (1) {
		UART_check_input();

		uint16_t ldr_val = ADC_read(0);
		uint16_t pot_val = ADC_read(1);

		int16_t error = (int16_t)setpoint - (int16_t)ldr_val;
		
		int16_t pwm_calc = 0;
		int8_t dir = 0;
		const char *dir_str = "CORRECTO";

		if (abs(error) > 15) {
			if (error > 0) {
				dir = 1;
				dir_str = "HORARIO";
				pwm_calc = error / 2;
				} else {
				dir = -1;
				dir_str = "ANTIHORARIO";
				pwm_calc = (-error) / 2;
			}


			if (pwm_calc > 255) pwm_calc = 255;
			if (pwm_calc < 30) pwm_calc = 30;
			} else {
			dir = 0;
			pwm_calc = 0;
			dir_str = "CORRECTO";
		}

		if (dir != 0) {
			for (uint8_t k = 0; k < 4; k++) {
				Stepper_step(dir);

				uint16_t delay_us = 4500 - ((uint32_t)pwm_calc * 3000) / 255;
				
				for (uint16_t us = 0; us < delay_us; us += 100) {
					_delay_us(100);
				}
			}
			} else {
			Stepper_step(0);
			_delay_ms(10);
		}

		sprintf(telemetry, "SP:%u | LDR:%u | POT:%u | PWM:%d | DIR:%s\r\n",
		setpoint, ldr_val, pot_val, pwm_calc, dir_str);
		UART_send_string(telemetry);
	}

	return 0;
}