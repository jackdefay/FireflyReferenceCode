// rf69 demo tx rx.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_RF69 class. RH_RF69 class does not provide for addressing or
// reliability, so you should only use RH_RF69  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf69_server.
// Demonstrates the use of AES encryption, setting the frequency and modem
// configuration

/*
Basic summary of the code:
It sets up all of the arrays,
Goes into the while loop in which signals are recieved and reacted to
If a synchronous signal is recieved, it breaks out of the while loop
If any other signal is received, the value and signal strength are recorded in the arrays as well as the boolean received being set to true
When it is time to break out of the while loop, it will check if the reason for breaking out was a synchronization blink or a counting blink
Depending on which, it will send a synchronization
*/


#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <RH_RF69.h>
//signal strength directly next to eachother in the basement ranges from -17 to -29, centering around 3 points of -17, -22 and -28 with almost all values within 1 of each
/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 900.0
#define PIN 5
#define WAIT 3000
#define ARDUINONUMBER 4  //change this for each feather (increasing from 2, current max value 13)

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
bool received[20];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, PIN, NEO_RGBW + NEO_KHZ800);

void setup()
{
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  for(int a = 0; a < 20; a++){ //zero all of the arrays
    names[a] = "";
    received[a] = false;
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

  pinMode(LED, OUTPUT); //check why I wrote this twice

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
}



void loop() {
  //static boolean reason = false; //false = synchronization blink, true = counting blink  //COMMENTED OUT BECAUSE DOESN'T SEEM TO DO ANYTHING
  static long lastBlink = (long) millis();  //got rid of "unsigned" marker
  static long timeBetweenBlinks = 1000;
  // while(((time != lightBlinkTime && time != radioBlinkTime)){
  // time != lightBlinkTime == lightBlinkTime > then a default time
  // time != radioBlinkTime == radioBlinkTIme > than something && < than something
  long systemTime = 0;
  /*for(int a = 0; a < RH_RF69_MAX_MESSAGE_LEN; a++){
    buf[a] = 0;
  }*/
  do{
    if (rf69.available()) {
      // Should be a message for us now
      uint8_t buf[RH_RF69_MAX_MESSAGE_LEN]; //required for radio, this is where the signals go
      uint8_t len = sizeof(buf); //required for radio, this is the length of the signal
      if (rf69.recv(buf, &len)) {
        if (!len) return;
        //Serial.print("Signal received: ");
        //Serial.println(millis());
        if(strstr((char *)buf, "ThisIsALongBlink")){ //if the signal is
          lastBlink = 0;
          //Serial.println("received synchronization signal");
        }
        else{
          //Serial.println("received signal strength checker signal");
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
              received[a] = true;
              /*Serial.print("Signal: ");
              Serial.print(names[a]);
              Serial.print(", Signal strength: ");
              Serial.println(signalStrength[a][0]);*/
            }
          }
          if(duplicate == 0){
            names[counter] = (char*)buf;
            signalStrength[counter][0] = rf69.lastRssi();
            received[counter] = true;
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
  }while((systemTime - lastBlink) < timeBetweenBlinks && (systemTime - lastBlink) != ARDUINONUMBER*70); //confirm it works
  /*Serial.print("systemTime: ");
  Serial.println(systemTime);
  Serial.print("lastBlink: ");
  Serial.println(lastBlink);
  Serial.print("timeBetweenBlinks: ");
  Serial.println(timeBetweenBlinks);
  Serial.print("ARDUINONUMBER*70: ");
  Serial.println(ARDUINONUMBER*70);
  Serial.print("Condition 1: ");
  Serial.println((systemTime - lastBlink) < timeBetweenBlinks); //if false that means that it was a synch blink
  Serial.print("Condition 2: ");
  Serial.println((systemTime - lastBlink) != ARDUINONUMBER*70); //if false that means that it was a counting blink*/
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
    int signalsBad = 0;
    int averageStrength = 0;
    for(int a = 0; a < counter; a++){  //ZERO AVERAGESTRENGTH BETWEEN ITERATIONS OF THE EMBEDDED FOR LOOP
      averageStrength = 0;
      for(int b = 0; b < 10; b++){
        averageStrength += signalStrength[a][b];
      }
      averageStrength /= 10; //average of the signal strengths in the arrays
      if(averageStrength == 0 || averageStrength < -50 || received[a] == false){  //FIX DIRECTION OF COMPARATOR AND ADD CLAUSE FOR IF IT = 0
        //Serial.print(names[a]);
        //Serial.println(" was bad");
        signalsBad++; //number of signals which are not received or not in range  //WAIT THE DIRECTION SHOULD BE OK NOW THAT SWAPPED TO SIGNALS BAD
      }
      else{
        //Serial.print(names[a]);
        //Serial.println(" was good");
      }
      received[a] = false; //zeroes the boolean
    }
    //Serial.print("Number of fireflies not in range: ");
    //Serial.println(signalsBad);
    char radiopacket[20] = "ThisIsALongBlink";
    //Serial.print("Sending "); Serial.println(radiopacket);
    rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
    rf69.waitPacketSent(); //send the synchronization blink
    //for(int a = 0; a < counter; a++){ //outputs all of the values in the arrays
      //Serial.print(names[a]);
      //Serial.print(millis());
      //Serial.print(",");
      //Serial.println(averageStrength);
      //for(int b = 0; b < 10; b++){
        //Serial.print(", ");
        //Serial.print(signalStrength[a][b]);
      //}
      //Serial.println();
    //}
    uint32_t c = 0; //it is grbw (green, red, blue, white)
    if(signalsBad == counter){
      c = strip.Color(0, 255, 0); //red
    }
    else if(signalsBad > 0){
      c = strip.Color(0, 0, 255); //blue
    }
    else{
      c = strip.Color(255, 0, 0); //green
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
    lastBlink = millis(); //sets the last time it blinked to the millis() once it is going to start a new cycle
    uint8_t ignoreThis[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t ignoreThisLen = sizeof(ignoreThis);
    rf69.recv(ignoreThis, &ignoreThisLen);


    /*uint8_t buf[RH_RF69_MAX_MESSAGE_LEN]; //required for radio, this is where the signals go
    uint8_t len = sizeof(buf); //required for radio, this is the length of the signal
    rf69.recv(buf, &len)*/
  }
}
