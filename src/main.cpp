/*
Auteur    : Bilel Belhadj
Titre     : Air purification intelligent system
Date      : 06-02-2023
Version   : 0.0.1
*/

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_CCS811.h"
#include <Adafruit_AHTX0.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "WIFIConnector_MKR1000.h"
#include "MQTTConnector.h" 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//Aht20
Adafruit_AHTX0 aht;
sensors_event_t hum, temp;

//Ecran OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//CO2 
Adafruit_CCS811 ccs;

//Constants
const int CO_SEN = A3;
const int FAN = 6;      //la broche relier au relai pour activer le ventilateur

//Variables
float tmperature = 1000, humidite = 1000;
int   co2 = 60, tvoc = 0;
long  oldTime = 0, newTime = 0;


void setup()
{
    Serial.begin(9600);     //demarrer le moniteur serie avec le vitesse 9600

    wifiConnect();                  //connecter sur internet et MQTT   
    MQTTConnect();

    pinMode(FAN, OUTPUT);   //mode de fonctionnement de broche
     
    if (aht.begin()) {      //Verifier l'existence du AHT20
        Serial.println("Found AHT20");
    } else {
        Serial.println("Didn't find AHT20");
    }
   
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {      //verifier l'existance du l'ecran OLED
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }else{
        Serial.println("Screen checked");
    }

    Serial.println("CCS811 test");  //Co2 sensor check
    if(!ccs.begin()){
        Serial.println("Failed to start sensor! Please check your wiring.");
        while(1);
    }    

    display.clearDisplay();         //effacer l'ecran et configurer le couleur
    display.setTextColor(WHITE);

    while(!ccs.available());        // Wait for the sensor to be ready  
}


void loop()
{
    ClientMQTT.loop();     //continuer a ecouter s'il y'a des RPC

    newTime = millis();
    if (newTime - oldTime > 2000){
        oldTime = newTime;

        
        /******************************* Capture des donnees *******************************/
        //AHT20 data 
        aht.getEvent(&hum, &temp);
        tmperature = temp.temperature;
        humidite = hum.relative_humidity;
        
        
        if(ccs.available()){
            if(!ccs.readData()){
                co2 = ccs.geteCO2();
            }else{
                co2 = -1;
                while(1);
            }
        }


        /******************************* Gerer les actuateurs *******************************/
        
        if (tmperature > 26 || etatFanStr == "true" || co2 > 1500){         //activer le filtre selon la tempperature
            etatFan = 1;
            digitalWrite(FAN, HIGH);
        }else{
            etatFan = 0;
            digitalWrite(FAN, LOW);
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
        display.print(String(co2));

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
        
        Serial.println("TMP");
        Serial.println(tmperature);
        Serial.println("HUM");
        Serial.println(humidite);
        Serial.println("CO2");
        Serial.println(co2);
        /******************************* envoyer les donnees sur thingsboard *******************************/
        appendPayload("CO2", co2);
        appendPayload("Temperature", tmperature);
        appendPayload("Himidite", humidite);
        appendPayload("Filtre", etatFan);
        sendPayload();
        
    }
}