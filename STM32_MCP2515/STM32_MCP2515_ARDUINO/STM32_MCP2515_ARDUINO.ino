#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
MCP2515 CAN(10);

void setup() {
  while(!Serial);
  Serial.begin(9600);
  SPI.begin();
  CAN.reset();
  CAN.setBitrate(CAN_250KBPS, MCP_8MHZ);
  CAN.setNormalMode();

}
int led = 0;
unsigned long start_t = millis();

void loop() {
  if(millis() - start_t > 1000){
    canMsg.can_id = 0x36; //CAN id as 0x036
    canMsg.can_dlc = 8; //CAN data length as 8
    canMsg.data[0] = led; //send Led value
    canMsg.data[1] = 0x00; 
    canMsg.data[2] = 0x00; 
    canMsg.data[3] = 0x00;
    canMsg.data[4] = 0x00;
    canMsg.data[5] = 0x00;
    canMsg.data[6] = 0x00;
    canMsg.data[7] = 0x00;
    CAN.sendMessage(&canMsg); //Sends the CAN message
    led = !led;

    start_t = millis();
  }

    if (CAN.readMessage(&canMsg) == MCP2515::ERROR_OK){
     Serial.print("ID: ");
     Serial.print(canMsg.can_id,HEX);
     Serial.print(", ");
     Serial.print("Data: ");
     for(int i=0;i<canMsg.can_dlc;i++)
      {
        Serial.print(canMsg.data[i],HEX);
        Serial.print(" ");
      }
     Serial.println("");
   }
  
}
