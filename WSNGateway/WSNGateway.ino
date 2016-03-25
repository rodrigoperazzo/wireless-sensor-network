#include <NRF905.h>
#include <SPI.h>

/* --------- SCENARIOS --------------------- */
#define SCN_1 0x0A
#define SCN_2 0x0B
#define SCN_3 0x0C
#define SCN_4 0x1A
#define SCN_5 0x1B
#define SCN_6 0x1C
#define SCN_7 0x3A
#define SCN_8 0x3B
#define SCN_9 0x3C

/* --------- COMM FRAME TYPES -------------- */
#define ADD_REQUEST  0xC2
#define ADD_RESPONSE 0xC3
#define DATA_FRAME   0xC4
#define ACK_FRAME    0xC5

/* --------- PAN LIMIT --------------------- */
#define MAX_NODES 50
#define DECLINE_ADD 0xFF

/* --------- MAC MODES --------------------- */
#define MAC_MODE_SEND_AT_WILL 0x0A
#define MAC_MODE_ALOHA        0x0B
#define MAC_MODE_CSMACA       0x0C

/* --------- LOG --------------------------- */
#define LOG_INTERVAL 5000

/* --------- NODES ------------------------- */
typedef struct sensor_node {

  // for association only
  unsigned char temp_addr;

  // statistical and control
  unsigned short lost_pkts;
  byte lost_pkts_turn;
  unsigned short pkts;
  unsigned short seq_num;
  unsigned short updated;

  // sensors
  unsigned short distance;
  unsigned short gas_mq2;
  unsigned short gas_mq3;

  // initialization
  sensor_node(): temp_addr(0), lost_pkts(0),
  lost_pkts_turn(0), pkts(0), seq_num(0), updated(0),
  distance(0), gas_mq2(0), gas_mq3(0) {}

  // functions
  void print_info() {

    Serial.print("TEMP ADDR:     ");
    Serial.println(temp_addr);
    Serial.print("LOST PACKETS:  ");
    Serial.println(lost_pkts);
    Serial.print("PACKETS:       ");
    Serial.println(pkts);
    Serial.print("LAST UPDATED:  ");
    Serial.println(updated);
    Serial.print("DISTANCE:      ");
    Serial.println(distance);
    Serial.print("GAS MQ2:       ");
    Serial.println(gas_mq2);
    Serial.print("GAS MQ3:       ");
    Serial.println(gas_mq3);
    Serial.println("");
  }

} sensor_node;

sensor_node nodes[MAX_NODES];
byte last_node = 0;

/* --------- COMM -------------------------- */
unsigned char buffer[NRF905_BUF_LEN]= {0};
unsigned char read_config_buf[NRF905_CONF_LEN];
NRF905 nrf905;

/* --------- LOG --------------------------- */
long last_log = 0;
short log_turn = 0;

void putstring(unsigned char *str, byte len)
{
  int i=0;
  while(str[i] && i < len){
    Serial.print(str[i++],HEX);
    Serial.print(' ');
  }
  Serial.println("");
}

void setup()
{
  /** SERIAL */
  Serial.begin(9600);
  Serial.println("GATEWAY");

  /** RF */
  nrf905.begin(true);

  //* Read Config */
  nrf905.read_config(read_config_buf);
  Serial.println("NRF905 configuration:");
  putstring(read_config_buf,NRF905_CONF_LEN);
}

void clear_buffer()
{
  memset(buffer,0,NRF905_BUF_LEN);
}

byte add_node(unsigned char received_addr, byte lost_pkts)
{
  if(last_node < MAX_NODES)
  {
    nodes[last_node].temp_addr = received_addr;
    nodes[last_node].lost_pkts = lost_pkts;
    nodes[last_node].lost_pkts_turn = lost_pkts;
    return ++last_node;

  } else {

    return DECLINE_ADD;
  }
}

void send_add_response()
{
  boolean found = false;
  byte addr = buffer[1];
  short seq_num = (buffer[3] << 8) | buffer[2];
  byte lost_pkts = buffer[4];

  clear_buffer();

  // search for past added nodes
  for(int temp = 0; temp < last_node; temp++){

    // already added
    if(addr == nodes[temp].temp_addr){

      nodes[temp].lost_pkts += (lost_pkts - nodes[temp].lost_pkts_turn);
      nodes[temp].lost_pkts_turn = lost_pkts;
      found = true;
    }
  }

  // build message
  buffer[0] = ADD_RESPONSE;
  buffer[1] = addr;

  if(!found) {
    buffer[2] = add_node(addr,lost_pkts);
  } else {
    buffer[2] = last_node;
  }

  // scenario 9
  buffer[3] = SCN_9;

  nrf905.send(buffer);
}

void send_ack_frame()
{
  //  change to ack frame, preserves address received and send back
  buffer[0] = ACK_FRAME;
  nrf905.send(buffer);

  // buffer deserialization
  byte addr = buffer[1]-1;
  short seq_num = (buffer[3] << 8) | buffer[2];
  byte lost_pkts = buffer[4];

  if(nodes[addr].seq_num != seq_num){

    nodes[addr].pkts += 1;
    nodes[addr].lost_pkts += lost_pkts;
    nodes[addr].lost_pkts_turn = lost_pkts;
    nodes[addr].seq_num = seq_num;
    nodes[addr].updated = log_turn;

    nodes[addr].distance = (buffer[6] << 8) | buffer[5];
    nodes[addr].gas_mq2 =  (buffer[8] << 8) | buffer[7];
    nodes[addr].gas_mq3 = (buffer[10] << 8) | buffer[9];

  } else {

    nodes[addr].lost_pkts += (lost_pkts - nodes[addr].lost_pkts_turn);
    nodes[addr].lost_pkts_turn = lost_pkts;
  }
}


void loop()
{
  // listen to transmissions
  if (nrf905.available()) {

    clear_buffer();
    nrf905.recv(buffer);

    Serial.println("");
    Serial.print("COMMAND: ");
    Serial.print(buffer[0]);
    Serial.print(" NODE: ");
    Serial.println(buffer[1]-1);

    // DATA FRAME
    if(buffer[0] == DATA_FRAME)
    {
      send_ack_frame();

    // ADD REQUEST
    } else if (buffer[0] == ADD_REQUEST)
    {
      send_add_response();
    }
  }

  // log received data
  if(millis() - last_log > LOG_INTERVAL){
    last_log = millis();

    //  keep log of sensor readings
    Serial.print("------- BEGIN LOG: ");
    Serial.print(log_turn);
    Serial.println(" -------");
    int n;
    for(n = 0; n < last_node; n++){
      Serial.print("NODE:          ");
      Serial.println(n);
      nodes[n].print_info();
    }
    Serial.print("------- END LOG: ");
    Serial.print(log_turn);
    Serial.println(" ---------");
    log_turn++;
  }
  delay(1);
}

