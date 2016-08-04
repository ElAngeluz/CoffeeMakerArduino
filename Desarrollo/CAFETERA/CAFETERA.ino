//codigo para cactus micro v2
#include <espduino.h>
#include <mqtt.h>
#include <IRTemp.h>

#define PIN_ENABLE_ESP 13
#define SSID  "LabProto"
#define PASS  "password"

ESP esp(&Serial1, &Serial, PIN_ENABLE_ESP);
MQTT mqtt(&esp);
boolean wifiConnected = false;
char buffer[10];

// IRTEMP SENSOR
// Pines de conexión para el sensor de temperatura
static const byte PIN_DATA    = 5; // Choose any pins you like for these
static const byte PIN_CLOCK   = 6;
static const byte PIN_ACQUIRE = 7;
static const TempUnit SCALE = CELSIUS;  // Options are CELSIUS, FAHRENHEIT
IRTemp irTemp(PIN_ACQUIRE, PIN_CLOCK, PIN_DATA);

// Sensor de proximidad
#define PROX_SENSOR_PIN 8
int sensorValue = 0;
int olDsensorValue = 0; 
int estadoProximidad=0;

// RELE - ARDUINO
#define RELAY_PIN 9 

//Definicion de la estructura para la maquina de estado
enum estados {
  A,B,C,D,E
};

String estadOn_Off="";
float irTemperature=0;
estados estado=A;

void wifiCb(void* response)
{
  uint32_t status;
  RESPONSE res(response);

  if(res.getArgc() == 1) {
    res.popArgs((uint8_t*)&status, 4);
    if(status == STATION_GOT_IP) {
      Serial.println("WIFI CONNECTED");
      mqtt.connect("m10.cloudmqtt.com", 19823, false);
      wifiConnected = true;
      //or mqtt.connect("host", 1883); /*without security ssl*/
    } else {
      wifiConnected = false;
      mqtt.disconnect();
    }
    
  }
}

void mqttConnected(void* response)
{
  Serial.println("Connected");
  mqtt.subscribe("/OnOff"); //or mqtt.subscribe("topic"); /*with qos = 0*/
  mqtt.publish("/Cafetera","data0");
}

void mqttDisconnected(void* response)
{

}
void mqttData(void* response)
{
  RESPONSE res(response);

  Serial.print("Received: topic=");
  String topic = res.popString();
  Serial.println(topic);

  Serial.print("data=");
  String data = res.popString();
  Serial.println(data);
  if (data == "ON")
    digitalWrite(RELAY_PIN,HIGH); 
  else if (data == "OFF")
    digitalWrite(RELAY_PIN,LOW);
  

}
void mqttPublished(void* response)
{

}
void setup() {
  Serial1.begin(19200);
  Serial.begin(19200);
  pinMode(RELAY_PIN,OUTPUT);
  pinMode(PROX_SENSOR_PIN, INPUT);
  esp.enable();
  delay(500);
  esp.reset();
  delay(500);
  while(!esp.ready());
  Serial.println("ARDUINO: setup mqtt client");
  //if(!mqtt.begin("nencvwor", "nencvwor", "RCI7Yxtvr1_J", 120, 1)) {
  if(!mqtt.begin("nencvwor", "nencvwor", "RCI7Yxtvr1_J", 120, 1)) {
    Serial.println("ARDUINO: fail to setup mqtt");
    while(1);
  }


  Serial.println("ARDUINO: setup mqtt lwt");
  mqtt.lwt("/lwt", "offline", 0, 0); //or mqtt.lwt("/lwt", "offline");

/*setup mqtt events */
  mqtt.connectedCb.attach(&mqttConnected);
  mqtt.disconnectedCb.attach(&mqttDisconnected);
  mqtt.publishedCb.attach(&mqttPublished);
  mqtt.dataCb.attach(&mqttData);

  /*setup wifi*/
  Serial.println("ARDUINO: setup wifi");
  esp.wifiCb.attach(&wifiCb);

  esp.wifiConnect(SSID, PASS);


  Serial.println("ARDUINO: system started");
}

void loop() {
  esp.process();
  if(wifiConnected) {
      irTemperature = irTemp.getIRTemperature(SCALE);
      mqtt.publish("/temp/ir",  dtostrf(irTemperature, 4, 3, buffer) );
      sensorValue = digitalRead(PROX_SENSOR_PIN);
      if (sensorValue == 1 && olDsensorValue == 0)
        mqtt.publish("/prox", "0");
      else if (sensorValue == 0 && olDsensorValue == 1)
        mqtt.publish("/prox", "1");
      olDsensorValue = sensorValue;
  }
}

