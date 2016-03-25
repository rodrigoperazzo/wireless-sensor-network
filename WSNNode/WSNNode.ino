#include <Entropy.h>
#include <NRF905.h>
#include <SPI.h>
#include <HCSR04.h>

/* -------- PINS --------------------------- */
#define TRIG      2
#define ECHO      3
#define GAS_MQ2   A0
#define GAS_MQ3   A1
#define CONNECTED A5

/* --------- COMM FRAME TYPES -------------- */
#define ADD_REQUEST  0xC2
#define ADD_RESPONSE 0xC3
#define DATA_FRAME   0xC4
#define ACK_FRAME    0xC5

/* --------- COMM --------------------------- */

#define MAX_RESPONSE_TIME 50 // 36ms + 10ms FOR EACH GATEWAY ACK ATTEMPT
#define MAX_LOST_PKTS 255

/* --------- MAC MODES ---------------------- */
#define MAC_MODE_SEND_AT_WILL 0x0A
#define MAC_MODE_ALOHA        0x0B
#define MAC_MODE_CSMACA       0x0C

/* --------- CSMA-CA PARAMS ----------------- */
#define BACKOFF_SLOT 25
#define BACKOFF_LIMIT 5

/* --------- SENSORS PARAMS ------------------ */
#define PRE_HEAT 180000

/* --------- SENSORS VARIABLES -------------- */
HCSR04 hcsr04;

/* --------- COMM VARIABLES ----------------- */
unsigned char buffer[NRF905_BUF_LEN]= {0};
unsigned char read_config_buf[NRF905_CONF_LEN];

unsigned char pan_addr = 0;
unsigned char temp_addr = 0;

boolean success = false;
boolean give_up = false;

unsigned short seq_num = 0;
byte lost_pkts = 0;

NRF905 nrf905;

/* --------- MAC VARIABLES -------------- */
byte num_backoffs = 0;
byte backoff_exponent = 1;
byte mac_mode = MAC_MODE_SEND_AT_WILL;

/* --------- POWER SAVE VARIABLES ----------- */
long time_elapsed = 0;
byte sleep_factor = 0;


void putstring(unsigned char *str, byte len)
{
    int i=0;
    while(str[i] && i < len){
        Serial.print(str[i++],HEX);
        Serial.print(' ');
    }
    Serial.println("\n");
}

void setup()
{
    /* SENSORS */
    hcsr04.begin(TRIG,ECHO);

    /* SERIAL */
    Serial.begin(9600);
    Serial.println("SENSOR NODE");

    /* COMM INIT */
    pinMode(CONNECTED,OUTPUT);
    digitalWrite(CONNECTED,LOW);

    Entropy.Initialize();
    randomSeed(Entropy.random());
    temp_addr = random(1,255);

    /* RF */
    nrf905.begin(false);

    /* Read Config */
    nrf905.read_config(read_config_buf);
    Serial.println("NRF905 configuration:");
    putstring(read_config_buf,NRF905_CONF_LEN);
}

void clear_buffer()
{
  memset(buffer,0,NRF905_BUF_LEN);
}

void build_message(unsigned char type, unsigned char addr, boolean read_sensors)
{
  clear_buffer();

  // Type of message
  buffer[0] = type;

  // Address of sender
  buffer[1] = addr;

  // sequence number
  buffer[2] = seq_num & 0xFF;
  buffer[3] = (seq_num >> 8) & 0xFF;

  // number of lost packets
  buffer[4] = lost_pkts;

  if(read_sensors){
    // Distance reading
    /*
    float distance = hcsr04.read();
    unsigned short di = round(distance);
    buffer[5] = di & 0xff;
    buffer[6] = (di >> 8) & 0xff;
    */
    buffer[5] = 0;
    buffer[6] = 0;

    // already heated
    if(millis() > PRE_HEAT){
      // Gas MQ2 reading
      unsigned short gas_mq2 = analogRead(GAS_MQ2);
      buffer[7] = gas_mq2 & 0xff;
      buffer[8] = (gas_mq2 >> 8) & 0xff;

      // Gas MQ3 reading
      unsigned short gas_mq3 = analogRead(GAS_MQ3);
      buffer[9] = gas_mq3 & 0xff;
      buffer[10] = (gas_mq3 >> 8) & 0xff;
    }
  }
}

boolean execute_mac_protocol(byte mode)
{
    // MAC_MODE_SEND_AT_WILL
    boolean ok = true;

    if(mode == MAC_MODE_ALOHA) {

        Serial.print("ALOHA: ");
        Serial.print("LPKTS ");
        Serial.print(lost_pkts);
        Serial.print(" ");

        if(lost_pkts > 0) {
            long d = random(0,MAX_RESPONSE_TIME*16);
            Serial.println(d);
            delay(d);
        }

    } else if(mode == MAC_MODE_CSMACA) {

        num_backoffs = 0;
        backoff_exponent = 1;

        do
        {
            delay(random(0,pow(2,backoff_exponent)-1)*BACKOFF_SLOT);
            num_backoffs++;
            backoff_exponent++;

        } while(nrf905.busy_channel() && num_backoffs < BACKOFF_LIMIT);

        ok = !(num_backoffs == BACKOFF_LIMIT);
    }

    return ok;
}

void loop()
{
    time_elapsed = millis();

    seq_num++;
    lost_pkts = 0;

    success = false;
    give_up = false;

    while(!success && !give_up){

      boolean mac_ok = false;

      // BUILD MESSAGE
      if(pan_addr == 0)
      {
        build_message(ADD_REQUEST,temp_addr,false);
      } else {
        build_message(DATA_FRAME,pan_addr,true);
      }

      putstring(buffer, NRF905_BUF_LEN);

      // TURN ON RADIO
      nrf905.power_up();

      // MAC PROTOCOL
      mac_ok = execute_mac_protocol(mac_mode);

      if(!mac_ok){

        give_up = true;

      } else {

        // SEND MESSAGE
        nrf905.send(buffer);

        // WAIT FOR RESPONSE
        long response_time = millis();
        while (!nrf905.available() && (millis() - response_time) < MAX_RESPONSE_TIME);

        if (nrf905.available()) {

          nrf905.recv(buffer);
          putstring(buffer,NRF905_BUF_LEN);

          // ACK RECEIVED
          if(buffer[0] == ACK_FRAME && buffer[1] == pan_addr){

            success = true;

          //ADD RESPONSE RECEIVED
          } else if(buffer[0] == ADD_RESPONSE && buffer[1] == temp_addr){

           //PAN ADDRESS OK
            if(buffer[2] != 0xFF){

              pan_addr = buffer[2];

              mac_mode = buffer[3] & 0x0F; // last 4 bits
              sleep_factor = (buffer[3] & 0xF0) >> 4; // first 4 bits

              Serial.print("Config: ");
              Serial.print(mac_mode);
              Serial.print(" ");
              Serial.println(sleep_factor);

              success = true;

              digitalWrite(CONNECTED,HIGH);

            //PAN ADDRESS FAIL
            } else { give_up = true; }

          // RESPONSE RECEIVED WITH ANOTHER COMMAND OR ANOTHER NODE ADDRESS
          } else { lost_pkts++; }

        // RESPONSE NOT RECEIVED - COLLISION?
        } else { lost_pkts++; }

        // GIVE UP
        if(lost_pkts >= MAX_LOST_PKTS) { give_up = true; }

      }

    }

    // POWER SAVE OPERATIONS

    // TURN OFF RADIO
    nrf905.power_down();

    // SLEEP
    time_elapsed = millis() - time_elapsed;
    Serial.print("Time: ");
    Serial.println(time_elapsed);
    delay(sleep_factor*time_elapsed);
}
