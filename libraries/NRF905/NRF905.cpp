#include <NRF905.h>
#include <SPI.h>

// nrf905 instruction set 
#define WC		0x00
#define RC		0x10
#define WTP		0x20
#define RTP		0x21
#define WTA		0x22
#define RTA		0x23
#define RRP		0x24

#if defined(__AVR_ATmega1280__)
// Pins for arduino Mega (atmega 1280)
//----------------------------------------------NRF905 SPI---------------------------------------------------
#define CSN       53
#define MISO      50
#define MOSI      51
#define SCK       52
//----------------------------------------------NRF905 status IO---------------------------------------------
#define DR        49
#define CD        48
//----------------------------------------------NRF905 IO----------------------------------------------------
#define TXEN      46
#define TRX_CE    47
#else
// Pins for arduino duemilanove (atmega 328P)
//----------------------------------------------NRF905 SPI---------------------------------------------------
// SPI: 10 (SS), 11 (MOSI), 12 (MISO), 13 (SCK). 
#define CSN       10
#define MISO      12
#define MOSI      11
#define SCK       13
//----------------------------------------------NRF905 status IO---------------------------------------------
#define DR        9
#define CD        8
//----------------------------------------------NRF905 POWER-------------------------------------------------
#define PWR		  5
//----------------------------------------------NRF905 IO----------------------------------------------------
#define TXEN      7
#define TRX_CE    4
#endif

unsigned char config_info_buf[NRF905_CONF_LEN]={
        0x01,                   //CH_NO 1, 433MHZ
        0x0C,                   //output power 10db, auto retransmission off, receive current reduced off
        0x44,                   //4-byte address
        0x20,0x20,              //receive or send data length 32 bytes
        0xCC,0xCC,0xCC,0xCC,    //receiving address
        0xD8,                   //16 bit CRC, CRC enable, 16MHZ Oscillator, external clock disable
};

//-------------------initial nRF905---------------------------------------------
void NRF905::begin(bool always_on)
{
    unsigned char info_buf[NRF905_CONF_LEN] = {0};
	int i = 0;
	
	if(!always_on)
	{
		pinMode(PWR,OUTPUT);
		power_up();
	}
	
	pinMode(CSN, OUTPUT);
	digitalWrite(CSN, HIGH); // SPI Disable

	pinMode(DR, INPUT);	// Init DR for input
	pinMode(CD, INPUT);	// Init CD for input

	pinMode(TRX_CE, OUTPUT);
	pinMode(TXEN, OUTPUT);
	set_rx();// set radio in Rx mode

	SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.begin();

    delay(3);
	
	// write default configuration
	write_config(config_info_buf);
	delay(1);
	read_config(info_buf);
	
	for (i=0; i<NRF905_CONF_LEN; i++) {
	    if (info_buf[i]!=config_info_buf[i]) {
		    break;
		}
	}
	
	if (i != NRF905_CONF_LEN) {
	    Serial.println("Radio init error!!!");
	} else {
	    Serial.println("Radio init OK.");
	}
}

//---------------write to configuration register-----------------
void NRF905::write_config(unsigned char *conf_buf)
{
	digitalWrite(CSN,LOW);				// Spi enable for write a spi command
	SPI.transfer(WC);                   // send write configuration command 
	for (int i=0;i<NRF905_CONF_LEN;i++)	// Write configration words
	{
	   SPI.transfer(conf_buf[i]);
	}
	digitalWrite(CSN,HIGH);				// Disable Spi
}

void NRF905::read_config(unsigned char *conf_buf)
{
    digitalWrite(CSN,LOW);				// Spi enable for write a spi command
    SPI.transfer(RC);					// Send read configuration command
	for (int i=0;i<NRF905_CONF_LEN;i++) // Read the config
	{
	   conf_buf[i] = SPI.transfer(0x00);
	}
	digitalWrite(CSN,HIGH);				// Disable Spi
}

void NRF905::power_up() {
	digitalWrite(PWR,HIGH);
	delay(3); // takes 3ms
}

void NRF905::power_down() {
	digitalWrite(PWR,LOW);
}

void NRF905::set_address(unsigned char *address) {
	// Change the addr in config buffer
    if(config_info_buf[5] != address[0] ||\
       config_info_buf[6] != address[1] ||\
       config_info_buf[7] != address[2] ||\
       config_info_buf[8] != address[3]){

        config_info_buf[5] = address[0];
        config_info_buf[6] = address[1];
        config_info_buf[7] = address[2];
        config_info_buf[8] = address[3];
		// write the config buffer to NRF905
        write_config(config_info_buf);
    }
}

bool NRF905::available() {
	return (digitalRead(DR)==HIGH);
}

bool NRF905::busy_channel() {
	return (digitalRead(CD)==HIGH); 
}

void NRF905::set_rx(void)
{
	digitalWrite(TXEN, LOW);
	digitalWrite(TRX_CE, HIGH);
	// delay for mode change(>=650us)
	delayMicroseconds(800);
}

void NRF905::send(unsigned char *buffer)
{
	send(buffer,config_info_buf+5);
}

void NRF905::send(unsigned char *buffer, unsigned char *tx_address)
{
	int i;
	
	/* BEHAVIOR NOW DECIDED BY MAC PROTOCOL.
	// verify if there is someone transmitting
	while (busy_channel()) {
		delay(1);
	}
	*/

	// set tx mode
	digitalWrite(TRX_CE,LOW);
	digitalWrite(TXEN,HIGH);
	// delay for mode change(>=650us)
	delayMicroseconds(800);

	digitalWrite(CSN,LOW);
	// Write payload command
	SPI.transfer(WTP);
	for (i=0; i<NRF905_BUF_LEN; i++){
	    // Write 32 bytes Tx data
		SPI.transfer(buffer[i]);
	}
	digitalWrite(CSN,HIGH);
	delay(1);

    // Spi enable for write a spi command
	digitalWrite(CSN,LOW);
	// Write address command
	SPI.transfer(WTA);
	// Write 4 bytes address
	for (i=0;i<4;i++){
		SPI.transfer(tx_address[i]);
	}
	// Spi disable
	digitalWrite(CSN,HIGH);

	// Set TRX_CE high,start Tx data transmission, CE pulse
	digitalWrite(TRX_CE,HIGH);
	delay(1);
	digitalWrite(TRX_CE,LOW);
	// wait transmission to finish
	while (digitalRead(DR)==0);
	
	// back to rx mode
	set_rx();
}

unsigned int NRF905::recv(unsigned char *buffer)
{
	unsigned int i=0;
	if (available()) {
		digitalWrite(TRX_CE,LOW);
		digitalWrite(CSN,LOW);
		delay(1);
		SPI.transfer(RRP);
		delay(1);
		for (i = 0 ;i < NRF905_BUF_LEN; i++){
			buffer[i]=SPI.transfer(NULL);
			delay(1);
		}
		digitalWrite(CSN,HIGH);
		delay(1);
		digitalWrite(TRX_CE,HIGH);
		delay(1);
	}
	return i;
}
