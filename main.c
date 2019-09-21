/*
 * Lab09.c
 *
 * Created: 11/9/2018 1:00:54 PM
 * Author : cdphan
 */ 


#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#define BAUD 9600
#define BAUDRATE ((F_CPU) / (16UL*BAUD)) - 1

void adc_init();
void usart_init();
void usart_transmit(unsigned char val);

volatile unsigned short ADCglobal1 = 999; //checkpoint global
volatile unsigned short ADCglobal2 = 0; //demo global


int main(void)
{
	//initialisations
	cli();
	
	DDRB = 0b00100000; //output: LED{5}
	
	//timer 0 handles PWM
	TCCR0A |= (1 << WGM01); //CTC
	OCR0A = 155.25; //duty cycle period = 10 ms AKA 100 Hz freq; achieved with a compare value of 155.25 under 1024x prescale
	TCCR0B |= (1 << CS02) | (1 << CS00); //prescale 1024, start timer 0
	
	//timer 1 handles periodic check
	TCCR1A |= (1 << WGM11); //CTC
	OCR1A = 124; //timer period = 2 ms; achieved with a compare value of 124 under 256x prescale
	TCCR1B |= (1 << CS12); //prescale 256, start timer 1
	TIMSK1 |= (1 << OCIE1A); //enable timer 1 compare interrupt
	
	adc_init();
	usart_init();
	sei();
	
	//main loop
    while (1) 
    {	
		//demo version:
		///**
		OCR0B = ADCglobal2 * 15.525; //results in a percentage of 155.25 in intervals of 10%
		
		PORTB |= (1 << 5); //ON
		while ((TIFR0 & (1 << OCF0B)) == 0){} //busy wait until duty cycle ends
		TIFR0 |= (1 << OCF0B); //clear flag
			
		PORTB &= ~(1 << 5); //OFF
		while ((TIFR0 & (1 << OCF0A)) == 0){} //busy wait until period ends
		TIFR0 |= (1 << OCF0A); //clear flag
		//**/
		
		
		//checkpoint version:
		/**
		//no timers, just run this block forever
		ADCSRA |= (1 << ADSC); //begin ADC conversion
		while (ADCSRA & (1 << ADSC)){} //busy wait until conversion is finished
		
		uint16_t intADC = ADC; //represent 10-bit ADC as a 16-bit unsigned int
		short rawoutval = intADC*10 / 1024; //obtains a percentage and multiplies it by 10 (max percentage < 100%)
		int outval = rawoutval; //truncate the short into a 1-digit int
	
		if (outval != ADCglobal1){ //only call usart transmission if the reading changes (prevent spam)
			ADCglobal1 = outval; //write result to global short for comparison purposes
			usart_transmit((char)outval+48); //output an ascii representation of the 1-digit int via usart
		}
		else ADCglobal1 = outval; //write result to global short for future comparison anyways
		**/
	}
}


void adc_init(){
	ADMUX |= (1 << REFS0) | (1 << MUX1) | (1 << MUX0); //AVcc, ADC input channel 3
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //enable ADC, 128 scale (125 kHz)
	ADCSRA |= (1 << ADIE); //enable ADC conversion interrupt
}


void usart_init(){
	UBRR0H = (BAUDRATE >> 8); //upper half into UBRR high
	UBRR0L = BAUDRATE; //lower half into UBRR low
	UCSR0B |= (1 << TXEN0); //transmitter functionality
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01); //8-bit data format
}


void usart_transmit(unsigned char val){
	while (!(UCSR0A & (1 << UDRE0))){} //while buffer is not empty (AKA 0 in UDRE0 bit), wait in an empty loop
	UDR0 = val; //data register gets val after the buffer becomes free
}


//periodic ADC starter (can optionally substitute this ISR with auto-trigger)
ISR(TIMER1_COMPA_vect){
	ADCSRA |= (1 << ADSC); //begin ADC conversion (let the next ISR handle the rest)
}


//ADC conversion complete interrupt
ISR(ADC_vect){
	uint16_t intADC = ADC; //represent 10-bit ADC as a 16-bit unsigned int
	short rawoutval = intADC*10 / 1023; //obtains a percentage and multiplies it by 10 (max percentage = 100%)
	int outval = rawoutval; //truncate the short into an int again
	ADCglobal2 = outval; //write result to global short to be fetched in main
}