#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(13, 12, 11, 8, 7, 6, 5);
String str;

#define pass(void);
SoftwareSerial mySerial(3, 2);
const int sensor_pin = A1;  /* Soil moisture sensor O/P pin */

const int Red_Daytime_No_water = 10;
const int Green_Water_Present = 9;
const int White_Night_No_Water = 4;

int Night_Value = 6;     //at night LDR value is of high resistance. hence adc reads a value greater than this
int minimum_moisture_level = 25;

#define field1 true
bool whos_turn = field1;


const int solenoidPin = 0;   //solenoid pin which turns on the solenoid valve for irrigation
int LDR_Pin = A0;
int LDR_Value = 0;


int counter = 0;     //used to send the message once. WHile loop within the main loop


void setup() {
      Serial.begin(9600); /* Define baud rate for serial communication */
      mySerial.begin(9600);
      Serial.println("Initializing...");
      pinMode(solenoidPin, OUTPUT);
      pinMode(Red_Daytime_No_water, OUTPUT);
      pinMode(Green_Water_Present, OUTPUT);
      pinMode(sensor_pin, INPUT);
      pinMode(LDR_Pin, INPUT);

      lcd.begin(16,2);   //initializes lcd
      lcd.clear();       //clears the lcd
      lcd.setCursor(0,0);
      lcd.print("Initializing...");

      digitalWrite(Red_Daytime_No_water ,HIGH);
      digitalWrite(Green_Water_Present ,HIGH);
      digitalWrite(White_Night_No_Water ,HIGH);     
      delay(1000);
      lcd.clear(); 

      digitalWrite(Red_Daytime_No_water ,LOW);
      digitalWrite(Green_Water_Present ,LOW);
      digitalWrite(White_Night_No_Water ,LOW);
}


void loop()
{
      lcd.clear();
      //lcd.setCursor(0,0);
      lcd.print("Moisture: ");
      lcd.setCursor(0,12);
      
      float moisture_percentage;
      int sensor_analog;
      sensor_analog = analogRead(sensor_pin);         //moisture sensor 
      moisture_percentage = ( 100 - ( (sensor_analog/1023.00) * 100 ) );

      LDR_Value = analogRead(LDR_Pin);    //light dependent resistor
      Serial.print("Moisture Percentage = ");
      Serial.print(moisture_percentage);
      lcd.print(moisture_percentage);
      Serial.print("\n");
      Serial.print("\n");
      
      Serial.print("Light intensity: ");
      Serial.print(LDR_Value);
      Serial.print("\n");
      Serial.print("\n");

     // Send_to_thingspeak(moisture_percentage);
        
      if(moisture_percentage < minimum_moisture_level && LDR_Value > Night_Value)   //daytime, no water
      {

          digitalWrite(Red_Daytime_No_water, HIGH);
          digitalWrite(Green_Water_Present, LOW);
          digitalWrite(White_Night_No_Water, LOW);
          //send the message only once, but continue posting to cloud
          while(counter < 1)
          {
            Send_Message();
            counter++;
          }
          
          Solenoid_Irrigate();

         //continue posting to cloud
         Send_to_thingspeak(moisture_percentage, LDR_Value);    
     }
       
     else if (moisture_percentage > minimum_moisture_level && LDR_Value > Night_Value)  //daytime, there is enough water
     {
      Send_to_thingspeak(moisture_percentage, LDR_Value);
      digitalWrite(Red_Daytime_No_water,LOW);
      digitalWrite(Green_Water_Present, HIGH);   //daytime
      digitalWrite(White_Night_No_Water, LOW);
      Send_to_thingspeak(moisture_percentage, LDR_Value);
      counter = 0;
     }

     else if (moisture_percentage < minimum_moisture_level && LDR_Value <= Night_Value)  //night, no water 
     {
      digitalWrite(Red_Daytime_No_water,LOW);
      digitalWrite(White_Night_No_Water, HIGH);
      digitalWrite(Green_Water_Present, LOW);
      Send_to_thingspeak(moisture_percentage, LDR_Value);
      counter = 0;
     }

     else if(moisture_percentage > minimum_moisture_level && LDR_Value <= Night_Value)  //night time, enough water green lights
     {
      digitalWrite(Red_Daytime_No_water,LOW);
      digitalWrite(Green_Water_Present, HIGH);
      digitalWrite(White_Night_No_Water, LOW);
      Send_to_thingspeak(moisture_percentage, LDR_Value);
      counter = 0;
      //delay(300);
      //digitalWrite(White_Night_No_Water, LOW);
      //delay(300);
     }
     else
     {
      //all conditions covered. do nothing
      
     }   
}


void Send_Message()
{ 
        mySerial.println("AT");
        Update_Serial();
        mySerial.println("AT+CMGF=1");
        Update_Serial();
        mySerial.println("AT+CMGS=\"+254710997855\"");
        Update_Serial();
        mySerial.print("Moisture below average level");
        mySerial.write(26);
        Update_Serial();
        delay(300); 
}


void Send_to_thingspeak(float moisture_percentage, int light_intensity)
{
      //digitalWrite(Red_Daytime_No_water, HIGH);
  
      mySerial.println("AT");
      delay(300);
      mySerial.println("AT+CPIN?");
      delay(300);
      mySerial.println("AT+CREG?");
      delay(300);
      mySerial.println("AT+CGATT?");
      delay(300);
      mySerial.println("AT+CIPSHUT");
      delay(700);
      mySerial.println("AT+CIPSTATUS");
      delay(1000);
      mySerial.println("AT+CIPMUX=0");
      delay(700);
      Update_Serial();
      
      mySerial.println("AT+CSIT=\"internet\"");  //start task and setting the APN
      delay(1000);

      Update_Serial();
      mySerial.println("AT+CIICR");//bring up wireless connection
      delay(500);
     
      Update_Serial();
     
      mySerial.println("AT+CIFSR");//get local IP adress
      delay(400);
     
      Update_Serial();
     
      mySerial.println("AT+CIPSPRT=0");
      delay(300);
     
      Update_Serial();
      
      mySerial.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection
      delay(2000);
     
      Update_Serial();
     
      mySerial.println("AT+CIPSEND");//begin send data to remote server
      delay(2000);
      Update_Serial();

      //this helps alternate between updating field1 and field2 on thingspeak server

      if (whos_turn == field1)
      {
        str="GET https://api.thingspeak.com/update?api_key=10Y25WONTUODL1I8&field1=0" + String(moisture_percentage);
      }

      else
      {
        str="GET https://api.thingspeak.com/update?api_key=10Y25WONTUODL1I8&field2=0" + String(light_intensity);
        
      }
      whos_turn = !whos_turn;  //change the boolean expression so that the other data is sent next.
      
    //  String str="GET https://api.thingspeak.com/update?api_key=10Y25WONTUODL1I8&field1=0" + String(moisture_percentage);

      
      mySerial.println(str);//begin send data to remote server
      delay(3000);
      Update_Serial();
    
      mySerial.println((char)26);//sending
      delay(3000);//waitting for reply, important! the time is base on the condition of internet 
      mySerial.println();
     
      Update_Serial();
     
      mySerial.println("AT+CIPSHUT");//close the connection
      delay(100);
      Update_Serial();         

      //digitalWrite(Red_Daytime_No_water, LOW);
      //delay(1000);
}



void Update_Serial()
{
      delay(500);
      while(Serial.available())
      {
        mySerial.write(Serial.read());
      }
    
      while(mySerial.available())
      {
        Serial.write(mySerial.read());
        
      }
 
}


void Solenoid_Irrigate()
{
      digitalWrite(solenoidPin, HIGH);
      delay(600000);
      digitalWrite(solenoidPin, LOW);
      delay(500);
}
