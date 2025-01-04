#include <SPI.h>
#include <Ethernet.h>

#include "PJLink.h"

#define PIN_BTN_POWER 4

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0xF9, 0x6E};
IPAddress ip(192, 168, 0, 151);
const int port = 4352;

// initialize the library instance:
EthernetClient client;

// projector IP address
IPAddress server(192, 168, 0, 229);

// assign your password when initializing
// instead, you cannot use the class name (PJLink) as an instance
//PJLinkOperator projector = PJLinkOperator("your password");

void setup() {
  // put your setup code here, to run once:
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // hardware configuration
  pinMode(PIN_BTN_POWER, INPUT_PULLUP);

  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  // initialize the ethernet device
  Ethernet.begin(mac, ip);
  // give the Ethernet shield a second to initialize:
  delay(1000);
  // reconnection setting
  //Ethernet.setRetransmissionTimeout(500);
  //Ethernet.setRetransmissionCount(3);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
  
  // try making a connection
  if (client.connect(server, port)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
    
    //PJLink.setPassword("password within 32 characters");
  }
  else{
    Serial.println("Connection failed");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  static bool is_on = false;

  if(client.connected()){
    // start communication after connecting via TCP/IP
    // (a message indicating establishment will be transmit automatically)
    if(client.available()){
      // receive a message until detecting the termination (CR)
      String str = client.readStringUntil(PJLinkOperator::PJLINK_TERMINAL) + PJLinkOperator::PJLINK_TERMINAL;
      // process the initial message in case of authorization
      if(str != NULL && str.length()) {
        Serial.println(str);
        PJLink.process(str);

        Serial.println(PJLink.getPacketAction());
        Serial.println(PJLink.getPacketCommand());
        Serial.println(PJLink.getPacketParameter());
      }

      // is_on = !is_on;
      // const char* cmd = PJLink.setPower(is_on);
      // const char* cmd = PJLink.setAVMute(PJLinkOperator::Mute::BOTH, false);
      const char* cmd = PJLink.setVideoInput(PJLinkOperator::VideoType::DIGITAL, 3);
      Serial.println(cmd); //"%1INPT 33\r"
      Serial.println(client.print(String(cmd)));
      delay(2000);

      cmd = PJLink.getProjectorInfo(PJLinkOperator::Control::PJLINK_INPUT);
      Serial.println(cmd); //"%1INPT ?\r"
      Serial.println(client.print(String(cmd)));
      delay(2000);

      // keep connection for 30 seconds
      // unsigned long time_start = millis();
      // while(client.available() && millis() - time_start < 30000){
      //   if(!digitalRead(PIN_BTN_POWER)){
      //     is_on = !is_on;
      //     client.write(PJLink.setPower(is_on));

      //     // receive a message until detecting the termination (CR)
      //     String str = client.readStringUntil('\r');  //PJLinkOperator::PJLINK_TERMINAL
      //     if(str != NULL && str.length()) {
      //       PJLink.process(str);
      //     }
      //   }
      // }
    }
  }
  // disconnection
  else{
    client.stop();
    //Serial.println("connection failed");
    reconnect();
  }
}

bool reconnect(){
  //Serial.println("Reconnection");
  // number of trial
  int count = 3;
  // try connection again
  while(!client.connect(server, port)){
    delay(200);
    // if over trial count
    if(--count <= 0){break;}
  }
  return client.connected();
}
