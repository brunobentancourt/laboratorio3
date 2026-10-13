#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

// FRECUENCIAS DE NOTAS (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_A3  220
#define NOTE_B3  247
#define NOTE_OFF 0

#define MODE_PIANO 0
#define MODE_SONG  1

volatile uint8_t mode = MODE_PIANO;
volatile char rx_buffer[8];
volatile uint8_t rx_index = 0;
volatile uint8_t command_ready = 0;

// Frecuencias de las 10 teclas
const uint16_t note_freqs[10] = {
	NOTE_A3, NOTE_B3, NOTE_C4, NOTE_D4, NOTE_E4,
	NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5
};

const char* note_names[10] = {
	"La3", "Si3", "Do4", "Re4", "Mi4",
	"Fa4", "Sol4", "La4", "Si4", "Do5*"
};

typedef struct {
	uint16_t freq_melody;
	uint16_t freq_bass;
	uint16_t duration_ms;
} NoteEvent;

// CANCIÓN 1: UP
const NoteEvent song1[] = {
	{NOTE_F4, NOTE_C4/2, 450}, {NOTE_A4, NOTE_C4/2, 450}, {NOTE_F4, NOTE_C4/2, 450}, {NOTE_E4, NOTE_C4/2, 900},
	{NOTE_E4, NOTE_D4/2, 450}, {NOTE_A4, NOTE_D4/2, 450}, {NOTE_E4, NOTE_D4/2, 450}, {NOTE_D4, NOTE_D4/2, 900},
	{NOTE_D4, NOTE_G4/2, 450}, {NOTE_F4, NOTE_G4/2, 450}, {NOTE_D4, NOTE_G4/2, 450}, {NOTE_C4, NOTE_G4/2, 900},
	{NOTE_D4, NOTE_C4/2, 450}, {NOTE_A4, NOTE_C4/2, 450}, {NOTE_G4, NOTE_C4/2, 1200},
	{NOTE_D4, NOTE_D4/2, 450}, {NOTE_A4, NOTE_D4/2, 450}, {NOTE_G4, NOTE_D4/2, 450}, {NOTE_F4, NOTE_D4/2, 450}, {NOTE_D4, NOTE_D4/2, 900},
	{NOTE_F4, NOTE_C4/2, 450}, {NOTE_G4, NOTE_C4/2, 450}, {NOTE_F4, NOTE_C4/2, 450}, {NOTE_E4, NOTE_C4/2, 900},
	{NOTE_E4, NOTE_G4/2, 450}, {NOTE_G4, NOTE_G4/2, 450}, {NOTE_E4, NOTE_G4/2, 450}, {NOTE_C4, NOTE_G4/2, 900},
	{NOTE_C4, NOTE_C4/2, 450}, {NOTE_E4, NOTE_C4/2, 450}, {NOTE_G4, NOTE_C4/2, 450}, {NOTE_A3, NOTE_C4/2, 1200}
};

// CANCIÓN 2: Gravity Falls
const NoteEvent song2[] = {
	{NOTE_D4, NOTE_D4/2, 250}, {NOTE_E4, NOTE_D4/2, 250}, {NOTE_F4, NOTE_D4/2, 500}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_A4, NOTE_D4/2, 250}, {NOTE_G4, NOTE_D4/2, 250}, {NOTE_A4, NOTE_D4/2, 250}, {NOTE_C4, NOTE_D4/2, 500}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_D4, NOTE_A3, 250}, {NOTE_E4, NOTE_A3, 250}, {NOTE_F4, NOTE_A3, 500}, {NOTE_E4, NOTE_A3, 500}, {NOTE_G4, NOTE_A3, 500}, {NOTE_A4, NOTE_A3, 500}, {NOTE_G4, NOTE_A3, 250}, {NOTE_F4, NOTE_A3, 600}, {NOTE_OFF, NOTE_OFF, 150},
	{NOTE_F4, NOTE_D4/2, 200}, {NOTE_F4, NOTE_D4/2, 200}, {NOTE_F4, NOTE_D4/2, 200}, {NOTE_A4, NOTE_D4/2, 200}, {NOTE_A4, NOTE_D4/2, 200}, {NOTE_G4, NOTE_D4/2, 200}, {NOTE_F4, NOTE_D4/2, 400}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_A4, NOTE_D4/2, 200}, {NOTE_A4, NOTE_D4/2, 200}, {NOTE_A4, NOTE_D4/2, 200}, {NOTE_G4, NOTE_D4/2, 200}, {NOTE_A4, NOTE_D4/2, 200}, {NOTE_G4, NOTE_D4/2, 200}, {NOTE_F4, NOTE_D4/2, 600}, {NOTE_OFF, NOTE_OFF, 150},
	{NOTE_F4, NOTE_A3, 200}, {NOTE_F4, NOTE_A3, 200}, {NOTE_F4, NOTE_A3, 200}, {NOTE_A4, NOTE_A3, 200}, {NOTE_A4, NOTE_A3, 200}, {NOTE_G4, NOTE_A3, 200}, {NOTE_F4, NOTE_A3, 400}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_A4, NOTE_A3, 200}, {NOTE_A4, NOTE_A3, 200}, {NOTE_A4, NOTE_A3, 400}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_C5, NOTE_C4/2, 250}, {NOTE_C5, NOTE_C4/2, 250}, {NOTE_C5, NOTE_C4/2, 700}, {NOTE_OFF, NOTE_OFF, 300}
};

// CANCIÓN 3: Mi Corazón Encantado
const NoteEvent song3[] = {
	{NOTE_G4, NOTE_C4/2, 680}, {NOTE_G4, NOTE_C4/2, 680}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_E4, NOTE_C4/2, 360}, {NOTE_F4, NOTE_C4/2, 360}, {NOTE_G4, NOTE_C4/2, 360}, {NOTE_A4, NOTE_C4/2, 360}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_G4, NOTE_G4/2, 680}, {NOTE_F4, NOTE_G4/2, 680}, {NOTE_E4, NOTE_G4/2, 680}, {NOTE_D4, NOTE_G4/2, 680}, {NOTE_E4, NOTE_G4/2, 680}, {NOTE_E4, NOTE_G4/2, 680}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_C4, NOTE_A3, 360}, {NOTE_D4, NOTE_A3, 360}, {NOTE_E4, NOTE_A3, 360}, {NOTE_F4, NOTE_A3, 360}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_E4, NOTE_A3, 680}, {NOTE_D4, NOTE_A3, 680}, {NOTE_C4, NOTE_A3, 680}, {NOTE_B3, NOTE_A3, 680}, {NOTE_A3, NOTE_A3, 680}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_A3, NOTE_F4/2, 280}, {NOTE_A3, NOTE_F4/2, 280}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_C4, NOTE_F4/2, 680}, {NOTE_A3, NOTE_F4/2, 680}, {NOTE_G4, NOTE_G4/2, 680}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_G4, NOTE_G4/2, 280}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_C4, NOTE_C4/2, 1130}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_D4, NOTE_C4/2, 280}, {NOTE_E4, NOTE_C4/2, 280}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_F4, NOTE_C4/2, 680}, {NOTE_E4, NOTE_G4/2, 680}, {NOTE_D4, NOTE_G4/2, 680}, {NOTE_C4, NOTE_G4/2, 680}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_D4, NOTE_G4/2, 1580}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_C4, NOTE_C4/2, 300}, {NOTE_C4, NOTE_C4/2, 300}, {NOTE_B3, NOTE_C4/2, 300}, {NOTE_OFF, NOTE_OFF, 100},
	{NOTE_C4, NOTE_C4/2, 2830}, {NOTE_OFF, NOTE_OFF, 200}
};

//CONTROL DEL LCD
#define LCD_ADDR (0x27 << 1)

void twi_init(void) {
	TWSR = 0x00;
	TWBR = 0x48;
}

void twi_start(void) {
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}

void twi_stop(void) {
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
}

void twi_write(uint8_t data) {
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}

void lcd_send_i2c(uint8_t val) {
	twi_start();
	twi_write(LCD_ADDR);
	twi_write(val | 0x08);
	twi_stop();
}

void lcd_write_nibble(uint8_t nibble, uint8_t rs) {
	uint8_t data = (nibble & 0xF0) | rs | 0x08;
	lcd_send_i2c(data | 0x04);
	_delay_us(1);
	lcd_send_i2c(data & ~0x04);
	_delay_us(50);
}

void lcd_cmd(uint8_t cmd) {
	lcd_write_nibble(cmd & 0xF0, 0);
	lcd_write_nibble((cmd << 4) & 0xF0, 0);
	_delay_ms(2);
}

void lcd_data(uint8_t data) {
	lcd_write_nibble(data & 0xF0, 1);
	lcd_write_nibble((data << 4) & 0xF0, 1);
	_delay_us(50);
}

void lcd_init_i2c(void) {
	twi_init();
	_delay_ms(50);
	lcd_write_nibble(0x30, 0); _delay_ms(5);
	lcd_write_nibble(0x30, 0); _delay_us(200);
	lcd_write_nibble(0x30, 0);
	lcd_write_nibble(0x20, 0);
	
	lcd_cmd(0x28);
	lcd_cmd(0x0C);
	lcd_cmd(0x01);
	_delay_ms(2);
}

void lcd_print(const char *str) {
	while (*str) lcd_data(*str++);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
	lcd_cmd((row == 0 ? 0x80 : 0xC0) + col);
}

//COMUNICACIÓN UART
void uart_init(uint32_t baud) {
	uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

ISR(USART_RX_vect) {
	char ch = UDR0;
	if (ch == '\n' || ch == '\r') {
		if (rx_index > 0) {
			rx_buffer[rx_index] = '\0';
			command_ready = 1;
			rx_index = 0;
		}
		} else if (ch >= ' ') {
		if (rx_index < sizeof(rx_buffer) - 1) {
			rx_buffer[rx_index++] = ch;
			rx_buffer[rx_index] = '\0';
			
			if (rx_index == 2 && rx_buffer[0] == 'C') {
				command_ready = 1;
				rx_index = 0;
				} else if (rx_index == 4 && strcmp((char*)rx_buffer, "STOP") == 0) {
				command_ready = 1;
				rx_index = 0;
			}
		}
	}
}



// Melodía principal
void play_melody(uint16_t freq) {
	if (freq == 0) {
		TCCR1A = 0;
		TCCR1B = 0;
		TCNT1 = 0;
		PORTB &= ~(1 << PB1);
		return;
	}
	TCCR1A = 0;
	TCNT1 = 0;
	OCR1A = (F_CPU / (2UL * 8UL * (uint32_t)freq)) - 1;
	TCCR1A = (1 << COM1A0);
	TCCR1B = (1 << WGM12) | (1 << CS11);
}

//  Acompañamiento:
void play_bass(uint16_t freq) {
	if (freq == 0) {
		TCCR2A = 0;
		TCCR2B = 0;
		TCNT2 = 0;
		PORTB &= ~(1 << PB3);
		return;
	}
	TCCR2A = 0;
	TCNT2 = 0;
	
	// Prescaler 256: evita desbordamiento de 8 bits
	uint32_t ocr_val = (F_CPU / (2UL * 256UL * (uint32_t)freq)) - 1;
	if (ocr_val > 255) {
		ocr_val = 255;
	}
	
	OCR2A = (uint8_t)ocr_val;
	TCCR2A = (1 << COM2A0) | (1 << WGM21);
	TCCR2B = (1 << CS22) | (1 << CS21);
}

void stop_audio(void) {
	play_melody(0);
	play_bass(0);
	PORTB &= ~((1 << PB1) | (1 << PB3));
}

uint8_t custom_delay_ms(uint16_t ms) {
	while (ms >= 10) {
		_delay_ms(10);
		ms -= 10;
		if (command_ready && strcmp((char*)rx_buffer, "STOP") == 0) {
			return 1;
		}
	}
	if (ms > 0) {
		_delay_ms(10);
	}
	return 0;
}

// LECTURA DE TECLAS
int8_t get_pressed_key(void) {
	if (!(PINC & (1 << PC0))) return 0;
	if (!(PINC & (1 << PC1))) return 1;
	if (!(PIND & (1 << PD2))) return 2;
	if (!(PIND & (1 << PD3))) return 3;
	if (!(PIND & (1 << PD4))) return 4;
	if (!(PIND & (1 << PD5))) return 5;
	if (!(PIND & (1 << PD6))) return 6;
	if (!(PIND & (1 << PD7))) return 7;
	if (!(PINB & (1 << PB0))) return 8;
	if (!(PINB & (1 << PB2))) return 9;
	return -1;
}

// REPRODUCCIÓN DE CANCIONES
void play_song(const NoteEvent *song, uint8_t length, const char* name) {
	mode = MODE_SONG;
	lcd_cmd(0x01);
	lcd_set_cursor(0, 0);
	lcd_print(name);

	for (uint8_t i = 0; i < length; i++) {
		if (command_ready) {
			if (strcmp((char*)rx_buffer, "STOP") == 0) break;
			command_ready = 0;
		}

		if (song[i].freq_melody == NOTE_OFF && song[i].freq_bass == NOTE_OFF) {
			stop_audio();
			if (custom_delay_ms(song[i].duration_ms)) break;
			continue;
		}

		uint16_t pause_duration = 50;
		uint16_t sound_duration = (song[i].duration_ms > pause_duration) ? (song[i].duration_ms - pause_duration) : (song[i].duration_ms / 2);

		play_melody(song[i].freq_melody);
		play_bass(song[i].freq_bass);

		lcd_set_cursor(1, 0);
		char buf[16];
		snprintf(buf, sizeof(buf), "M:%dHz B:%dHz", song[i].freq_melody, song[i].freq_bass);
		lcd_print(buf);

		if (custom_delay_ms(sound_duration)) break;

		stop_audio();
		if (custom_delay_ms(pause_duration)) break;
	}

	stop_audio();
	mode = MODE_PIANO;
	lcd_cmd(0x01);
	lcd_set_cursor(0, 0);
	lcd_print("Modo: Piano");
}

int main(void) {
	// PB1 (Pin 9) y PB3 (Pin 11) como salidas a 0V
	PORTB &= ~((1 << PB1) | (1 << PB3));
	DDRB |= (1 << PB1) | (1 << PB3);

	// Pull-ups para los 10 botones
	DDRC &= ~((1 << PC0) | (1 << PC1));
	PORTC |= (1 << PC0) | (1 << PC1);

	DDRD &= ~((1 << PD2)|(1 << PD3)|(1 << PD4)|(1 << PD5)|(1 << PD6)|(1 << PD7));
	PORTD |= (1 << PD2)|(1 << PD3)|(1 << PD4)|(1 << PD5)|(1 << PD6)|(1 << PD7);

	DDRB &= ~((1 << PB0)|(1 << PB2));
	PORTB |= (1 << PB0)|(1 << PB2);

	// Inicializaciones
	uart_init(9600);
	lcd_init_i2c();
	stop_audio();
	sei();

	lcd_set_cursor(0, 0);
	lcd_print("Modo: Piano");

	int8_t last_key = -1;

	while (1) {
		// Control por UART
		if (command_ready) {
			command_ready = 0;
			if (strcmp((char*)rx_buffer, "C1") == 0) {
				play_song(song1, sizeof(song1)/sizeof(NoteEvent), "Up (x2)");
				} else if (strcmp((char*)rx_buffer, "C2") == 0) {
				play_song(song2, sizeof(song2)/sizeof(NoteEvent), "Gravity Falls");
				} else if (strcmp((char*)rx_buffer, "C3") == 0) {
				play_song(song3, sizeof(song3)/sizeof(NoteEvent), "Corazon Encant.");
				} else if (strcmp((char*)rx_buffer, "STOP") == 0) {
				stop_audio();
				mode = MODE_PIANO;
				lcd_cmd(0x01);
				lcd_set_cursor(0, 0);
				lcd_print("Modo: Piano");
			}
		}

		// Modo Piano
		if (mode == MODE_PIANO) {
			int8_t key = get_pressed_key();
			if (key != last_key) {
				if (key != -1) {
					// Pin 9 reproduce la melodía y Pin 11 reproduce el bajo
					play_melody(note_freqs[key]);
					play_bass(note_freqs[key] / 2);

					lcd_set_cursor(1, 0);
					char text[16];
					snprintf(text, sizeof(text), "Nota: %s (%dHz)", note_names[key], note_freqs[key]);
					lcd_print(text);
					} else {
					stop_audio();
					lcd_set_cursor(1, 0);
					lcd_print("Nota: Ninguna   ");
				}
				last_key = key;
			}
			_delay_ms(20);
		}
	}
	return 0;
}
