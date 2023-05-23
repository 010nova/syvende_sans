// globale variabler
bool strom;
bool modus; // true = steke, false = vannbad
String klokke = "00:00";

//pins
int paaKnapp = 10;
int modusKnapp = 2;
int ledSteke = 3;
int ledVannbad = 4;
int registrerKnapp = 5;
// skjermen er A4 og A5

// For skjerm
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128    // OLED display width
#define SCREEN_HEIGHT 64    // OLED display height
#define OLED_RESET     -1   // Reset pin number, -1 means Arduino reset pin
#define SCREEN_ADDRESS 0x3C // Address may be either 3C or 3d
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// for termistor
#include <math.h>
int sensorPin = A0;       // analoge pinen termistoren er koblet til
int sensorValue;          // verdien til A0
float Vin = 5;            // spenning inn
float Vout;               // spenning ut
float Rref = 999;         // Referanse resistans
float T;                  // temperatur i flyt tall
String temp;              // temperatur som string


void setup() {
  Serial.begin(9600);  
  
  // sjekker om skjermen er koblet opp riktig
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Looper forevig om noe er galt, programmet stopper
  }

  strom = false;    // artefakten starter avskrudd
  modus = true;     // Modus starter som steke

  // pinmodes
  pinMode(modusKnapp, INPUT_PULLUP);
  pinMode(paaKnapp, INPUT_PULLUP);
  pinMode(registrerKnapp, INPUT_PULLUP);
  pinMode(ledSteke, OUTPUT);
  pinMode(ledVannbad, OUTPUT);

  // Starts I2C communication with the wifi-connected ESP32
  Wire.begin();
}

void loop() {
  // sjekker om den skal være på eller av
  strom = sjekkKnapp(paaKnapp, strom);

  // om den er skrudd på
  if(strom) { 
    paa();
  }
  else {
    av();
  }
}

void paa(){
  T = sjekkTemp();
  temp = String(T);
  sjekkRegistrer();
  oppdaterKlokke();
  
  if(T < -100 || T > 1000){
    displayPrint("Termometer\nfrakoblet" , 2);
    //delay(200); 
  }
  
  else{
    modus = sjekkKnapp(modusKnapp, modus);

    // gjør at riktig LED lyser
    if (modus){
      digitalWrite(ledSteke, HIGH);
      digitalWrite(ledVannbad, LOW);
    }
    else{
      digitalWrite(ledVannbad, HIGH);
      digitalWrite(ledSteke, LOW);
    }

    // skriver til skjermen
    displayPrint(temp + "C \n" + klokke , 3);
    delay(500);   // skjermen oppdaterer seg hver 0.5 sekund
  } 
}

void av(){
  // skrur av LED-ene
  digitalWrite(ledSteke, LOW);
  digitalWrite(ledVannbad, LOW);

  // fjerner innhold fra skjermen
  displayPrint("",0);
}

float sjekkTemp(){
  sensorValue = analogRead(sensorPin);      // henter verdi fra termistor
  Vout = (Vin * sensorValue) / 1023;        // Måler spenning ut
  int res = Rref * (1 / ((Vin / Vout) -1)); // Måler resistansen i termistoren

  return 158.8 - 23.69 * log(res);
}

void oppdaterKlokke(){
    // ser om data blir sendt
  if (Serial.available()) {  
    // lager en variabel tilsvarer stringen som blir sendt
    String motatt = Serial.readStringUntil('\n');

    int lengde = motatt.length();
    if(lengde == 5){
      klokke = motatt;  
    }
  }

}

// funksjon som tar inn en knapp den skal registrere, og variebelen trykket skal endre
bool sjekkKnapp(int knapp, bool variabel){
  int trykk = digitalRead(knapp);

  if(trykk == HIGH){        // dersom knappen blir trykket
    while(trykk == HIGH){   // venter til knappen blir sluppet 
      trykk = digitalRead(knapp);
      delay(50);            // for å unngå debounce
    }
    return !variabel;
  }
  return variabel;
}

void sjekkRegistrer(){
  bool trykk = true;

  // om registrer knappen blir trykket
  if (trykk != sjekkKnapp (registrerKnapp, trykk)){
    registrer();
  }
}

void registrer(){
  // Transmit the measured temperature value to the wifi-connected 
  Wire.beginTransmission(9); // transmit to device #9
  if(!modus) {
    char signal = 'S';
    Wire.write((byte *) &signal, sizeof (signal));   // sends type of signal as char 
    Wire.write((byte *) &T, sizeof (T));   // sends temperature as bytes 
  }
  else {
    char signal = 'V';
    Wire.write((byte *) &signal, sizeof (signal));   // sends type of signal as char 
    Wire.write((byte *) &T, sizeof (T));   // sends temperature as bytes 
  }
  Wire.endTransmission(); // end transmission

  // gjør at skjermen viser riktig
  Serial.println("registrert " + temp);
  String tekst = "Temperatur\nregistrert";
  int storrelse = 2;
  displayPrint(tekst, storrelse);
  delay(2000);
}

void displayPrint(String tekst, int storrelse){
  display.clearDisplay();       // tømmer skjermen
  display.setTextSize(storrelse);    // setter riktig skriftstørrelse   
  display.setTextColor(SSD1306_WHITE);       
  display.setCursor(0,0);       // starter fra øverst til venstre
  display.println(tekst);       // skriver ut tekst
  display.display();            // starter å vise
}

