/*
 * GbCartridgeKSController.cpp
 *
 * Created: 2015/02/05 23:35:27
 *  Author: Alan
 */ 


#define F_CPU 16000000	// 16 MHz oscillator.

#include <avr/io.h>
#include <avr/delay.h>
//#include <avr/interrupt.h>
#include <stddef.h>
#include <stdlib.h>

#define BaudRate 9600
#define MYUBRR (F_CPU / 16 / BaudRate ) - 1

#define TALKATIVE 

#define DATABUS_WRITE_MODE { DDRA = 0xFF; PORTA = 0x00; PINA = 0x00; }
#define DATABUS_READ_MODE { DDRA = 0x00; PORTA = 0x00; PINA = 0x00; }

#define ASCII0  48
#define ASCII9  57
#define ASCIIA  65

#define PORT_ADDR_H			PORTE
#define PORT_ADDR_M			PORTC
#define PORT_ADDR_L			PORTF
#define PORT_DATA			PORTA
#define PORT_CONTROL		PORTB
#define PIN_CE				0x08
#define PIN_OE				0x10
#define PIN_WE				0x20
#define PIN_RST				0x40
//mask to enable result of only our control pins
#define PIN_CONTROLS_MASK	0x78


void delayLong()
{
	unsigned int delayvar;
	delayvar = 0;
	while (delayvar <=  65500U)
	{
		asm("nop");
		delayvar++;
	}
}


inline unsigned char serialCheckRxComplete(void)
{
	return( UCSR0A & _BV(RXC0)) ;		// nonzero if serial data is available to read.
}

unsigned char serialCheckTxReady(void)
{
	return( UCSR0A & _BV(UDRE0) ) ;		// nonzero if transmit register is ready to receive new data.
}

unsigned char serialRead(void)
{
	while (serialCheckRxComplete() == 0)		// While data is NOT available to read
{;;}
	return UDR0;
}

void serialWrite(unsigned char DataOut)
{
	while (serialCheckTxReady() == 0)		// while NOT ready to transmit
{;;}
	UDR0 = DataOut;
}

void serialWriteString(const char* pString){
	if (pString == NULL)
		return;
		
	uint8_t i, c;
	i =0;
	while ((c = pString[i]) != 0){
		i++;
		serialWrite(c);
	}
}

void establishContact() {
	while (serialCheckRxComplete() == 0) {
		//serialWrite('A');
		delayLong();
		delayLong();
		delayLong();
		delayLong();
		delayLong();
		delayLong();
		delayLong();
	}
#ifdef TALKATIVE	
	serialWriteString("***GBCartridgeKS ***\r\n");
#endif //TALKATIVE
}


void setupSerial() 
{
	//Thanks to http://www.evilmadscientist.com/2009/basics-serial-communication-with-avr-microcontrollers/
	//Copy paste, worked from the first try
	
	//Serial Initialization
	
	/*Set baud rate */
	UBRR0H = (unsigned char)(MYUBRR>>8);
	UBRR0L = (unsigned char) MYUBRR;
	/* Enable receiver and transmitter   */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Frame format: 8data, No parity, 1stop bit */
	UCSR0C = (3<<UCSZ00);
}


void setupExternalFlash(){
	//address 0..7
	DDRF = 0xFF;
	//address 8..15
	DDRC = 0xFF;
	//address 16..17
	DDRE |= 0xC0; //sets PE7 and PE6
	
	//commands PB3..PB6
	DDRB |= 0x78;
	
	//data bus bi-directional so will change
	DATABUS_WRITE_MODE;
}

//////////////////////////////////////////////////////////////////////////
// Works for [0..9A-F]
uint8_t hexToInt(uint8_t pChar){
	if (pChar >= ASCII0 && pChar<= ASCII9)	{
		return pChar - ASCII0;
	}
	else {
		return pChar - ASCIIA;		
	}
}



uint16_t flashSetAddressFromInput() {
	uint8_t inByte = 0;         // incoming serial byte
	uint16_t vAddress = 0x0000;
	
	//read the 3 address byte MSB first
	inByte = serialRead() - ASCII0;
	//PORT_ADDR_H = (inByte << 6) | (PORTE & 0x3F);
	//just for not IGNORE PORTE
	PORT_ADDR_H = PORT_ADDR_H & 0x3f;

	inByte = hexToInt(serialRead());
	inByte = inByte << 4;
	inByte = inByte | hexToInt(serialRead());
	PORT_ADDR_M = inByte;
	
	vAddress = inByte;
	vAddress = vAddress << 8;

	inByte = hexToInt(serialRead());
	inByte = inByte << 4;
	inByte = inByte + hexToInt(serialRead());
	PORT_ADDR_L = inByte;

	vAddress = vAddress | (uint16_t)inByte;

	return vAddress;
}

uint8_t flashGetDataFromInput() {
	uint8_t inByte = 0;         // incoming serial byte
	
	inByte = hexToInt(serialRead());
	inByte = inByte << 4;
	inByte = inByte | hexToInt(serialRead());
	//don't set it now, we have to respect a proper sequence for writing
	//PORTA = inByte;
	
	return inByte;
}

/************************************************************************/
/* GET byte                                                             */
/************************************************************************/
uint8_t flashGetByteDecode2(uint16_t pAddr) {
	//read FLASH
	DATABUS_READ_MODE;

	//Doc: READ = /CE(l) & /WE(h) & /OE(l) & /RST(h)
	PORT_CONTROL = PIN_CONTROLS_MASK & (PIN_RST | PIN_CE | PIN_OE);
	_delay_us(1);

	//WE goes high
	PORT_CONTROL = PORT_CONTROL | PIN_WE;
	_delay_us(1);

	//put the address
	PORT_ADDR_L = (uint8_t)pAddr;
	PORT_ADDR_M = (uint8_t)(pAddr >> 8);
	PORT_ADDR_H = PORT_ADDR_H & 0x3f;
	_delay_us(1);

	//CE goes low
	PORT_CONTROL = PORT_CONTROL & !PIN_CE;
	_delay_us(1);

	//OE goes low
	PORT_CONTROL = PORT_CONTROL & !PIN_OE;
	//doc says tOE = 0ns
	_delay_us(1);
	
	//read !!	
	uint8_t vData = PINA;
	
	//OE & CE goes high
	PORT_CONTROL = PORT_CONTROL | (PIN_OE | PIN_CE);
	_delay_us(1);

	//WE goes low
	PORT_CONTROL = PORT_CONTROL & !PIN_WE;
	_delay_us(1);
	
	
#ifdef TALKATIVE

	serialWriteString("GET addr: 0x");

	char vBuffChar[6];
	serialWriteString(itoa(pAddr, vBuffChar, 16));

	serialWriteString(" > Value: 0x");

	serialWriteString(itoa(vData, vBuffChar, 16));

	serialWriteString("\r\n");
#endif //TALKATIVE
	return vData;
}
uint8_t flashGetByteDecode() {
	uint16_t vAddress = flashSetAddressFromInput();
	
	return flashGetByteDecode2(vAddress);
}


/************************************************************************/
/* WRITE byte                                                           */
/* Input: -                                                             */
/* Output: CONTROL = OE high                                            */
/************************************************************************/
void flashWriteSeq1Byte (uint16_t pAddr, uint8_t pData, uint8_t pIsLast){
	//Doc: READ = /CE(l) & /WE(l) & /OE(h) & /RST(h)
	//but watch the sequence
	
	//go write mode
	DATABUS_WRITE_MODE;
	
	//Start : CE and WE high
	PORT_CONTROL = PORT_CONTROL | (PIN_RST | PIN_CE | PIN_WE);
	_delay_us(1);

	//put the address
	PORT_ADDR_L = (uint8_t)pAddr;
	PORT_ADDR_M = (uint8_t)(pAddr >> 8);
	PORT_ADDR_H = PORT_ADDR_H & 0x3f;
	_delay_us(1);

	//CE goes low
	PORT_CONTROL = PORT_CONTROL & !PIN_CE;
	_delay_us(1);

	//OE goes high
	PORT_CONTROL = PORT_CONTROL | PIN_OE;
	_delay_us(1);
	
	//WE goes low
	PORT_CONTROL = PORT_CONTROL & !PIN_WE;
	_delay_us(1);
	
	//put the data
	PORT_DATA = pData;
	_delay_us(1);
		
	//WE goes high
	PORT_CONTROL = PORT_CONTROL | PIN_WE;
	_delay_us(1);	

	if (pIsLast != 0){
		//CE goes high
		PORT_CONTROL = PORT_CONTROL | PIN_CE;
		_delay_us(1);

		//CE goes low
		PORT_CONTROL = PORT_CONTROL & !PIN_CE;
		_delay_us(1);

		//OE goes low
		PORT_CONTROL = PORT_CONTROL & !PIN_OE;
		_delay_us(1);
		
	}
}


void flashWriteByteDecode2(uint16_t pAddress, uint8_t pData) {

	PORT_CONTROL = PORT_CONTROL | (PIN_RST);
	
/*
	//Byte Program 4 555h AAh AAAh 55h 555h A0h PA PD
	flashWriteSeq1Byte(0x0555, 0xaa,0);
	flashWriteSeq1Byte(0x0aaa, 0x55,0);
	flashWriteSeq1Byte(0x0555, 0xa0,0);
	flashWriteSeq1Byte(pAddress, pData,1);
*/
	
	//5555 / aa
	//2aaa / 55
	//5555 / a0
	//addr / data
	
	flashWriteSeq1Byte(0x5555, 0xaa, 0);
	flashWriteSeq1Byte(0x2aaa, 0x55, 0);
	flashWriteSeq1Byte(0x5555, 0xa0, 0);
	flashWriteSeq1Byte(pAddress, pData, 1);
	

#ifdef TALKATIVE
	serialWriteString("WRITE addr: ");

	char vBuffChar[6];
	serialWriteString(itoa(pAddress, vBuffChar, 16));

	serialWriteString(" < Value: 0x");

	serialWriteString(itoa(pData, vBuffChar, 16));

	serialWriteString("\r\n");
#endif //TALKATIVE
}


void flashWriteByteDecode() {
	//address
	uint16_t vAddress = flashSetAddressFromInput();
	uint8_t vData = flashGetDataFromInput();

	flashWriteByteDecode2(vAddress, vData);
}

/************************************************************************/
/* Shows content of dataport                                            */
/************************************************************************/

#ifdef TALKATIVE
void showDataPort() {
	serialWriteString("Data port content: PORTA=0x");

	char vBuffChar[6];
	serialWriteString(itoa(PORTA, vBuffChar, 16));

	serialWriteString(" PINA=0x");
	serialWriteString(itoa(PINA, vBuffChar, 16));

	serialWriteString("\r\n");
}
#endif //TALKATIVE

/************************************************************************/
/* DECODE command                                                       */
/************************************************************************/
void decodeCommand (){	
	uint8_t inByte = 0;         // incoming serial byte
	
	if (serialCheckRxComplete()) {
		inByte = serialRead();
			
		switch (inByte)
		{
			case 'g':
			case 'G':
			//GET a byte
			//Format "gAAAAA" with AAA is address MSB first
			flashGetByteDecode();
			break;

			case 'w':
			case 'W':
			//WRITE a byte
			//Format "gAAAAADD" with AAA is address and DD is data (MSB first)
			flashWriteByteDecode();
			break;					

#ifdef TALKATIVE
			case 'n':
			//GET a byte for the test
			flashGetByteDecode2(0x1111);
			break;
			case 'm':
			//WRITE a byte for the test
			flashWriteByteDecode2(0x1111, 0x67);
			break;

			case 'd':
			case 'D':
			//show content of data port
			showDataPort();
			break;			
#endif //TALKATIVE

			default:
			//Unknown command
#ifdef TALKATIVE
			serialWriteString("ERROR: unknown command: ");serialWrite(inByte);serialWriteString("\r\n");
#endif //TALKATIVE				
			break;
		}
	}
}

/************************************************************************/
/*  MAIN                                                                */
/************************************************************************/
int main (void)
{
	
	//Interrupts are not needed in this program. You can optionally disable interrupts.
	//asm("cli");		// DISABLE global interrupts.

		
	setupSerial();
	setupExternalFlash();
	

	establishContact();  // send a byte to establish contact until Processing responds
	
#ifdef TALKATIVE
	serialWriteString("[Commands: gAAAAA wAAAAADD d]\r\n");
#endif //TALKATIVE

	while(1) {
		if (serialCheckRxComplete()) {
			decodeCommand();
			
#ifdef TALKATIVE
			serialWriteString("[Commands: gAAAAA wAAAAADD d]\r\n>");
#endif //TALKATIVE

		}
	}	//End main loop.
	return 0;
}