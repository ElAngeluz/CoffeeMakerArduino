
/**
 * \file
 *       ESP8266 MQTT Bridge example
 * \author
 *       Tuan PM <tuanpm@live.com>
 */
#include <SoftwareSerial.h>
#include <espduino.h>
#include <mqtt.h>
#include <IRTemp.h>

#define SSID  "CTI_DOMO"     
#define PASSWORD "ct1esp0l15" 
#define HOST_NAME   "m10.cloudmqtt.com"
#define HOST_PORT   19823

// IRTEMP SENSOR
// Pines de conexión para el sensor de temperatura
static const byte PIN_DATA    = 5; // Choose any pins you like for these
static const byte PIN_CLOCK   = 6;
static const byte PIN_ACQUIRE = 7;

static const TempUnit SCALE = CELSIUS;  // Options are CELSIUS, FAHRENHEIT

IRTemp irTemp(PIN_ACQUIRE, PIN_CLOCK, PIN_DATA);

#define RELAY_PIN 8 // Pin de comunicacion entre el Relay y arduino

String estadOn_Off="";
int estadoProximidad=0;
float irTemperature=0;

//Definicion de la estructura para la maquina de estado
enum estados {
  A,B,C,D,E
};
estados estado=A;

SoftwareSerial debugPort(2, 3); // RX, TX
ESP esp(&debugPort, &Serial, 4);
MQTT mqtt(&esp);

boolean wifiConnected = false;
unsigned long oldTime = 0;
const unsigned long tiempoEspera = 15000; //15 segundos
char buffer[10];

void wifiCb(void* response)
{
  uint32_t status;
  RESPONSE res(response);

  if(res.getArgc() == 1) {
    res.popArgs((uint8_t*)&status, 4);
    if(status == STATION_GOT_IP) {
      Serial.println("WIFI CONNECTED");
      wifiConnected = true;
      mqtt.connect(HOST_NAME, HOST_PORT);
      //mqtt.connect("m10.cloudmqtt.com", 19823, false);
      //mqtt.connect("broker.mqttdashboard.com", 1883); /*without security ssl*/
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
  mqtt.subscribe("LedOnOff"); //or mqtt.subscribe("topic"); /*with qos = 0*/
  mqtt.publish("LedOnOff", "data0");

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
  if (data == "H"){
    digitalWrite(9, HIGH);
  } else if (data == "L"){
    digitalWrite(9, LOW);
  }
}
void mqttPublished(void* response)
{

}
void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  
  Serial.begin(19200);
  debugPort.begin(19200);
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

  esp.wifiConnect(SSID ,PASSWORD );


  Serial.println("ARDUINO: system started");
}

void loop() {
  esp.process();
  static int relayVal = 0;
  if ((millis() - oldTime) >= tiempoEspera){
    oldTime = millis();  
    //Temperature:
    irTemperature = irTemp.getIRTemperature(SCALE);
    mqtt.publish("temp/ir",  dtostrf(irTemperature, 4, 3, buffer) );
    float ambientTemperature = irTemp.getAmbientTemperature(SCALE);
    mqtt.publish("temp/amb", dtostrf(ambientTemperature, 4, 3, buffer));
  }
  if(wifiConnected) {
  }
}
