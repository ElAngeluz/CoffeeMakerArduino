//codigo para arduino uno
#include <SoftwareSerial.h>
#include <espduino.h>
#include <mqtt.h>
#include <IRTemp.h>

// Pines de conexión para el sensor de temperatura
static const byte PIN_DATA    = 5; // Choose any pins you like for these
static const byte PIN_CLOCK   = 6;
static const byte PIN_ACQUIRE = 7;

static const TempUnit SCALE=CELSIUS;  // Options are CELSIUS, FAHRENHEIT

IRTemp irTemp(PIN_ACQUIRE, PIN_CLOCK, PIN_DATA);

#define SSID  "CTI_DOMO"     
#define PASSWORD "ct1esp0l15" 
#define HOST_NAME   "m10.cloudmqtt.com"
#define HOST_PORT   19823
#define HOST_NAMET   "api.thingspeak.com"
#define HOST_PORTT   80
#define RELAY_PIN 9 // Pin de comunicacion entre el Relay y arduino

//El modulo ESP8266 usa SoftwareSerial...
SoftwareSerial debugPort(2, 3); // RX, TX
ESP esp(&debugPort, &Serial, 4);
MQTT mqtt(&esp);


String estadOn_Off="";
int estadoProximidad=0;
float irTemperature=0;
boolean wifiConnected = false;
unsigned long oldTime = 0;
const unsigned long tiempoEspera = 15000; //15 segundos
char buffer[10];

//Definicion de la estructura para la maquina de estado
enum estados {
  A, //..noenvia datos
  B, //verifica que la taza este en su lugar
  C, //verifica el valor de la temperatura
  D, //estado que prende el sistema
  E  //predeterminado, no envia datos
};

estados estado=A;

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

void mqttDisconnected(void* response)
{

}

void mqttPublished(void* response)
{

}

void setup()
{
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(9600); // open serial
    //Configura el módulo ESP8266
  SetupWifi();
}

SetupWifi()
{
	Serial.print("setup begin\r\n");
    
    Serial.print("FW Version:");
    Serial.println(wifi.getVersion().c_str());
      
    while(!wifi.setOprToStation()) {
       Serial.print("to station + softap err\r\n");
       delay(1000);
    }
    Serial.print("to station + softap ok\r\n");
 
    while(!wifi.joinAP(SSID, PASSWORD)) {
        Serial.print("Join AP failure\r\n");
    }
    Serial.print("Join AP success\r\n");
    Serial.print("IP:");
    Serial.println( wifi.getLocalIP().c_str());    
    
    while(!wifi.enableMUX()) {
        Serial.print("multiple error\r\n");
        delay(1000);
    } 
    Serial.print("multiple ok\r\n");
    
    Serial.print("setup end\r\n");
}

void loop()
{
  static int relayVal = 0;
  //Proximity
  int sensorValue = analogRead(A0);
  // print out the value you read:
  Serial.print("Proximidad: ");
  Serial.println(sensorValue);
  delay(10);  
  //temperature:
  Serial.println("IRTemp example");
  Serial.println("~~~~~~~~~~~~~~");
   irTemperature = irTemp.getIRTemperature(SCALE);
  printTemperature("IR", irTemperature);

  float ambientTemperature = irTemp.getAmbientTemperature(SCALE);
  printTemperature("Ambient", ambientTemperature);

  if(sensorValue<=930){
    estadoProximidad=1;    
  }else{
      estadoProximidad=0;
  }

  delay(10);
 
    //Serial.print("Estado: "+estadOn_Off);
    Serial.print("VEr estado:");
    Serial.println(String(estado));

  //Maquina de estados:
    switch (estado)
    {
    case A:
      {
        digitalWrite(RELAY_PIN, LOW);
        getOn_Off_State();

        Serial.println("estado A");
        break;    
      }
    case B:
      {
        if(sensorValue<=930){
          estadoProximidad=1;
          estado=C;
        }else{
            estadoProximidad=0;
			estado=A;
            digitalWrite(RELAY_PIN, LOW);
            sendDatos();
        } 

        Serial.println("estado B");
        break;
      }
    case C:
      {
        if(irTemperature<=90)
          estado=D;
        else{
          estado=A;
          digitalWrite(RELAY_PIN, LOW);
          sendDatos();
        }

        Serial.println("estado C");
        break;
      }
     case D:
      {
        digitalWrite(RELAY_PIN, HIGH);
        estado=E;
        sendDatos();
        Serial.println("estado D");
        break;
      }
    case E:
      {
        Serial.println("estado E");
        getOn_Off_State();
        break; 
      }
    }
    estadOn_Off="";
    delay(100);
}

//Funcion que envía datos hacia el ThingSpeak, el de la temperatura y de proximidad.
void sendDatos(){
    String field3 = "&field3=";
    String field2="&field2=";
    String mensaje= "GET http://api.thingspeak.com/update?api_key=JOXZJ0MWXKS66ENR"+field2+String(irTemperature,2)+field3+ String(estadoProximidad)+"HTTP/1.0\r\n\r\n\r\n";
    //Serial.println(mensaje);
    
   String cmd = "AT+CIPSTART=2,\"TCP\",\"";  //make this command: AT+CPISTART="TCP","146.227.57.195",80
    cmd += HOST_NAME;
    cmd += "\",80";
  
    mySerial.println(cmd);  //send command to device
    //mySerial.println("AT+ CIPMUX=1");
      delay(5000);  //wait a little while for 'Linked' response - this makes a difference
    if(mySerial.find("Linked"))  //message returned when connection established WEAK SPOT!! DOESN'T ALWAYS CONNECT
    {
      Serial.print("Connected to server for data at ");  //debug message
      Serial.println(HOST_NAME);
    }
    else
    {
      Serial.println("'Linked' response not received");  //weak spot! Need to recover elegantly
    }
    
       Serial.print(mensaje);
     
     //send 
     mySerial.print("AT+CIPSEND=2,");
     mySerial.println(mensaje.length());

  if(mySerial.find(">"))    //prompt offered by esp8266
  {
    //Serial.println("found > prompt - issuing GET request");  //a debug message 
    mySerial.println(mensaje);
    
  }
  else
  {
      //doesn't seem to work here?
    Serial.println("No '>' prompt received after AT+CPISEND");
  }

  if (mySerial.find("Status")) //get the date line from the http header (for example)
  {
    for (int i=0;i<30;i++)  //this should capture the 'Date: ' line from the header
    {
      if (mySerial.available())  //new cahracters received?
      {
        char c=mySerial.read();
         Serial.write(c);
      }else i--;  //if not, keep going round loop until we've got all the characters
    }
    
  }
  mySerial.println("AT+CIPCLOSE");
    delay(15000);
  }
  
// Lectura del estado de prendido o apagado de la cafetera desde ThingSpeak
void getOn_Off_State(void){
 String cmd = "AT+CIPSTART=1,\"TCP\",\"";  //make this command: AT+CPISTART="TCP","146.227.57.195",80
  cmd += HOST_NAME;
  cmd += "\",80";

  mySerial.println(cmd);  //send command to device
     // mySerial.println("AT+ CIPMUX=1");
    delay(5000);  //wait a little while for 'Linked' response - this makes a difference
  if(mySerial.find("Linked"))  //message returned when connection established WEAK SPOT!! DOESN'T ALWAYS CONNECT
  {
    Serial.print("Connected to server at ");  //debug message
    Serial.println(HOST_NAME);
  }
  else
  {
    Serial.println("'Linked' response not received");  //weak spot! Need to recover elegantly
  }
  
   String sendcommand =  "GET http://api.thingspeak.com/channels/81636/fields/1/last.json HTTP/1.0\r\n\r\n\r\n";  //construct http GET request
  //cmd += "Host: api.thingspeak.com\r\n\r\n";        //test file on my web
       Serial.print(sendcommand);
     
     //send 
     mySerial.print("AT+CIPSEND=1,");
     mySerial.println(sendcommand.length());

  if(mySerial.find(">"))    //prompt offered by esp8266
  {
    //Serial.println("found > prompt - issuing GET request");  //a debug message 
    mySerial.println(sendcommand);
    
  }
  else
  {
      //doesn't seem to work here?
    Serial.println("No '>' prompt received after AT+CPISEND");
  }

  if (mySerial.find("field1")) //get the date line from the http header (for example)
  {
    for (int i=0;i<7;i++)  //this should capture the 'Date: ' line from the header
    {
      if (mySerial.available())  //new cahracters received?
      {
        char c=mySerial.read();  //print to console
        if(c!='"'&& c!=':'&& c!='}'){
          estadOn_Off.concat(c);
          }
      }else i--;  //if not, keep going round loop until we've got all the characters
    }
    
  }
  char estadoVER=estadOn_Off[0];
  if(estadoVER=='1')
    estado=B;
  else
    estado=A;
  
  mySerial.print("AT+CIPCLOSE");
  delay(5000);
  }

//Para el debug, función que permite ver la temperatura en C o F  
void printTemperature(
  char  *type,
  float  temperature) {

  Serial.print(type);
  Serial.print(" temperature: ");

  if (isnan(temperature)) {
    Serial.println("Failed");
  }
  else {
    Serial.print(temperature);
    Serial.println(SCALE == FAHRENHEIT  ?  " F"  :  " C");
  }
}
