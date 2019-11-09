/*   All header files are included in this section 	*/
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <avr/interrupt.h>
#include <stdio.h>
/****************************************************/
/* All #defines are declared here		*/


#define Ndef 40
#define Compare 1999		// 10 kHz sampling
#define Pixel_Pin PB0	// Using Port B, bit 0 for the Neopixel
#define Pixel_Port PORTB
#define Pixel_Count 64 // Number of pixels in your chai
/****************************************************/
/*   All variables are declared in this section     */
/*   Any variable used in an interrupt must be 		*/
/*   declared volatile - e.g. volatile int Fred		*/
	int ADC_Converstion(void); 
	int ADC_Result[40];
	long int goertzel(int ADC_Result[], long int coeff, int N) ; 
	int baud = 129;
	char Goertzel_String[20];
	volatile int sam;
	long out;
	int in[8] = {0x7641, 0x6D23, 0x6154, 0x5321,0x42E1, 0x30FB, 0x3BC3, 0x1415};
	int Graphic_Equalizer[9];
	uint8_t Panel [3][64];
	int Max_LED;
	int Column;
	int x = 16; //x is brightness
	int y = 8;
	int input;
	long output[8];

/****************************************************/
/*  All user-created functions go here				*/
 int ADC_Conversion(void)
{
 	ADCSRA |= 1<<ADSC;                                                                                                                                                                                                                  
	while(ADCSRA & 1<<ADSC);
	return (ADC);
}

void Tx_Character(char Ch)
	{
	while((UCSR0A & 1<<UDRE0) == 0); 
	UDR0= Ch; //pass char to UARTs date reg
	}
void Tx_String(char *String)
{
	int i =0; //index for traverssing the array
	while(String[i] != '\0')
		{
		Tx_Character(String[i]);
		i++;
		}
	Tx_Character('\r'); // Transmit a return (new line)
}
enum {None, Red, Green, Yellow, Blue, Purple, Cyan, White};
enum {pRed, pGreen, pBlue};	

// Neopixel functions------------------------------------
//-- Pixel 0 bit high - 400ns with 20MHz
inline void Pixel_0_H(){
	Pixel_Port |= 1<<Pixel_Pin;
	asm("nop");
	asm("nop"); 
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	Pixel_Port &= ~(1<<Pixel_Pin);
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop"); 
	asm("nop");
	asm("nop");
 

	}
//-----------------------
//-- Pixel 0 bit high - 650ns with 20MHz
inline void Pixel_1_H(){
	Pixel_Port |= 1<<Pixel_Pin;
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop"); 
	asm("nop");
	asm("nop"); 
	asm("nop");
	asm("nop");
	asm("nop");
	
	Pixel_Port &= ~(1<<Pixel_Pin);
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");

	 
	}
//-------------------
//-- Pixel Reset
void Pixel_Reset(){
	Pixel_Port &= ~(1<<Pixel_Pin);
	_delay_us(10);
	}
//-----------------------
void Pixel_RGB(uint8_t Red, uint8_t Green, uint8_t Blue){
	static uint8_t Pixel_Mask;

// Green byte
	Pixel_Mask = 0x80;
	while(Pixel_Mask){
		if(Green & Pixel_Mask) Pixel_1_H();
			else Pixel_0_H();
		Pixel_Mask = Pixel_Mask>>1;
		}	

// Red byte
	Pixel_Mask = 0x80;
	while(Pixel_Mask){
		if(Red & Pixel_Mask) Pixel_1_H();
			else Pixel_0_H();
		Pixel_Mask = Pixel_Mask>>1;
		}
// Blue byte	
	Pixel_Mask = 0x80;
	while(Pixel_Mask){
		if(Blue & Pixel_Mask) Pixel_1_H();
			else Pixel_0_H();
		Pixel_Mask = Pixel_Mask>>1;
		}
	}
//---------------------------------------------

//-------Goertzel function---------------------------------------//	
long int goertzel(int ADC_Result[], long int coeff, int N) 
//---------------------------------------------------------------//	
{
//initialize variables to be used in the function
int Q, Q_prev, Q_prev2,i;
long prod1,prod2,prod3,power;

	Q_prev = 0; 		//set delay element1 Q_prev as zero
    Q_prev2 = 0;		//set delay element2 Q_prev2 as zero
	power=0;			//set power as zero
	
    for (i=0; i<N; i++) // loop N times and calculate Q, Q_prev, Q_prev2 at each iteration
		{
	        Q = (ADC_Result[i]) + ((coeff* Q_prev)>>14) - (Q_prev2); // >>14 used as the coeff was used in Q15 format
	        Q_prev2 = Q_prev;									 // shuffle delay elements
	        Q_prev = Q;
    	}
		
		//calculate the three products used to calculate power
		prod1=( (long) Q_prev*Q_prev);
		prod2=( (long) Q_prev2*Q_prev2);
		prod3=( (long) Q_prev *coeff)>>14;
		prod3=( prod3 * Q_prev2);

		power = ((prod1+prod2-prod3))>>11; //calculate power using the three products and scale the result down

		return power;	
}

/****************************************************/



int main(void)
{
/* Peripheral initialisation */
int i;

OCR1A = Compare;						//load compare value
TCCR1B = 1<<WGM12 | 1<<CS10;			//ctc mode| prescale 8
TIMSK1 = 1<<OCIE1A;						//enable timer interrupt
ADMUX= 1<<REFS0 |1<<MUX0;				//enable ADC1
ADCSRA=1<<ADEN  | 1<<ADPS1| 1<<ADPS0; 	//enable ADC and prescale 8
DIDR0= 1<<ADC1D;						//
UCSR0B= 1<<TXEN0;						//enable uart
UBRR0= baud;							//set baud rate
sei();									//enable global interrupt
 
/*Neopixel initialisation */
	uint8_t Count = 0;
	// The pin the Neopixel is on needs to be an output
	DDRB = 1<<Pixel_Pin;


	// Give everything chance to settle after power-up.
	_delay_ms(10);

	// Reset the interface
	Pixel_Reset();
	// Clear all pixels by writing 0 to every R,G and B of every pixel
	while(Count < Pixel_Count){ 
		Pixel_RGB(0, 0, 0);
		Count++;
		}
	// Now display some colours
	Pixel_Reset();
	_delay_ms(100);



	while(1)
		{

		
	for(i=0; i<8; i++){	
				   
	input = in[i]; 											//set the input to one of the pre-determined inputs

	sprintf(Goertzel_String, "Coefficient %x",input);		//output current input to serial monitor
		
	Tx_String(Goertzel_String);								//Transmit string
			


				while( sam < Ndef );						//wait for Ndef results


				out = goertzel(ADC_Result,input,Ndef);		//calculte power rating for current input

				sam = 0;									//reset sam count
	
				output[i]=out;								//eneter output into array
				
				sprintf(Goertzel_String, "%ld",output[i]);	//output current output to serial monitor
				
				Tx_String(Goertzel_String);					//Transmit string
				
				_delay_ms(100);								//delay for neopixel

		}
				_delay_ms(100);								//delay for neopixel

		//Column 1											//array to set neopixel colour
		Panel[0][0] = x;
		Panel[1][0] = x;
		Panel[2][0] = 0;
		Panel[0][1] = x;
		Panel[1][1] = x;
		Panel[2][1] = 0;
		Panel[0][2] = x;
		Panel[1][2] = y;
		Panel[2][2] = 0;
		Panel[0][3] = x;
		Panel[1][3] = y;
		Panel[2][3] = 0;
		Panel[0][4] = x;
		Panel[1][4] = y;
		Panel[2][4] = 0;
		Panel[0][5] = x;
		Panel[1][5] = 0;
		Panel[2][5] = 0;
		Panel[0][6] = x;
		Panel[1][6] = 0;
		Panel[2][6] = 0;
		Panel[0][7] = x;
		Panel[1][7] = 0;
		Panel[2][7] = 0;
		//Column 2
		Panel[0][8] = x;
		Panel[1][8] = x;
		Panel[2][8] = 0;
		Panel[0][9] = x;
		Panel[1][9] = x;
		Panel[2][9] = 0;
		Panel[0][10] = x;
		Panel[1][10] = y;
		Panel[2][10] = 0;
		Panel[0][11] = x;
		Panel[1][11] = y;
		Panel[2][11] = 0;
		Panel[0][12] = x;
		Panel[1][12] = y;
		Panel[2][12] = 0;
		Panel[0][13] = x;
		Panel[1][13] = 0;
		Panel[2][13] = 0;
		Panel[0][14] = x;
		Panel[1][14] = 0;
		Panel[2][14] = 0;
		Panel[0][15] = x;
		Panel[1][15] = 0;
		Panel[2][15] = 0;
		//Column 3
		Panel[0][16] = x;
		Panel[1][16] = x;
		Panel[2][16] = 0;
		Panel[0][17] = x;
		Panel[1][17] = x;
		Panel[2][17] = 0;
		Panel[0][18] = x;
		Panel[1][18] = y;
		Panel[2][18] = 0;
		Panel[0][19] = x;
		Panel[1][19] = y;
		Panel[2][19] = 0;
		Panel[0][20] = x;
		Panel[1][20] = y;
		Panel[2][20] = 0;
		Panel[0][21] = x;
		Panel[1][21] = 0;
		Panel[2][21] = 0;
		Panel[0][22] = x;
		Panel[1][22] = 0;
		Panel[2][22] = 0;
		Panel[0][23] = x;
		Panel[1][23] = 0;
		Panel[2][23] = 0;
		//Column 4
		Panel[0][24] = x;
		Panel[1][24] = x;
		Panel[2][24] = 0;
		Panel[0][25] = x;
		Panel[1][25] = x;
		Panel[2][25] = 0;
		Panel[0][26] = x;
		Panel[1][26] = y;
		Panel[2][26] = 0;
		Panel[0][27] = x;
		Panel[1][27] = y;
		Panel[2][27] = 0;
		Panel[0][28] = x;
		Panel[1][28] = y;
		Panel[2][28] = 0;
		Panel[0][29] = x;
		Panel[1][29] = 0;
		Panel[2][29] = 0;
		Panel[0][30] = x;
		Panel[1][30] = 0;
		Panel[2][30] = 0;
		Panel[0][31] = x;
		Panel[1][31] = 0;
		Panel[2][31] = 0;
		//Column 5
		Panel[0][32] = x;
		Panel[1][32] = x;
		Panel[2][32] = 0;
		Panel[0][33] = x;
		Panel[1][33] = x;
		Panel[2][33] = 0;
		Panel[0][34] = x;
		Panel[1][34] = y;
		Panel[2][34] = 0;
		Panel[0][35] = x;
		Panel[1][35] = y;
		Panel[2][35] = 0;
		Panel[0][36] = x;
		Panel[1][36] = y;
		Panel[2][36] = 0;
		Panel[0][37] = x;
		Panel[1][37] = 0;
		Panel[2][37] = 0;
		Panel[0][38] = x;
		Panel[1][38] = 0;
		Panel[2][38] = 0;
		Panel[0][39] = x;
		Panel[1][39] = 0;
		Panel[2][39] = 0;
		//Column 6
		Panel[0][40] = x;
		Panel[1][40] = x;
		Panel[2][40] = 0;
		Panel[0][41] = x;
		Panel[1][41] = x;
		Panel[2][41] = 0;
		Panel[0][42] = x;
		Panel[1][42] = y;
		Panel[2][42] = 0;
		Panel[0][43] = x;
		Panel[1][43] = y;
		Panel[2][43] = 0;
		Panel[0][44] = x;
		Panel[1][44] = y;
		Panel[2][44] = 0;
		Panel[0][45] = x;
		Panel[1][45] = 0;
		Panel[2][45] = 0;
		Panel[0][46] = x;
		Panel[1][46] = 0;
		Panel[2][46] = 0;
		Panel[0][47] = x;
		Panel[1][47] = 0;
		Panel[2][47] = 0;
		//Column 7
		Panel[0][48] = x;
		Panel[1][48] = x;
		Panel[2][48] = 0;
		Panel[0][49] = x;
		Panel[1][49] = x;
		Panel[2][49] = 0;
		Panel[0][50] = x;
		Panel[1][50] = y;
		Panel[2][50] = 0;
		Panel[0][51] = x;
		Panel[1][51] = y;
		Panel[2][51] = 0;
		Panel[0][52] = x;
		Panel[1][52] = y;
		Panel[2][52] = 0;
		Panel[0][53] = x;
		Panel[1][53] = 0;
		Panel[2][53] = 0;
		Panel[0][54] = x;
		Panel[1][54] = 0;
		Panel[2][54] = 0;
		Panel[0][55] = x;
		Panel[1][55] = 0;
		Panel[2][55] = 0;
		//Column8
		Panel[0][56] = x;
		Panel[1][56] = x;
		Panel[2][56] = 0;
		Panel[0][57] = x;
		Panel[1][57] = x;
		Panel[2][57] = 0;
		Panel[0][58] = x;
		Panel[1][58] = y;
		Panel[2][58] = 0;
		Panel[0][59] = x;
		Panel[1][59] = y;
		Panel[2][59] = 0;
		Panel[0][60] = x;
		Panel[1][60] = y;
		Panel[2][60] = 0;
		Panel[0][61] = x;
		Panel[1][61] = 0;
		Panel[2][61] = 0;
		Panel[0][62] = x;
		Panel[1][62] = 0;
		Panel[2][62] = 0;
		Panel[0][63] = x;
		Panel[1][63] = 0;
		Panel[2][63] = 0;
		

		Graphic_Equalizer [0] = 200>>3;							//array to determine which pixel turns on
		Graphic_Equalizer [1] = 255>>3;
		Graphic_Equalizer [2] = 361>>3;
		Graphic_Equalizer [3] = 402>>3;
		Graphic_Equalizer [4] = 511>>3;
		Graphic_Equalizer [5] = 690>>3;
		Graphic_Equalizer [6] = 740>>3;
		Graphic_Equalizer [7] = 890>>3;
		Graphic_Equalizer [8] = 1023>>3;								
	


	for(Column=0; Column<8; Column++){							//loop to set current neopixel colour
				Max_LED=0;
				while(output[Column] > Graphic_Equalizer[Max_LED]) Max_LED++;
				for(Count=0; Count<8; Count++){
					if(Max_LED >= Count) Panel[0][(Column * 8) + Count] = Panel[0][(Column * 8) + Count],
						Panel[1][(Column * 8) + Count] = Panel[1][(Column * 8) + Count];
					else Panel[0][(Column * 8) + Count] =0,Panel[1][(Column * 8) + Count] =0, Panel[2][(Column * 8) + Count] =0;
					} 
				}
		_delay_ms(1);
		for(Count=0; Count<64;Count++)							//output current colour
			{
			cli();												//disable global interrupts
			Pixel_RGB(Panel[0][Count], Panel[1][Count],Panel[2][Count]);
			sei();												//enable global interrupts
			}
			
		}	
					
		
} 
/****************************************************/
/* Interrupts (ISRs) go in this section */

ISR(TIMER1_COMPA_vect)
{

	if ( sam < Ndef )							  //runs every 100us
	{
		ADC_Result[sam] = ADC_Conversion() >> 4 ; // divide to reduce overflow 
		sam++;
	
	}

}





/****************************************************/
