#include <NRF905.h>
#include <SPI.h>

unsigned char buffer[NRF905_BUF_LEN]= {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D};
unsigned char read_config_buf[NRF905_CONF_LEN];

NRF905 nrf905;

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
    if (!nrf905.busy_channel()) {
		nrf905.send(buffer);
    }

    /*buffer[30]++;
    if(buffer[30] == 0x3A){
      buffer[30] = '0';
    }*/
    
    delay(100);
}
