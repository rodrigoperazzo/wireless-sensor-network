#ifndef NRF905_h
#define NRF905_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define NRF905_BUF_LEN    32
#define NRF905_CONF_LEN   10

class NRF905
{
public:
    inline NRF905(){};
    void begin(bool always_on);
	
	void power_up();
	void power_down();
    
    void write_config(unsigned char *conf_buf);
    void read_config(unsigned char *conf_buf);
	void set_address(unsigned char *address);
	
	bool available();
	bool busy_channel();
	
	unsigned int recv(unsigned char *buffer);
    void send(unsigned char *buffer);
    void send(unsigned char *buffer, unsigned char *tx_address);
	
private:
    void set_rx();
};

#endif


