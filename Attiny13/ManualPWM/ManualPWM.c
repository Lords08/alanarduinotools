/*
 * ManualPWM.c
 *
 * Created: 2012/02/13 0:08:35
 *  Author: Alan
 */ 


#define F_CPU 8000000L
#define LIMIT_BLUEGREEN 13
#define LIMIT_GREENRED 20

#include <avr/io.h>
#include <util/delay.h>

uint8_t mRGB[3];


uint8_t ReadADC() 
{
	ADCSRA |= (1 << ADEN); // Analog-Digital enable bit
	ADCSRA |= (1 << ADSC); // Start conversion
 
	while (ADCSRA & (1 << ADSC)); // wait until conversion is done
 /*
	ADCSRA |= (1 << ADSC); // start single conversion
 
	while (ADCSRA & (1 << ADSC)) // wait until conversion is done
 
	ADCSRA &= ~(1<<ADEN); // shut down the ADC */
		 
	return ADCH;
}

//Simple conversion by zone, no fading
//inline saves 50 byte ... 50byte for 2 jump and stack in/stack out ?
inline void Temp2RGB_Simple( uint8_t pTemp )	{
	mRGB[0] = 0;
	mRGB[1] = 0;
	mRGB[2] = 0;
			
	if (pTemp <= LIMIT_BLUEGREEN){
		//blue
		mRGB[2] = 255;
	}
	else {
		if (pTemp <= LIMIT_GREENRED){
			mRGB[1] = 255;
		}
		else {
			mRGB[0] = 255;
		}
	}
}

//Fading conversion
//inline saves 50 byte ... 50byte for 2 jump and stack in/stack out ?
inline void Temp2RGB_Fade( uint8_t pTemp )	{
			
	//Blue
	if (pTemp > LIMIT_BLUEGREEN+2){
		mRGB[2] = 0;
	}
	else {
		if (pTemp < LIMIT_BLUEGREEN-2){
			mRGB[2] = 255;
		}
		else {
			mRGB[2] = 255 - (LIMIT_BLUEGREEN +2 - pTemp) * (255/(2*2));
		}
	}
	
	
	//Green
	if (pTemp > LIMIT_BLUEGREEN-2 && pTemp <= LIMIT_BLUEGREEN +2){
		mRGB[1] = (pTemp - (LIMIT_BLUEGREEN -2) ) * (255/(2*2));
	}
	else {
		if (pTemp > LIMIT_GREENRED-2 && pTemp <= LIMIT_GREENRED +2){
			mRGB[1] = 255 - (LIMIT_GREENRED +2 - pTemp) * (255/(2*2));
		}
		else {
			mRGB[1] = 255;
		}
	}
	
	/*
	if (pTemp <= LIMIT_BLUEGREEN){
		//blue
		mRGB[2] = 255;
	}
	else {
		if (pTemp <= LIMIT_GREENRED){
			mRGB[1] = 255;
		}
		else {
			mRGB[0] = 255;
		}
	}
	*/
}

int main(void)
{
	mRGB[0] = 0;
	mRGB[1] = 0;
	mRGB[2] = 0;

	
	//Timer full speed !
	CLKPR = (1<<CLKPCE);
	CLKPR = 0; // Divide by 1 
	
	//port B pins 0-5 go OUTPUT
    DDRB = 0xFF;
	
	
	//------------START ADC INIT --------------------------
	 ADCSRA |= (1 << ADEN);
 /*| // Analog-Digital enable bit
 (1 << ADPS1)| // set prescaler to 8 (clock / 8)
 (1 << ADPS0); // set prescaler to 8 (clock / 8)
 */
 ADMUX |= 
	(1 << ADLAR) // AD result store in (more significant bit in ADCH) : ADCH holds the 8 MSB like that: perfect for PWM
	| (1 << MUX1); // Choose AD input AD2 (BP 4)
	//------------END ADC INIT --------------------------
	
	
	uint8_t vADCCheckRound = 0;
	uint8_t vRGBCurrentRound = 0;
    while(1)
    {
		
		if(vADCCheckRound == 0){
			uint8_t vADCValue = ReadADC();
			
			//uint8_t vTemp = (uint8_t)((uint32_t)vADCValue * 5UL * 100UL / 255UL );
			//replace a /255 by a shift right 8 bit and save 92 bytes! 10% of the memory!!! 
			//WTF a divide costs 92 bytes ? Oo;
			uint8_t vTemp = (uint8_t)(((uint32_t)vADCValue * 5UL * 100UL) >> 8UL);	
			
			Temp2RGB_Fade(vTemp);

		}
		
		
		
        //todo : put this in an interrupt
		if (vRGBCurrentRound == 0){
			
			//make pins B0-2 high
			PORTB = 
				(1 << PORTB0) 
				| (1 << PORTB1)
				| (1 << PORTB2)
				;
				
			/*
			PORTB = 0x00;
			if (mRGB[0] > 0)
				PORTB |= (1 << PORTB0);
			if (mRGB[1] > 0)
				PORTB |= (1 << PORTB1);
			if (mRGB[2] > 0)
				PORTB |= (1 << PORTB2);
				*/
		}
		

		if (vRGBCurrentRound == mRGB[0] && vRGBCurrentRound != 255){
			PORTB &= ~(1 << PORTB0);
		}

	
		if (vRGBCurrentRound == mRGB[1] && vRGBCurrentRound != 255){
			PORTB &= ~(1 << PORTB1);
		}
		

		if (vRGBCurrentRound == mRGB[2] && vRGBCurrentRound != 255){
			PORTB &= ~(1 << PORTB2);
		}
		
		//and the POV
		_delay_us(50);
		vRGBCurrentRound++;
		
		vADCCheckRound++;
    }
}