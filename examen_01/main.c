/*
 * examen_01.c
 *
 * Created: 26/06/2019 20:20:54
 * Author : Usuario
 */ 

#define F_CPU 16000000UL // Frecuencia a la que va operar
#include <avr/io.h>		//C de alto nivel compatible independientemente de OS y computadora.
#include <inttypes.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include "uart.h" // biblioteca de comunicación serie
#include <stdlib.h>
#include <stdbool.h> // para bool

// valores de tiempo de espera para cada tarea
#define t1 250
#define t2 5
#define t3 50

// eeprom define destinatario
#define eeprom_true 0
#define eeprom_data 1

// las tres subrutinas de tareas
void task1(void);  	// temporizador como comparador de 1s
void task2(void);	//
void task3(void);	//
void task4(void);   // boton

void initialize(void); //all the usual mcu stuff 

volatile unsigned int time1, time2, time3, time4, time5, time6, num;	//contadores de tiempo de espera
unsigned char tsk3m1, tsk3m2;				//task 3 message to task 1
unsigned char led;					//estados de luz
unsigned int time ;				//tiempo desde el arranque
unsigned char btnPush; // mensaje que indica un botón pulsado
unsigned char estadoPush;	// estado de maquina
unsigned int contador;
unsigned int pulsador;
unsigned int initialTime;
unsigned int finalTime;
bool verifica;

// UART file descriptor
// putchar and getchar are in uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);


//  #######################################################################################################################
//timer 0 compare ISR

ISR(TIMER0_COMPA_vect){
	 //Decrement the three times if they are not already zero. Disminuye las tres veces si no son ya cero.
	 if (time1>0)	--time1;
	 if (time2>0) 	--time2;
	 if (time3>0)	--time3;
}

// ######################################################################################################################
//Configurarlo todo para ininicializar

void initialize(void){
	// habilito los puertos
	DDRD = 0x00;	// PORT D is an input
	DDRB = 0x03; // PORT b0, b1 and b2 son outputs
	PORTB = 0x03; // apago los leds
	
	//configurar el temporizador 0 para 1 mSec base de tiempo
	TIMSK0= (1<<OCIE0A);	//turn on timer 0 cmp match ISR .... encender el temporizador 0 cmp partido ISR
	OCR0A = 249;  		//set the compare reg to 250 time ticks .... configurar el registro de comparación a 250 tics de tiempo
	
	//set prescalar to divide by 64 ...establecer prescalar para dividir por 64
	TCCR0B= 3;
	// turn on clear-on-match ....encender claro en el partido
	TCCR0A= (1<<WGM01) ;
	
	//init the LED status (all off)
	PORTB |=(1<<0);
	PORTB &=~(1<<2);
	PORTB &=~(1<<1);
	
	verifica = false;
	
	// iniciar los temporizadores de tareas
	time1 = t1;
	time2 = t2;
	time3 = t3;
	time4 = 2;
	time5 = 4;
	time6 = 4;
	
	//iInicia el mensaje
	btnPush=0; // no presiona ningun boton
	estadoPush=1; // inicia la maquina de estado
	
	pulsador = 0;
	
	 //init the UART -- uart_init() is in uart.c
	 uart_init();
	 stdout = stdin = stderr = &uart_str;
	 fprintf(stdout,"Starting...\n\r");
	 eeprom_write_word((uint16_t*)eeprom_data,0);
	 
	 //crank up the ISRs ... habilitar las interrupciones
	 sei();
}


// #################################################################################################################

//Punto de entrada y bucle del programador de tareas

int main(void)
{
    initialize();
	// averiguar si eeprom ha sido escrito alguna vez
	if (eeprom_read_byte((uint8_t*)eeprom_true) != 'T'){ // Si no, escribe algunos datos y la bandera "escrita"
		eeprom_write_word((uint16_t*)eeprom_data,0);
		eeprom_write_byte((uint8_t*)eeprom_true,'T');
	}
	//bucle del planificador de tareas principales
    while (1) {
		if(pulsador == 1){
			if(time1 == 0){
				time1 = t1;
				task1();
			}
		}
		if(pulsador == 2){
			if(time1 == 0){
				time1 = t1;
				task4();
			}
		}
		if(time2 == 0){time2 = t2; task2();}
		if (time3==0)	{task3();}
    }
}

// #################################################################################################################
void task1(void){
	if(--time4 == 0){
		time4 = 4;
		PORTB ^= 0x04;
		PORTB ^= 0x02;// para el buzzer
		pulsador =0;
		initialTime = time;
		verifica = true;
	}
}

void task2(void){
	time ++; //se aumenta cada 0,1 s
	PORTB ^= 0x01;
}

void task4(void){
	if (--time5 == 0){
		time5 = 4;
		PORTB ^= 0x04;
		PORTB ^= 0x02; // para el buzzer
		initialTime = time;
		pulsador =0;
		verifica = true;
		
	}
}
void task3(void){
	time3=t3;     //reset the task timer
	switch (estadoPush){
		case 1: // No push
				if (~PIND & 0x02){
					estadoPush=2;
					contador +=1;

					if(contador == 1){
						num = rand() % 2; // Genera el numero aleatorio
						fprintf(stdout,"%s","Iniciado!!\n\r");
						if (num == 0){
							pulsador = 1;
						}else{
							pulsador = 2;
						}
					}else if(contador == 2){
			
						if(verifica){
							unsigned int epromTime= 0;
							epromTime= eeprom_read_word((uint16_t*)eeprom_data);
							finalTime = time;
							unsigned int tiempoImprimir=0;
							tiempoImprimir = (int)((finalTime - initialTime)/2);
							fprintf(stdout,"%s%d%s\n\r", "Time, ",tiempoImprimir,"0 ms");
							if(tiempoImprimir >= 100){

								if (epromTime == 0){
									tiempoImprimir = 1;
									eeprom_write_word((uint16_t*)eeprom_data,tiempoImprimir);
									fprintf(stdout,"%s%d%s\n\r", "Saved time in eeprom, ",eeprom_read_word((uint16_t*)eeprom_data)," s");
									
								}else{
									
										fprintf(stdout,"%s%d%s\n\r", "Best time ",eeprom_read_word((uint16_t*)eeprom_data),"0 ms");
										if (tiempoImprimir < epromTime){
											eeprom_write_word((uint16_t*)eeprom_data,tiempoImprimir);
											fprintf(stdout,"%s%d%s\n\r", "Saved time in eeprom,",eeprom_read_word((uint16_t*)eeprom_data),"0 ms") ;
										}
								}

							}else{
				
								if (epromTime == 0){
									eeprom_write_word((uint16_t*)eeprom_data,tiempoImprimir);
									fprintf(stdout,"%s%d%s\n\r", "Saved time in eeprom, ",eeprom_read_word((uint16_t*)eeprom_data),"0 ms");
					
								}else{
					
									fprintf(stdout,"%s%d%s\n\r", "Best time ",eeprom_read_word((uint16_t*)eeprom_data),"0 ms");
									if (tiempoImprimir < epromTime){
										eeprom_write_word((uint16_t*)eeprom_data,tiempoImprimir);
										fprintf(stdout,"%s%d%s\n\r", "Saved time in eeprom,",eeprom_read_word((uint16_t*)eeprom_data),"0 ms") ;
									}
								}
							}
				
							time=0;
				
							PORTB ^= 0x04;
							PORTB ^= 0x02;
							contador=0;
							verifica = false;
							}else{
							fprintf(stdout,"%s \n\r", "Error!! Press the button before. Try again");
							time=0;
							PORTB ^= 0x04;
							PORTB ^= 0x02;
							contador=0;
						}
			
					}
				}// 0000 0010
				else {
					estadoPush=1;
				}
				break;

		case 2:
				if (~PIND & 0x02){
					estadoPush=3;
					btnPush=1;
				}else {
					estadoPush=1;
				}
				break;

		case 3:
				if (~PIND & 0x02) {
					//PushState=Pushed;
					//fprintf(stdout,"%s","pushed\n\r");
				}else {
					estadoPush=4;
				}
				break;

		case 4:
				if (~PIND & 0x02) {
					estadoPush=3;
				}else {
					estadoPush=1;
					btnPush=0;
				}
				break;
	}
}
