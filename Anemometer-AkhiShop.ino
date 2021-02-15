// Program ini di buat oleh: Tri Suliswanto untuk AKHI SHOP Electronics
// Gunakan dengan Bijak

#include <Arduino.h>
#include <SoftwareSerial.h>

//#define PRINT_FRAME_DATA  true

#define RX        2    //Serial Receive pin
#define TX        3    //Serial Transmit pin
#define DERE_pin    4    //RS485 Direction control
#define RS485Transmit    HIGH
#define RS485Receive     LOW

// Anemometer Parameter
#define DEFAULT_DEVICE_ADDRESS  0x02
#define WIND_SPEED_REG_ADDR 0x002A
#define SLAVE_ADDRESS_REG_ADDR_H 0x20
#define SLAVE_ADDRESS_REG_ADDR_L 0x00
#define READ_HOLDING_REG  0x03      //Function code 3 
#define WRITE_SINGLE_REG  0x06      //Function code 3 


SoftwareSerial RS485Serial(RX, TX);

unsigned int calculateCRC(unsigned char * frame, unsigned char bufferSize) 
{
  unsigned int temp, temp2, flag;
  temp = 0xFFFF;
  for (unsigned char i = 0; i < bufferSize; i++)
  {
    temp = temp ^ frame[i];
    for (unsigned char j = 1; j <= 8; j++)
    {
      flag = temp & 0x0001;
      temp >>= 1;
      if (flag)
        temp ^= 0xA001;
    }
  }
  // Reverse byte order. 
  temp2 = temp >> 8;
  temp = (temp << 8) | temp2;
  temp &= 0xFFFF;
  // the returned value is already swapped
  // crcLo byte is first & crcHi byte is last
  return temp; 
}

float getWindSpeed(byte address){
  float windSpeed = 0.0f;
  byte Anemometer_buf[8];
  byte Anemometer_request[] = {address, READ_HOLDING_REG, 0x00, WIND_SPEED_REG_ADDR, 0x00, 0x01, 0x00, 0x00}; // Request wind Speed Frame Data

  unsigned int crc16 = calculateCRC(Anemometer_request, sizeof(Anemometer_request) - 2);  
  Anemometer_request[sizeof(Anemometer_request) - 2] = crc16 >> 8; // split crc into 2 bytes
  Anemometer_request[sizeof(Anemometer_request) - 1] = crc16 & 0xFF;
  
  digitalWrite(DERE_pin, RS485Transmit);     // init Transmit
  RS485Serial.write(Anemometer_request, sizeof(Anemometer_request));
  RS485Serial.flush();

#ifdef PRINT_FRAME_DATA
  Serial.print("Send Request Frame data: "); 
  for(uint8_t i=0; i<sizeof(Anemometer_request); i++){
    Serial.print("0x");
    Serial.print(Anemometer_request[i], HEX); 
    Serial.print(" "); 
  }
  Serial.println();
#endif

  digitalWrite(DERE_pin, RS485Receive);      // Init Receive
  RS485Serial.readBytes(Anemometer_buf, 7);

#ifdef PRINT_FRAME_DATA
  Serial.print("Received Frame data: "); 
  for(uint8_t i=0; i<7; i++){
    Serial.print("0x");
    Serial.print(Anemometer_buf[i], HEX); 
    Serial.print(" "); 
  }
  Serial.println();
#endif

  byte dataH = Anemometer_buf[3];
  byte dataL = Anemometer_buf[4];
  windSpeed = ((dataH << 8) | dataL ) / 100.0;  //Range data 0~3000 = 0~30m/s

  return windSpeed;
  
}

int setSlaveAddress(byte current_address, unsigned int new_address){
  unsigned int new_addrH, new_addrL;
  byte Anemometer_buf[8];
  byte Anemometer_request[] = {current_address, WRITE_SINGLE_REG, SLAVE_ADDRESS_REG_ADDR_H, SLAVE_ADDRESS_REG_ADDR_L, 0x00, 0x00, 0x00, 0x00}; // Request Change Slave Addr

  //split address into 2 byte
  new_addrH = (new_address >> 8) & 0xFF;
  new_addrL = new_address & 0xFF;
  Anemometer_request[4] = new_addrH;
  Anemometer_request[5] = new_addrL;

  // calculate CRC16
  unsigned int crc16 = calculateCRC(Anemometer_request, sizeof(Anemometer_request) - 2);  
  Anemometer_request[sizeof(Anemometer_request) - 2] = crc16 >> 8; // split crc into 2 bytes
  Anemometer_request[sizeof(Anemometer_request) - 1] = crc16 & 0xFF;  

  digitalWrite(DERE_pin, RS485Transmit);     // init Transmit
  RS485Serial.write(Anemometer_request, sizeof(Anemometer_request));
  RS485Serial.flush();

#ifdef PRINT_FRAME_DATA
  Serial.print("Send Request Change addr Frame data: "); 
  for(uint8_t i=0; i<sizeof(Anemometer_request); i++){
    Serial.print("0x");
    Serial.print(Anemometer_request[i], HEX); 
    Serial.print(" "); 
  }
  Serial.println();
#endif

  digitalWrite(DERE_pin, RS485Receive);      // Init Receive
  RS485Serial.readBytes(Anemometer_buf, 8);

#ifdef PRINT_FRAME_DATA
  Serial.print("Received Frame data: "); 
  for(uint8_t i=0; i<8; i++){
    Serial.print("0x");
    Serial.print(Anemometer_buf[i], HEX); 
    Serial.print(" "); 
  }
  Serial.println();
#endif

  // calculate Received CRC16
  unsigned int crc16_received = ((Anemometer_buf[sizeof(Anemometer_buf) - 2] << 8) | Anemometer_buf[sizeof(Anemometer_buf) - 1] ); 
  unsigned int crc16_calculated = calculateCRC(Anemometer_buf, sizeof(Anemometer_buf) - 2); 
 
#ifdef PRINT_FRAME_DATA
  Serial.print("Received CRC data: "); 
  Serial.print("crc16: 0x"); Serial.println(crc16_received, HEX);
  Serial.print("Calculated CRC data: "); 
  Serial.print("crc16: 0x"); Serial.println(crc16_calculated, HEX);
#endif

  // verifikasi apakah data valid dengan cara melakukan kalkulasi CRC
  if(crc16_received == crc16_calculated){
    byte dataH = Anemometer_buf[4];
    byte dataL = Anemometer_buf[5];
    int address = (dataH << 8) | dataL ;
    return address;
  }
  else{
    return -1;
  }
  
}

void setup() {

  pinMode(DERE_pin, OUTPUT);  
  
  // Start the built-in serial port, for Serial Monitor
  Serial.begin(9600);
  Serial.println("Anemometer"); 

  // Start the Modbus serial Port, for anemometer
  RS485Serial.begin(9600);   
  delay(1000);

  // untuk ganti alamat slave dari 2 menjadi 3
//  int addr = setSlaveAddress(2, 3);
//  if(addr < 0){
//    Serial.println("Kesalahan Terjadi Ketika melakukan penggantian alamat SLAVE..."); 
//    while(1);
//  }
//  else{
//    Serial.print("Sukses melakukan penggantian alamat SLAVE menjadi: ");
//    Serial.println(addr); 
//  }
//  delay(5000);
}

void loop() {

  Serial.print("Wind speed : ");
  Serial.print(getWindSpeed(2));  // from address 3
  Serial.print(" m/s");
  Serial.println();
                    
  delay(100);

}
