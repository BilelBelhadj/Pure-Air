/*
Auteur    : Bilel Belhadj
Titre     : Air purification intelligent system
Date      : 06-02-2023
Version   : 0.0.1
*/

#include <Arduino.h>
#include <Adafruit_AHTX0.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "WIFIConnector_MKR1000.h"
#include "MQTTConnector.h" 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_AHTX0 aht;
sensors_event_t hum, temp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Constants
const int CO_SEN = A3;
const int FAN = 1;      //la broche relier au relai pour activer le ventilateur

//Variables
float tmperature = 0, humidite = 0;
int   carbone = 0;


void setup()
{
    Serial.begin(9600);     //demarrer le moniteur serie avec le vitesse 9600
    pinMode(FAN, OUTPUT);   //mode de fonctionnement de broche

    //Verifier l'existence du AHT20
    if (aht.begin()) {
        Serial.println("Found AHT20");
    } else {
        Serial.println("Didn't find AHT20");
    }

    //verifier l'existance du l'ecran OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }

    //effacer l'ecran et configurer le couleur
    display.clearDisplay();
    display.setTextColor(WHITE);

    //connecter sur internet et MQTT
    wifiConnect();              
    MQTTConnect();
}


void loop()
{
    ClientMQTT.loop(); //continuer a ecouter s'il y'a des RPC

    /******************************* Capture des donnees *******************************/
    //AHT20 data 
    aht.getEvent(&hum, &temp);
    tmperature = temp.temperature;
    humidite = hum.relative_humidity;
    carbone = analogRead(CO_SEN);       //CO data



    /******************************* Gerer les actuateurs *******************************/
    if (tmperature > 25)
    {
        etatFan = 1;
        digitalWrite(FAN, HIGH);
    }else{
        etatFan = 0;
        digitalWrite(FAN, LOW);
    }

    if(etatFanStr == "true"){
        if (digitalRead(FAN) == 1)
        {
            Serial.println("Le filtre est deja activee");
        }else{
            Serial.println("Activation....Done!");
            digitalWrite(FAN, HIGH);
        } 
    }else{
        if (digitalRead(FAN) == 0)
        {
            Serial.println("Le filtre est deja arretee");
        }else{
            Serial.println("desactivation....Done!");
            digitalWrite(FAN, LOW);
        } 
    }

    /******************************* Affichage des donnees *******************************/
    display.clearDisplay();  //effacer l'ecran

    //display temperature
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("Temp:");
    display.setTextSize(2);
    display.setCursor(0,10);
    display.print(String(tmperature));
    
    //display humidity
    display.setTextSize(1);
    display.setCursor(65, 0);
    display.print("HUM: ");
    display.setTextSize(2);
    display.setCursor(65, 10);
    display.print(String(humidite));

    //display quantite du CO2
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("CO2: ");
    display.setTextSize(2);
    display.setCursor(0, 45);
    display.print(String(carbone));

    //display statu du filtre
    display.setTextSize(1);
    display.setCursor(65, 35);
    display.print("Filtre: ");
    display.setTextSize(2);
    display.setCursor(65, 45);
    
    if (etatFan == 1)
    {
        display.print(String("ON"));
    }else{
        display.print(String("OFF"));
    }
    display.display(); 


    /******************************* envoyer les donnees sur thingsboard *******************************/

    appendPayload("CO2", carbone);
    appendPayload("Temperature", tmperature);
    appendPayload("Himidite", humidite);
    appendPayload("Filtre", etatFan);
    sendPayload();
   
    delay(3000);
}
