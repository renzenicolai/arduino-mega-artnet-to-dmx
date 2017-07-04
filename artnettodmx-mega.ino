/* Art-Net to DMX512 bridge */
/* Arduino Mega version */
/* Copyright Renze Nicolai 2016 */

#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "dmx.h"

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

/* GPIO configuration */
#define PIN_DHCP_OK_LED 2
#define PIN_DHCP_ERROR_LED 3
#define PIN_DMX_RECV_LED 4
#define PIN_DMX_ERROR_LED 5
#define PIN_BTN_UP 22
#define PIN_BTN_DOWN 23
#define PIN_BTN_OK 24

/* Software configuration */
#define FW_VERSION 0x0001
#define FACTORY_OEM_CODE 0x0001

/* Network configuration */
const byte factory_mac[] = { 0x00, 0x50, 0x04, 0x3E, 0xF9, 0x8A };
#define BUFFER_SIZE 768

/* Protocol constants */
#define STYLE 0x00 //A DMX to / from Art-Net device
#define ARTNET_DEFAULT_PORT 6454
#define HEADER_LENGTH 12
#define OPCODE_DMX 0x5000
#define OPCODE_ARTPOLL 0x2000
#define OPCODE_ARTPOLLREPLY 0x2100
#define OPCODE_DIAGDATA 0x2300 //Not implemented.
#define OPCODE_OPCOMMAND //Not implemented.
#define OPCODE_DMX 0x5000
#define OPCODE_ARTIPPROG 0xf800
#define OPCODE_ARTIPPROGREPLY 0xf900

/* Global variables: network */
char buff[BUFFER_SIZE] = {0};
IPAddress controller_ip(0,0,0,0);
uint16_t controller_port = 0;

/* Global variables: protocol */
uint8_t diag_priority = 0;
bool diag_broadcast = false;
bool diag_enabled = false;
bool send_artpollreply_onchange = false;

uint8_t port_address_programming_authority = 0b01;
uint8_t indicator_state = 0b11; //00: unknown, 01: locate/identify, 10: mute, 11: normal

uint16_t numports = 2; //Number of ports

uint8_t porttypes[4] = {0};

uint8_t goodinput[4] = {0b00001000}; //All inputs disabled (device does not have inputs)
uint8_t goodoutput[4] = {0};

uint8_t swin[4] = {0,1,2,3};
uint8_t swout[4] = {0,1,2,3};

uint16_t artpollreplycounter = 0;

uint32_t dmxerrorcounter = 0;
uint32_t packetcounter = 0;

uint16_t nodeStatus = 0x0001; //RcPowerOk
char nodeStatusText[16] = "OK"; //Because not implemented yet

uint8_t dmxsequenceposition[3] = {0};
uint8_t dmxsequencelastposition[3] = {0};

bool displayNeedsRedraw = false;

struct eeprom_settings_struct {
  uint32_t magic;
  uint16_t oem_code;
  byte mac[6];
  bool use_dhcp;
  IPAddress ip;
  IPAddress subnet;
  uint16_t port;
  char shortname[18];
  char longname[64];
  uint8_t netswitch;
  uint8_t subswitch;
  uint32_t chksum;
};

/* Objects */
EthernetUDP udp;
Adafruit_SSD1306 display(-1);

/* Functions: GPIO */
void dhcp_led(uint8_t state) {
  digitalWrite(PIN_DHCP_OK_LED, state&0x01);
  digitalWrite(PIN_DHCP_ERROR_LED, state&0x02);
}

/* Functions: settings */

eeprom_settings_struct eeprom_settings;

uint32_t calcChksum() {
  return 0; //To-Do
}

void loadDefaultSettings() {
  eeprom_settings.magic=0x42424242;
  eeprom_settings.oem_code = FACTORY_OEM_CODE;
  for (uint8_t i = 0; i<6; i++) eeprom_settings.mac[i] = factory_mac[i];
  eeprom_settings.use_dhcp = false;
  eeprom_settings.subnet[0] = 255;
  eeprom_settings.subnet[1] = 0;
  eeprom_settings.subnet[2] = 0;
  eeprom_settings.subnet[3] = 0;
  eeprom_settings.ip[0] = 2;
  eeprom_settings.ip[1] = eeprom_settings.mac[3]+(eeprom_settings.oem_code>>8)+(eeprom_settings.oem_code&0xFF);
  eeprom_settings.ip[2] = eeprom_settings.mac[4];
  eeprom_settings.ip[3] = eeprom_settings.mac[5];
  eeprom_settings.port = ARTNET_DEFAULT_PORT;
  sprintf(eeprom_settings.shortname,"Art-Net to DMX");
  //for (uint8_t i = 0; i<18; i++) eeprom-settings.shortname[i] = buff[i];
  sprintf(eeprom_settings.longname, "Simple Art-Net to DMX converter");  
  //for (uint8_t i = 0; i<64; i++) eeprom-settings.longname[i] = buff[i];
  eeprom_settings.netswitch = 0;
  eeprom_settings.subswitch = 0;
  eeprom_settings.chksum = calcChksum();
}

bool loadSettings(void) {
  EEPROM.get(0,eeprom_settings);
  if ((eeprom_settings.magic!=0x42424242)||(eeprom_settings.chksum!=calcChksum())) {
    //Serial.println("DEFAULT SETTINGS");
    loadDefaultSettings();
    return false;
  }
  //Serial.println("EEPROM SETTINGS");  
  return true;
}

bool saveSettings(void) {
  eeprom_settings.chksum = calcChksum();
  EEPROM.put(0,eeprom_settings);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("");
  display.println(" SETTINGS");
  display.println("  STORED"); 
  display.display();
  delay(1000);
}

void deviceSetup_drawIpMode() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("");
  display.println(" IP MODE:");
  if (eeprom_settings.use_dhcp) {
    display.println("   DHCP");
  } else {
    display.println("  STATIC");
  }
  display.display();
}

void deviceSetup_drawNetswitch() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("");
  display.println("NETSWITCH");
  display.println(eeprom_settings.netswitch);
  display.display();
}

void deviceSetup_drawSubswitch() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("");
  display.println("SUBSWITCH");
  display.println(eeprom_settings.subswitch);
  display.display();
}

bool deviceSetupOption(uint8_t chosen) {
  if (chosen==1) {
    while(digitalRead(PIN_BTN_OK)) {
      deviceSetup_drawIpMode();
      if (!digitalRead(PIN_BTN_UP)) {
        eeprom_settings.use_dhcp = false;
      }
      if (!digitalRead(PIN_BTN_DOWN)) {
        eeprom_settings.use_dhcp = true;
      }
    }
  } else if (chosen==2) {
    while(digitalRead(PIN_BTN_OK)) {
      deviceSetup_drawNetswitch();
      if (!digitalRead(PIN_BTN_UP)) {
        eeprom_settings.netswitch++;
        deviceSetup_drawNetswitch();
        while(!digitalRead(PIN_BTN_UP));
      }
      if (!digitalRead(PIN_BTN_DOWN)) {
        eeprom_settings.netswitch--;
        deviceSetup_drawNetswitch();
        while(!digitalRead(PIN_BTN_DOWN));
      }
    }
  } else if (chosen==3) {
    while(digitalRead(PIN_BTN_OK)) {
      deviceSetup_drawSubswitch();
      if (!digitalRead(PIN_BTN_UP)) {
        eeprom_settings.subswitch++;
        if (eeprom_settings.subswitch>0b00001111) {
          eeprom_settings.subswitch = 0;
        }
        deviceSetup_drawSubswitch();
        while(!digitalRead(PIN_BTN_UP));
      }
      if (!digitalRead(PIN_BTN_DOWN)) {
        eeprom_settings.subswitch--;
        if (eeprom_settings.subswitch>0b00001111) {
          eeprom_settings.subswitch = 0b00001111;
        }
        deviceSetup_drawSubswitch();
        while(!digitalRead(PIN_BTN_DOWN));
      }
    }
  } else if (chosen==4) {
    saveSettings();
    while(!digitalRead(PIN_BTN_OK));
    return false;
  }
  while (!digitalRead(PIN_BTN_OK));
  return true;
}

void deviceSetup_drawMenu(uint8_t option) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("SETUP MENU");
  display.setTextSize(1);
  display.println("");
  if (option==1) { display.print("> "); } else {display.print("  "); } 
  display.println("IP MODE");
  if (option==2) { display.print("> "); } else {display.print("  "); } 
  display.println("NETSWITCH");
  if (option==3) { display.print("> "); } else {display.print("  "); }
  display.println("SUBSWITCH");
  if (option==4) { display.print("> "); } else {display.print("  "); }
  display.println("SAVE SETTINGS");
  display.display();
}

void deviceSetup() {
  uint8_t chosen = 0;
  uint8_t selected = 1;
  deviceSetup_drawMenu(0);
  while(!digitalRead(PIN_BTN_OK));
  while(deviceSetupOption(chosen)) {
    chosen=0;
    while(chosen==0) {
      deviceSetup_drawMenu(selected);   
      if (!digitalRead(PIN_BTN_UP)) {
        selected--;
        if (selected<1) selected = 4;
        deviceSetup_drawMenu(selected);
        while(!digitalRead(PIN_BTN_UP));  
      }
      if (!digitalRead(PIN_BTN_DOWN)) {
        selected++;
        if (selected>4) selected = 1;
        deviceSetup_drawMenu(selected);  
        while(!digitalRead(PIN_BTN_DOWN));
      }
      if (!digitalRead(PIN_BTN_DOWN)) {
        selected++;
        if (selected>4) selected = 1;
        deviceSetup_drawMenu(selected);  
        while(!digitalRead(PIN_BTN_DOWN));
      }
      if (!digitalRead(PIN_BTN_OK)) {
        chosen = selected;
        while(!digitalRead(PIN_BTN_OK));
      }
    }
  }
}

void displayIPAddress() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("");
  display.println("IP ADDRESS");
  uint8_t numchars = 4;
  for (uint8_t i = 0; i<4; i++) {
    if (Ethernet.localIP()[i]>9) numchars++;
    if (Ethernet.localIP()[i]>99) numchars++;
  }
  if (numchars>7) display.setTextSize(1);
  display.println("");
  display.println(String(Ethernet.localIP()[0])+"."+String(Ethernet.localIP()[1])+"."+String(Ethernet.localIP()[2])+"."+String(Ethernet.localIP()[3]));
  display.display();
}

void displayDeviceInfo() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  for (uint8_t i = 0; i<6; i++) {
    char b[3] = {};
    sprintf(b, "%02x", eeprom_settings.mac[i]);
    display.print(b);
    if (i<5) display.print(":");
  }
  display.println("");
  display.print("IP MODE: ");
  if (eeprom_settings.use_dhcp) {
    display.println("DHCP");
  } else {
    display.println("STATIC");
  }
  display.println("IP: "+String(Ethernet.localIP()[0])+"."+String(Ethernet.localIP()[1])+"."+String(Ethernet.localIP()[2])+"."+String(Ethernet.localIP()[3]));
  display.println("SN: "+String(Ethernet.subnetMask()[0])+"."+String(Ethernet.subnetMask()[1])+"."+String(Ethernet.subnetMask()[2])+"."+String(Ethernet.subnetMask()[3]));
  display.println("PORT: "+String(eeprom_settings.port));
  display.println("SWITCH: N="+String(eeprom_settings.netswitch)+", S="+String(eeprom_settings.subswitch));
  display.display();
}

void(* resetFunc) (void) = 0;

void startNetwork() {
  if (eeprom_settings.use_dhcp) {
    bool state = false;
    while(!state) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0,0);
      display.println("");
      display.println(" DHCP IP");
      display.display();
      state = Ethernet.begin(eeprom_settings.mac);
      if (!state) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("");
        display.println("   DHCP   ");
        display.println(" TIMEOUT! ");
        display.display();
        if (!digitalRead(PIN_BTN_OK)) resetFunc();
        delay(1000);
      }
    }
    Serial.println("IP address: "+String(Ethernet.localIP()[0])+"."+String(Ethernet.localIP()[1])+"."+String(Ethernet.localIP()[2])+"."+String(Ethernet.localIP()[3]));
  } else {
    dhcp_led(0); //Off
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("");
    display.println("STATIC IP");
    display.display();
    delay(1000);
    IPAddress gw(2,0,0,1);
    IPAddress dns(2,0,0,1);
    Serial.println("IP address: "+String(eeprom_settings.ip[0])+"."+String(eeprom_settings.ip[1])+"."+String(eeprom_settings.ip[2])+"."+String(eeprom_settings.ip[3]));
    Ethernet.begin(eeprom_settings.mac, eeprom_settings.ip, dns, gw, eeprom_settings.subnet);
  }
  displayNeedsRedraw = true;
  udp.begin(eeprom_settings.port);
}

void setup() {
  pinMode(PIN_DHCP_OK_LED, OUTPUT);
  pinMode(PIN_DHCP_ERROR_LED, OUTPUT);
  pinMode(PIN_DMX_RECV_LED, OUTPUT);
  pinMode(PIN_DMX_ERROR_LED, OUTPUT);
  pinMode(PIN_BTN_UP, INPUT);
  pinMode(PIN_BTN_DOWN, INPUT);
  pinMode(PIN_BTN_OK, INPUT);  
  
  Serial.begin(115200);
  RnDMX.init1();
  RnDMX.init2();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("");
  display.println("  ARTNET  ");
  display.println("  TO DMX  "); 
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("");

  if (digitalRead(PIN_BTN_UP)) {
    if (loadSettings()) {
      display.println("SETTINGS LOADED");
    } else {
      display.println("EEPROM INVALID");
      display.println(" -> DEFAULT SETTINGS");
    }
  } else {
    loadDefaultSettings();
    display.println("BUTTON PRESSED");
    display.println(" -> DEFAULT SETTINGS");
  }
  
  display.display();

  delay(1000);

  if (!digitalRead(PIN_BTN_OK)) {
    deviceSetup();
  }

  porttypes[0] = 0b10000000; //First DMX output can output data from the Art-Net network
  porttypes[1] = 0b10000000; //Second DMX output can output data from the Art-Net network
  artpollreplycounter = 0;
  
  startNetwork();
}

void send_artpollreply() {
  if (controller_port==0) {return;}
  udp.beginPacket(controller_ip, controller_port);
  udp.write("Art-Net", 8);
  uint16_t opCode = OPCODE_ARTPOLLREPLY;
  buff[0] = opCode&0xFF;
  buff[1] = ((opCode&0xFF00)>>8);
  udp.write(buff, 2);
  for (uint8_t i=0; i<4; i++) buff[i] = Ethernet.localIP()[i];
  udp.write(buff, 4); 
  buff[0] = eeprom_settings.port&0xFF;
  buff[1] = (eeprom_settings.port&0xFF00)>>8;
  udp.write(buff, 2);
  buff[0] = FW_VERSION&0xFF;
  buff[1] = (FW_VERSION&&0xFF00)>>8;
  udp.write(buff, 2);
  udp.write(eeprom_settings.netswitch);
  udp.write(eeprom_settings.subswitch);
  buff[0] = (eeprom_settings.oem_code&0xFF00)>>8;
  buff[1] = eeprom_settings.oem_code&0xFF;
  udp.write(buff, 2);
  udp.write((uint8_t) 0);
  udp.write(((indicator_state&0x03)<<6)+((port_address_programming_authority&0x03)<<4));
  udp.write('R');
  udp.write('N');
  for (uint8_t i = 0; i<18; i++) buff[i] = eeprom_settings.shortname[i];
  udp.write(buff,18);
  for (uint8_t i = 0; i<64; i++) buff[i] = eeprom_settings.longname[i];
  udp.write(buff,64);
  sprintf(buff, "#%04x [%04d] %s", nodeStatus, artpollreplycounter, nodeStatusText);
  artpollreplycounter++;
  if (artpollreplycounter>9999) artpollreplycounter = 0;
  udp.write(buff,64);
  buff[0] = (numports&0xFF00)>>8;
  buff[1] = numports&0xFF;
  udp.write(buff,2);
  for (uint8_t i = 0; i<4; i++) buff[i] = porttypes[i];
  udp.write(buff,4);
  for (uint8_t i = 0; i<4; i++) buff[i] = goodinput[i];  
  udp.write(buff,4);
  for (uint8_t i = 0; i<4; i++) buff[i] = goodoutput[i];
  udp.write(buff,4);
  for (uint8_t i = 0; i<4; i++) buff[i] = swin[i];
  udp.write(buff,4);
  for (uint8_t i = 0; i<4; i++) buff[i] = swout[i];
  udp.write(buff,4);
  udp.write((uint8_t) 0); //swvideo (always 0)
  udp.write((uint8_t) 0); //swmacro
  udp.write((uint8_t) 0); //swremote
  udp.write((uint8_t) 0); //spare
  udp.write((uint8_t) 0); //spare
  udp.write((uint8_t) 0); //spare
  udp.write((uint8_t) STYLE);
  for (uint8_t i = 0; i<6; i++) buff[i] = eeprom_settings.mac[i]; //MAC
  udp.write(buff,6);
  buff[0] = 0; //ip_master (ignore)  
  buff[1] = 0; //ip_master (ignore)  
  buff[2] = 0; //ip_master (ignore)  
  buff[3] = 0; //ip_master (ignore)  
  buff[4] = 0; //bindindex (ignore)  
  udp.write(buff,5);
  udp.write((eeprom_settings.use_dhcp<<1)+(1<<2)+(1<<3)); //Status2: support DHCP and 15 bit port address
  for (uint8_t i = 0; i<26; i++) { udp.write((uint8_t) 0); } //filler (0)
  udp.endPacket();
}

void artnet_artpoll() {
  //Parse buffer as artpoll packet and send reply
  uint8_t talkToMe = buff[12];
  diag_enabled = (talkToMe&0x04)>>2;
  diag_broadcast = (talkToMe&0x08)>>3;
  send_artpollreply_onchange = (talkToMe&0x02)>>1;
  diag_priority = buff[13];
  /*//Serial.println("Artpoll:");
  //Serial.println(" - diag enabled: "+String(diag_enabled));
  //Serial.println(" - diag broadcast: "+String(diag_broadcast));
  //Serial.println(" - diag priority: "+String(diag_priority));
  //Serial.println(" - send onchange: "+String(send_artpollreply_onchange));*/
  controller_ip = udp.remoteIP();
  controller_port = udp.remotePort();
  send_artpollreply();//To sender
  for (uint8_t i = 0; i<4; i++) controller_ip[i] = 255;
  send_artpollreply();//Broadcast
}

bool outofsyncshown = false;

void displayOutOfSync(uint8_t internal_subnet, uint16_t a, uint16_t b) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("SYNC ERROR");
  display.println(String(internal_subnet)+": "+String(a)+" / "+String(b));
  display.display();
  dmxerrorcounter++;
}

void artnet_artdmx() {
  //Checks are in artnet().
  //if (buff[15]!=eeprom_settings.netswitch) return; //Check netswitch: return if packet is not for me.
  //if (buff[14]&0b11111110 != (eeprom_settings.subswitch<<4)&0b11111110) return; //Check subswitch (don't check last bit: we have 2 channels, 0 and 1)
  
  uint8_t internal_subnet = buff[14]&0x01; //Port number: last bit of subswitch

  /*if (sn==(eeprom_settings.subswitch<<4)) {
    internal_subnet = 0;
  } else if (sn==((eeprom_settings.subswitch<<4)+1)) {
    internal_subnet = 1;
  } else {
    return; //Packet is not for me.
  }*/

  uint8_t seq = buff[12];
  
  if (seq==0) { //Sequence disabled
    dmxsequenceposition[internal_subnet] = 0;
    dmxsequencelastposition[internal_subnet] = 0;
    //Serial.println("Sequence disabled.");
  } else if (seq>dmxsequenceposition[internal_subnet]) {
    dmxsequenceposition[internal_subnet] = seq;
    dmxsequencelastposition[internal_subnet] = seq;
    //Serial.println("Received sequence pos "+String(dmxsequenceposition[internal_subnet])+".");
    if (dmxsequenceposition[internal_subnet]>0xFE) dmxsequenceposition[internal_subnet] = 0;
  } else {
    if (dmxsequencelastposition[internal_subnet]==seq-1) {
      dmxsequenceposition[internal_subnet] = seq;
      dmxsequencelastposition[internal_subnet] = seq;
      Serial.println("R1");
      displayNeedsRedraw = true;
    } else if (dmxsequencelastposition[internal_subnet]>seq+10) {
      dmxsequenceposition[internal_subnet] = seq;
      dmxsequencelastposition[internal_subnet] = seq;
      Serial.println("R2");
      displayNeedsRedraw = true;
    } else {
      dmxsequencelastposition[internal_subnet] = seq;
      Serial.println("Received sequence "+String(seq)+" out of order (current position: "+String(dmxsequenceposition[internal_subnet])+").");
      displayOutOfSync(internal_subnet, seq,dmxsequenceposition[internal_subnet]);
      return; //Stop parsing.
    }
  }

  uint16_t len = (buff[16]<<8)+buff[17]+1;
  if (len>513) len = 513;
  buff[17] = 0; //Temp fix!!
  if (internal_subnet==0) {
    //for (uint16_t p = 0; p<len; p++) RnDMX.write2(p+1, (uint8_t) buff[18+p]);
    memcpy(RnDMX.getBuffer2(), &buff[17], len);
    
    //Serial.println(String(buff[18], DEC));
  } else if (internal_subnet==1) {
    //for (uint16_t p = 0; p<len; p++) RnDMX.write1(p+1, (uint8_t) buff[18+p]);
    memcpy(RnDMX.getBuffer1(), &buff[17], len);
  }
  packetcounter++;
}

void artnet(void) {
  uint8_t pktspercall = 40;
  while (pktspercall>0) {
    int packetSize = udp.parsePacket();
    if (packetSize>0) {
      if ((packetSize>BUFFER_SIZE)||(packetSize<HEADER_LENGTH)) {
        Serial.println("PKT_SIZE_ERROR");
      } else {
        udp.read(buff, BUFFER_SIZE);
        if (!(strcmp(buff, "Art-Net")==0)) {
          Serial.println("NOT ARTNET");
        } else {
          uint16_t opCode = buff[8]+(buff[9]<<8);
          //uint8_t protVerHi = buff[10];
          //uint8_t protVerLo = buff[11];
          switch(opCode) {
            case OPCODE_DMX:
              //Serial.println(String((uint8_t) buff[15])+", "+String((uint8_t) buff[14]));
              if (buff[15]==eeprom_settings.netswitch) {
                if ((buff[14]&0b11111110) == (eeprom_settings.subswitch<<4)) {
                  artnet_artdmx();
                }
              }
              break;
            case OPCODE_ARTPOLL:
              if (packetSize==14) artnet_artpoll();
              break;
            default:
              Serial.println("Unknown opcode: 0x"+String(opCode, HEX));
              break;
          }
        }
      }
      while (udp.read()!=-1);
      pktspercall--;
    } else {
      break;
    }
  }
  //Serial.println(pktspercall);
}

void displayHome() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("READY");
  display.println("IP: "+String(Ethernet.localIP()[0])+"."+String(Ethernet.localIP()[1])+"."+String(Ethernet.localIP()[2])+"."+String(Ethernet.localIP()[3]));
  display.println("NETSWITCH: "+String(eeprom_settings.netswitch));
  display.println("SUBSWITCH: "+String(eeprom_settings.subswitch));
  display.println("");
  display.println("Pkt recv: "+String(packetcounter));
  display.println("Pkt err: "+String(dmxerrorcounter));
  display.display();
}

void maintainDHCP() {
  int r = Ethernet.maintain();
  if ((r==1)||(r==3)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("DHCP ERROR");
    display.display();
  }
  if ((r==2)||(r==4)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("DHCP OK");
    display.display();
  }
}

uint16_t uiCounter = 0;
void userInterface() {
  uiCounter++;
  if (uiCounter>10000) {
    uiCounter = 0;
    digitalWrite(PIN_DMX_RECV_LED, LOW);
    displayNeedsRedraw = true;
  }
  if (!digitalRead(PIN_BTN_OK)) {
    deviceSetup();
    startNetwork();
  }
  if (!digitalRead(PIN_BTN_UP)) {
    displayDeviceInfo();
    displayNeedsRedraw = true;
  } else if (!digitalRead(PIN_BTN_DOWN)) {
    displayIPAddress();
    displayNeedsRedraw = true;
  } else if (displayNeedsRedraw) {
    displayNeedsRedraw = false;
    displayHome();
  }
}

void loop() {
  if(eeprom_settings.use_dhcp) maintainDHCP();
  artnet(); //Art-Net (network packets)
  userInterface(); //User interface (panel)
}
