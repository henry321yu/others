#include "max6675.h"
#include<stdio.h> 
#include<math.h>
#include <SPI.h> 
#include <SD.h>

// Deklaration für die Thermoelemente

int thermoDO = 51;
int thermoCLK = 52;

int thermoCS1 = 22;
int thermoCS2 = 24;
int thermoCS3 = 26;
int thermoCS4 = 28;
int thermoCS5 = 30;
int thermoCS6 = 32;

float Temp1 = 0;

MAX6675 thermocouple11(thermoCLK, thermoCS1, thermoDO);
MAX6675 thermocouple12(thermoCLK, thermoCS2, thermoDO);
MAX6675 thermocouple13(thermoCLK, thermoCS3, thermoDO);
MAX6675 thermocouple14(thermoCLK, thermoCS4, thermoDO);
MAX6675 thermocouple15(thermoCLK, thermoCS5, thermoDO);
MAX6675 thermocouple16(thermoCLK, thermoCS6, thermoDO);

//Dekleration für die Drucksensoren

float AI_Druck1 = 0;
float AI_Druck2 = 0;
float AI_Druck3 = 0;
float AI_Druck4 = 0;
float AI_Druck5 = 0;
float AI_Druck6 = 0;

float Druck1 = 0;
float Druck2 = 0;
float Druck3 = 0;
float Druck4 = 0;
float Druck5 = 0;
float Druck6 = 0;

float X  = 211.03;        //Aretmetischer Mittelwert des Umrechnungsfaktors X
float Vs = 4.84;          //gemessene Versorhungspannung direkt am Drucksensor

// Dekleration der Datensicherung

const int chipSelect = 4;

void setup() {
 Serial.begin(9600);
 delay(500);                                          //Wartezeit für Drucksensoren und A/D Wandler der Thermoelemente

  while (!Serial) { ; }
  Serial.print("Initialisiere SD-Karte");
  if (!SD.begin(chipSelect)) {
    Serial.println("Karte nicht gefunden");
    return;
  }
  Serial.println("Karte erfolgreich initialisiert");
  delay(100);
}

void loop() {
// Druck Ausgabe

 AI_Druck1 = analogRead(A0);
 AI_Druck2 = analogRead(A1);
 AI_Druck3 = analogRead(A2);
 AI_Druck4 = analogRead(A3);
 AI_Druck5 = analogRead(A4);
 AI_Druck6 = analogRead(A5);

 Druck1 = ((AI_Druck1 - X * Vs * 0.04) / (X * Vs * 0.0012858))/100;
 Serial.print("Druck1 = ");
 Serial.println(Druck1);
 Druck2 = ((AI_Druck2 - X * Vs * 0.04) / (X * Vs * 0.0012858))/100;
 Serial.print("Druck2 = ");
 Serial.println(Druck2);
 Druck3 = ((AI_Druck3 - X * Vs * 0.04) / (X * Vs * 0.0012858))/100;
 Serial.print("Druck3 = ");
 Serial.println(Druck3);
 Druck4 = ((AI_Druck4 - X * Vs * 0.04) / (X * Vs * 0.0012858))/100;
 Serial.print("Druck4 = ");
 Serial.println(Druck4);
 Druck5 = ((AI_Druck5 - X * Vs * 0.04) / (X * Vs * 0.0012858))/100;
 Serial.print("Druck5 = ");
 Serial.println(Druck5);
 Druck6 = ((AI_Druck6 - X * Vs * 0.04) / (X * Vs * 0.0012858))/100;
 Serial.print("Druck6 = ");
 Serial.println(Druck6);
 
// Themperatur Ausgabe
 
  Serial.print("T1 = "); 
  Serial.println(thermocouple11.readCelsius());


  Serial.print("T2 = "); 
  Serial.println(thermocouple12.readCelsius());


  Serial.print("T3 = "); 
  Serial.println(thermocouple13.readCelsius());


  Serial.print("T4 = "); 
  Serial.println(thermocouple14.readCelsius());


  Serial.print("T5 = "); 
  Serial.println(thermocouple15.readCelsius());


  Serial.print("T6 = "); 
  Serial.println(thermocouple16.readCelsius());


  //Temp1 = thermocouple11.readCelsius();
  

  // Datenspeicherung
 
  File dataFile = SD.open("datalog.txt", FILE_WRITE);


  if (dataFile) {
    dataFile.print(Druck1);
    dataFile.print(Druck2);
    dataFile.print(Druck3);
    dataFile.print(Druck4);
    dataFile.print(Druck5);
    dataFile.print(Druck6);
    dataFile.print(thermocouple11.readCelsius());
    dataFile.print(thermocouple12.readCelsius());
    dataFile.print(thermocouple13.readCelsius());
    dataFile.print(thermocouple14.readCelsius());
    dataFile.print(thermocouple15.readCelsius());
    dataFile.print(thermocouple16.readCelsius());
  
    dataFile.println(";");                                                                               //Trennzeichen für die .csv Datei
    dataFile.close();
  } else {
    Serial.println("Fehler beim Öffnen!");                                                            // Falls die Datei nicht geöffnet werden kann, soll eine Fehlermeldung ausgegeben werden
  }
  delay(1000);

}
