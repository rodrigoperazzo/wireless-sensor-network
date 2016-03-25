#include <NRF905.h>
#include <SPI.h>

unsigned char buffer[NRF905_BUF_LEN]= {0};
unsigned char read_config_buf[NRF905_CONF_LEN];

NRF905 nrf905;

void putstring(unsigned char *str)
{
    int i=0;
    while(str[i] && i < NRF905_BUF_LEN){
        Serial.print(str[i++],HEX);
        Serial.print(' ');
    }
    Serial.println("");
}

void setup()
{
    char i;
    nrf905.begin(true); 

    /***********************************************************
	read register configuration, check register value written */
    nrf905.read_config(read_config_buf);
    
    /** serial communication configurate */
    Serial.begin(9600);
    
    Serial.println("NRF905 configuration:");
    /** test configuration */
    for(i=0; i<NRF905_CONF_LEN; i++){
        Serial.print(read_config_buf[i],HEX);
        Serial.print(' ');
    }
    Serial.println("\n");  
}

void loop()
{
    /** recieve data packet with default RX address */
	if (nrf905.available()) {
		nrf905.recv(buffer);
		putstring(buffer);
    }
    
    delay(1);
}
