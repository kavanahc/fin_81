#include <MQUnifiedsensor.h>
#include <SoftwareSerial.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include<ThingerWifi.h>
ThingerWifi thing("AKK", "Alert", "9731876928");
#include "OneWire.h"
#include <MAX30100_PulseOximeter.h>
#define DHTTYPE DHT22
#define DHTPin D5
#define buzzer D6
#define Temp_sensor D7  
const char* ssid = "Xiaomi 11i";  
const char* password = "12345678";  
ESP8266WebServer server(80);
DHT dht(DHTPin, DHTTYPE);
OneWire oneWire(Temp_sensor);
DallasTemperature sensors(&oneWire);
#define USE_ARDUINO_INTERRUPTS true   // Set-up low-level interrupts for most acurate BPM math
#define REPORTING_PERIOD_MS    1000
//Definitions
#define placa "ESP-8266"
#define Voltage_Resolution 3.3
#define pin A0 //Analog input 0 of your arduino
#define type "MQ-135" //MQ135
#define ADC_Bit_Resolution 10 // For arduino UNO/MEGA/NANO
#define RatioMQ135CleanAir 3.6//RS / R0 = 3.6 ppm  
PulseOximeter pox;
uint32_t tsLastReport = 0;
//#define calibration_button 13 //Pin to calibrate your sensor
//OneWire oneWire(Temp_sensor);

//Declare Sensor
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

float CO2=0,Co2=0,CO=0,body_Temp=0,BPM,Spo2;
float  temperature,humidity;


void setup() 

{
  pinMode(16, OUTPUT);
  thing.add_wifi("Xiaomi 11i","12345678");
  pinMode(buzzer,OUTPUT);
  pinMode(Temp_sensor,INPUT);
  Serial.begin(9600);
  dht.begin();
  WiFi.begin(ssid, password);
  //Init the serial port communication - to debug the library
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
  
  server.begin();
  Serial.println("HTTP server started");
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b    
  MQ135.init(); 
  calibrate_MQ135(); 
   if (!pox.begin()) 
   {
    Serial.println("FAILED");
  } 
  else 
  {
    Serial.println("SUCCESS");
   }
}
void calibrate_MQ135()
{
  
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0/10);
  Serial.println("  done!.");

  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/
  delay(1000);
  }

void readsensors()
{
  MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
  MQ135.setA(605.18); MQ135.setB(-3.937); // Configure the equation to calculate CO concentration value
  CO = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
  MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to calculate CO2 concentration value
  CO2 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
  temperature = dht.readTemperature();
  humidity= dht.readHumidity();
  sensors.requestTemperatures(); 
  body_Temp=sensors.getTempCByIndex(0);
}

void Serialprint()
{
  Serial.print("CO2 in PPM "); Serial.println(Co2=CO2+400);
  Serial.print("CO in PPM ");  Serial.println(CO);
  Serial.print("temperature in degree Cel ");Serial.println(temperature);
  Serial.print("Humidity in %RH ");Serial.println(humidity);
  Serial.print("Body temperature:");Serial.println(body_Temp);
  delay(1000); 
  }
void beep(unsigned long int i)
{
    digitalWrite(buzzer,HIGH);
    delay(i);
    digitalWrite(buzzer,LOW); 
}
void loop() 
{
  server.handleClient();
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS)
  {  
    BPM = pox.getHeartRate();
    Spo2 = pox.getSpO2();
    Serial.print("BPM: ");
    Serial.println(BPM); 
    Serial.print("SpO2: ");
    Serial.print(Spo2);
    Serial.println("%");
    tsLastReport = millis();
  }
  readsensors();
  Serialprint();
  
  if(CO2>450)
  {
   unsigned long int d1=100;
    beep(d1);
    thing.handle();
    thing.call_endpoint("EmailSer"); 
    delay(3000);
    }
  if(CO>20)
  {
 unsigned long int d2=200;
    beep(d2);
  }
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}
void handle_OnConnect() 
{
  
   server.send(200, "text/html", SendHTML(Co2,CO,temperature,humidity,body_Temp,BPM,Spo2));
  
}
String SendHTML( float Co2, float CO, float temp, float humi,float body_Temp,float BPM, float Spo2 )
{
  
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr+="<meta http-equiv='refresh' content='5'>";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>IoT Assisted Health Alert Report</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #0000FF;margin: 50px auto 30px;}\n";
  ptr +="p{font-size: 24px;color: #12AD2B;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>IoT Assisted Parameter Monitoring for health maitainance for senior Citizens </h1>\n";
  ptr +="<h2>Presented by AIEMS students,Bidadi </h2>\n";
  ptr +="<h3>Monitoring following parameters </h3>\n";
  ptr +="<p>Co2: ";
  ptr +=(float)Co2;
  ptr +="ppm</p>";
 
  ptr +="<p>CO: ";
  ptr +=(float)CO;
  ptr +=" ppm</p>";

  ptr +="<p>Temperature: ";
  ptr +=(float)temp;
  ptr +="DegC</p>";

  
  ptr +="<p>Humidity: ";
  ptr +=(float)humi;
  ptr +="%RH</p>";
  
  ptr +="<p>Body temperature:";
  ptr +=(float)body_Temp;
  ptr +="Deg C</p>";

  ptr +="<p>Heart Rate:";
  ptr +=(int)BPM;
  ptr +="BPM</p>";

  ptr +="<p>SPo2:";
  ptr +=(float)Spo2;
  ptr +="%</p>";
  
  return ptr;
}
