// rf69 demo tx rx.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_RF69 class. RH_RF69 class does not provide for addressing or
// reliability, so you should only use RH_RF69  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf69_server.
// Demonstrates the use of AES encryption, setting the frequency and modem
// configuration

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <RH_RF69.h>
//signal strength directly next to eachother in the basement ranges from -17 to -29, centering around 3 points of -17, -22 and -28 with almost all values within 1 of each
/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 900.0
#define PIN 5
#define WAIT 3000
#define ARDUINONUMBER 4  //change this for each feather (increasing from 2)

#if defined(ARDUINO_SAMD_FEATHER_M0) // Feather M0 w/Radio
  #define RFM69_CS      8
  #define RFM69_INT     3
  #define RFM69_RST     4
  #define LED           13
#endif

//#define FIREFLYCOUNT

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

int16_t packetnum = 0;  // packet counter, we increment per xmission
int counter = 0;
String names[20];
int signalStrength[20][10];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, PIN, NEO_RGBW + NEO_KHZ800);

void setup()
{
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  for(int a = 0; a < 20; a++){
    names[a] = "";
    for(int b = 0; b < 10; b++){
      signalStrength[a][b] = 0;
    }
  }

  pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 TX Test!");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);

  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
}



void loop() {
  static long lastBlink = (long) millis();
  static long timeBetweenBlinks = 1000;
  // while(((time != lightBlinkTime && time != radioBlinkTime)){
  // time != lightBlinkTime == lightBlinkTime > then a default time
  // time != radioBlinkTime == radioBlinkTIme > than something && < than something
  long systemTime = 0;
  do{
    if (rf69.available()) {
      // Should be a message for us now
      uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      if (rf69.recv(buf, &len)) {
        if (!len) return;
        //Serial.print("Signal recieved: ");
        //Serial.println(millis());
        if(strstr((char *)buf, "ThisIsALongBlink")){
          lastBlink -= 1000;
          Serial.println("Recieved synchronization signal");
        }
        else{
          //Serial.println("Recieved signal strength checker signal");
          buf[len] = 0;
          int duplicate = 0;
          for(int a = 0; a < 20; a++){
            if(names[a].equals((char*)buf) /*strstr((char *)buf, names[a])*/){
              duplicate = 1;
              signalStrength[a][9] = signalStrength[a][8];
              signalStrength[a][8] = signalStrength[a][7];
              signalStrength[a][7] = signalStrength[a][6];
              signalStrength[a][6] = signalStrength[a][5];
              signalStrength[a][5] = signalStrength[a][4];
              signalStrength[a][4] = signalStrength[a][3];
              signalStrength[a][3] = signalStrength[a][2];
              signalStrength[a][2] = signalStrength[a][1];
              signalStrength[a][1] = signalStrength[a][0];
              signalStrength[a][0] = rf69.lastRssi();
              /*Serial.print("Signal: ");
              Serial.print(names[a]);
              Serial.print(", Signal strength: ");
              Serial.println(signalStrength[a][0]);*/
            }
          }
          if(duplicate == 0){
            names[counter] = (char*)buf;
            signalStrength[counter][0] = rf69.lastRssi();
            /*Serial.print("Signal: ");
            Serial.print(names[counter]);
            Serial.print(", Signal strength: ");
            Serial.println(signalStrength[counter][0]);*/
            counter++;
          }
        }
      }
      //Serial.print("Done: ");
      //Serial.println(millis());
    }
    systemTime = (long) millis();
  }while((systemTime - lastBlink) < timeBetweenBlinks && (systemTime - lastBlink) != ARDUINONUMBER*100);

  systemTime = (long) millis();
  if(systemTime - lastBlink < timeBetweenBlinks){
    //Serial.println("Sent signal strength checker signal");
    char radiopacket[20] = "ThisIsLong     ";
    packetnum = ARDUINONUMBER;
    itoa(packetnum, radiopacket+10, 10);
    //Serial.print("Sending "); Serial.println(radiopacket);
    for(int a = 0; a < 10; a++){
      rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
      rf69.waitPacketSent();
      delay(5);
    }
  }
  else{
    //Serial.println("Sent synchronization signal");
    int signalsGood = 0;
    int averageStrength = 0;
    for(int a = 0; a < counter; a++){  //******need to zero signalstrength variable!!!*******
      for(int b = 0; b < 10; b++){
        averageStrength += signalStrength[a][b];
      }
      averageStrength /= 10;
      if(averageStrength < -50){
        signalsGood++;
      }
    }
    Serial.print("Number of fireflies not in range: ");
    Serial.println(signalsGood);
    char radiopacket[20] = "ThisIsALongBlink";
    //Serial.print("Sending "); Serial.println(radiopacket);
    rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
    rf69.waitPacketSent();
    for(int a = 0; a < counter; a++){
      Serial.print(names[a]);
      for(int b = 0; b < 10; b++){
        Serial.print(", ");
        Serial.print(signalStrength[a][b]);
      }
      Serial.println();
    }
    uint32_t c = 0;
    if(signalsGood == counter){
      c = strip.Color(0, 255, 0); //it is rgbw (red, green, blue, white)
    }
    else if(signalsGood > 0){
      c = strip.Color(0, 0, 255);
    }
    else{
      c = strip.Color(255, 0, 0);
    }
    for(unsigned int i=0; i<strip.numPixels(); i++){
      strip.setPixelColor(i, c);
      strip.show();
    }
    delay(100);
    for(unsigned int i=0; i<strip.numPixels(); i++){
      strip.setPixelColor(i, strip.Color(0, 0, 0));
      strip.show();
    }
    lastBlink = millis();
  }
}
