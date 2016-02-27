#include "ESP8266.h"
#include <SoftwareSerial.h>
#include "IRTemp.h"

// Pines de conexión para el sensor de temperatura
static const byte PIN_DATA    = 4; // Choose any pins you like for these
static const byte PIN_CLOCK   = 5;
static const byte PIN_ACQUIRE = 6;

static const TempUnit SCALE=CELSIUS;  // Options are CELSIUS, FAHRENHEIT

IRTemp irTemp(PIN_ACQUIRE, PIN_CLOCK, PIN_DATA);

#define SSID  "moncayo CNT"     //  "CTI_DOMO"  //    "Nexxt_0714E8"  
#define PASSWORD "0912671898"  //   "ct1esp0l15"//   // "S3c0D3P0ll0"
#define HOST_NAME   "api.thingspeak.com"
#define HOST_PORT   80

//El modulo ESP8266 usa SoftwareSerial...
SoftwareSerial mySerial(3,2); /* RX:D2, TX:D3 */
ESP8266 wifi(mySerial);

#define RELAY_PIN 8 // Pin de comunicacion entre el Relay y arduino
 String estadOn_Off="";
int estadoProximidad=0;
float irTemperature=0;

//Definicion de la estructura para la maquina de estado
enum estados {
  A,B,C,D,E
};
estados estado=A;

void setup()
{
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(9600); // open serial
    //Configura el módulo ESP8266
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
  int sensorValue = analogRead(A5);
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
          
        }else{
            estadoProximidad=0;
          } 
                 
        if(estadoProximidad==1)
          estado=C;
         else{
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
