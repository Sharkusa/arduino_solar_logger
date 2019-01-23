// Program Summary
// This program reads Analog Inputs from Pins 0,1,2 and sends these inputs to a remote server via WIFI
// ESP8266 device is interfaced with Arduino to provide Wireless Connectivity to Arduino
// AT Commands are used to communicate with ESP8266 Device.
// HTTP POST Request is used to send data from Arduino to the remote Server.
// A unique and random AppKey is sent with every request from Arduino to Server.
// AppKey is encrypted using AESLib library to implement security. When the HTTP request is sent to the server, AppKey is sent in encrypted fashion.
// This encrypted AppKey is decrypted on the server and only if the request is valid, the Request is Honoured.
// AppKey is a 16 DIGIT number and contains the following three parts.
// AppKey  = BASE_ID + ARD_ID + reqBuff
// BASE_ID = Random 4 digit number
// ARD_ID = Arduino Logger ID, 2 digit number from 00 to 99. At present only 01,02,03,04,05 IDs are supported.
// reqBuff =  10 Digit Request ID based on a Random number. 

//Configuration Changes for new Logger Installation
// 1. ssid = "AIRTEL_E5172_4D8B"; Please use your Access Point ID
// 2. password = "80805B49C80"; Please use your Access Point password.
// 3. ip = "192.168.1.2"; Please use the I.P address of the server to which you are posting HTTP Request.
// 4. port = "80"; By default port = 80 unless your HTTP Server is running on other Port.
// 5. scriptpath = "/cgi-bin/ardLogger_crypt.py"; This is the Path of Python program that is receiving data from Arduino



#include <SoftwareSerial.h>
#include <stdlib.h>
#include <AESLib.h>

#define DEBUG true
 
SoftwareSerial esp8266(2,3); // connect TX line from esp to the Arduino's pin 2
                             // connect RX line from esp to the Arduino's pin 3
int solarV = 0;
int batteryV= 1;
int ampsV= 2;
float vout = 0.0;
float vin = 0.0;
float vin1=0.0;
float vout1=0.0;
float vin2=0.0;
float vout2=0.0;
float R1 = 4.7;   
float R2 = 1.005; 
float R6=4.690;
float R7= .998;
float R10=4.7;
float R11= 4.7;
float solarPanelVoltage = 0;
float batteryVoltage =0;
float chargeAmps =0;        
char buffer[14];

String ssid = "Alcatel";
String password = "8089416980894169";
//String ip = "144.202.75.241";
String ip = "192.168.88.234";
String ARD_ID = "01";
String port = "80";
String scriptpath = "/cgi-bin/ardLogger_crypt.py";
//String scriptpath = "/usr/lib/cgi-bin/ardLogger_crypt.py";
//Encryption Key
uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3};

String BASE_ID = "7812";
long request_id = 0;
char zero_counter = 0;
char ZERO_COUNT = 2;
char sendOKFlag = 0;

//Variables for reading sensor.
const int numReadings = 30;
//float readings[numReadings]; // the readings from the analog input
float reading = 0.0;
int index = 0; // the index of the current reading
float total = 0; // the running total
float average = 0; // the average
float adcData = 0.0;

void setup()
{
 
  Serial.begin(9600);
  esp8266.begin(9600); // your esp's baud rate might be different.
  
  //Create Random Seed.
  randomSeed(analogRead(A5));   
  
  // declaration of pin modes
  analogReference(DEFAULT);
  pinMode(solarV, INPUT);
  pinMode(batteryV, INPUT);
  pinMode(ampsV, INPUT);  
  
  sendData("AT+RST\r\n",2000,DEBUG); // reset module OK
  sendData("AT+CWMODE=1\r\n",500,DEBUG); // 
  sendData("AT+CWJAP=\""+ssid+"\",\""+password+"\"\r\n",1000,DEBUG); // Connect to Access point
  delay(5000);
  
}
 
float readSensor(float vcc, char dataPin) {
 total = 0;
 for(int x=0; x< numReadings; x++ ) {
   adcData = analogRead(dataPin); //Raw data reading   
   reading = (adcData-510)*vcc/1024/0.04-0.04;      // For 50A sensor
//Data processing:510-raw data from analogRead when the input is 0; 5-5v; the first 0.04-0.04V/A(sensitivity); the second 0.04-offset val;   
//   reading = (adcData-510)*vcc/1024/0.02-0.02;         // For 100A sensor
   total= total + reading;    
   delay(10);
 }
 Serial.println("ADC Data");
 Serial.println(adcData);
 average = total/numReadings; //Smoothing algorithm (http://www.arduino.cc/en/Tutorial/Smoothing)  
 return average;
} 
 
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}
 
 
void loop()
{
    String cmdResponse="";  
    int HEXAPPKEY_LENGTH = 32;
    int CRLF_LENGTH = 4;
    char reqBuff[10];    
    sprintf(reqBuff, "%010ld", request_id);
    String data = BASE_ID + ARD_ID + reqBuff;   
    
    float vcc = readVcc()/1000.0;    
   
    // Encryption of AppKey.
    char cdata[data.length()+1];
    data.toCharArray(cdata,data.length()+1);
    aes128_enc_single(key, cdata);
  
    solarPanelVoltage = analogRead(solarV);
    delay(10);
    solarPanelVoltage = analogRead(solarV);
    delay(10);
//    vout = (solarPanelVoltage * 5.0) / 1024.0;
    vout = (solarPanelVoltage * vcc) / 1024.0;    
    vin = vout / (R2/(R1+R2));

    batteryVoltage = analogRead(batteryV);
    delay(10);
    batteryVoltage = analogRead(batteryV);
    delay(10);
//    vout1=(batteryVoltage * 5.0) / 1024.0;
    vout1=(batteryVoltage * vcc) / 1024.0;    
    vin1= vout1 / (R7/(R6+R7));
  
/*    chargeAmps = analogRead(ampsV);
    delay(10);
    chargeAmps = analogRead(ampsV);
    delay(10);
    vout2=(chargeAmps * 5.0) / 1024.0;
    vin2= vout2 / (R11/(R10+R11));*/
  
    Serial.println("VCC");    
    Serial.println(vcc,4);    
    vin2 = readSensor(vcc, ampsV);     
    //debug code
    //vin2 = 25.123
    Serial.println("Sensor Output");
    Serial.println(vin2,4);    
  
    String strvin = dtostrf(vin, 7, 2, buffer);
    String strvin1 = dtostrf(vin1, 7, 2, buffer);
    String strvin2 = dtostrf(vin2, 7, 2, buffer);
    strvin.trim();
    strvin1.trim();
    strvin2.trim();   
   
    //post request
    sendOKFlag = 0;
    String params = "v="+strvin+"&b="+strvin1+"&a="+strvin2+ "&AK=";
    String command = "POST "+scriptpath+ " HTTP/1.1\r\nHost: "+ip+"\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: "+String(params.length()+HEXAPPKEY_LENGTH)+"\r\n\r\n";          
    cmdResponse = sendData("AT+CIPSTART=\"TCP\",\""+ip+"\","+port+"\r\n",1000,DEBUG); // Connect to Website     
    if(cmdResponse.indexOf("Linked")<=0){
         sendData("AT+RST\r\n",2000,DEBUG); // reset module OK
         sendData("AT+CWJAP=\""+ssid+"\",\""+password+"\"\r\n",1000,DEBUG); // Connect to Access point
         delay(5000);
         sendData("AT+CIPSTART=\"TCP\",\""+ip+"\","+port+"\r\n",1000,DEBUG); // Connect to Website    
         sendData("AT+CIPSEND="+String(command.length()+params.length()+HEXAPPKEY_LENGTH+CRLF_LENGTH)+"\r\n",10,DEBUG); // Length of the Data to be sent.   
         cmdResponse = sendDataParams(command,params,cdata,500,DEBUG);         
      }else{
         sendData("AT+CIPSEND="+String(command.length()+params.length()+HEXAPPKEY_LENGTH+CRLF_LENGTH)+"\r\n",10,DEBUG); // Length of the Data to be sent.                  
         cmdResponse = sendDataParams(command,params,cdata,500,DEBUG);      
     }     
     
   if(zero_counter < ZERO_COUNT) {
     zero_counter++;
   }else if( zero_counter == ZERO_COUNT) {
     zero_counter++;
     request_id = random(100000, 999999);
   }else {
     if(sendOKFlag == 1){ // Increment the request id only if previous request id is sent successfully.
       request_id++;
     }
   }
   Serial.println("REQUESTID");
   Serial.println(request_id);
   sendData("AT+CIPCLOSE\r\n",10,DEBUG); // Close connection  

   delay(5000);      
}





// Function Sends AT Commands to ESP8266 Device.
// command: contains AT Command to be sentt to ESP8266
// timeout : time in milliseconds to wait for the response from esp8266
// debug: If debug messages are expected in Serial Monitor, set this to 1.

String sendData(String command, const int timeout, boolean debug) {
  
    String response = "";
    esp8266.print(command); // send the read character to the esp8266
    long int time = millis();
    while( (time+timeout) > millis()) {
      while(esp8266.available()) {        
        // The esp has data so display its output to the serial window 
        char c = esp8266.read(); // read the next character.
        response+=c;
      }  
    }    
    if(debug) {
      Serial.print(response);      
    }    
    return response;
}

// Function Sends HTTP POST Request to the Server.
// If the POST Request is sent successfully, sendOKFlag is set to 1.
// command: contains AT Command to be sent to the server as part of HTTP POST Request.
// params: contains voltage, battery voltage and amp values to be sent to the server.
// timeout : time in milliseconds to wait for the response from esp8266
// debug: If debug messages are expected in Serial Monitor, set this to 1.


String sendDataParams(String command,String params , char appkey[], const int timeout, boolean debug) {

    String response = "";
    String hexdata = "";
    String tmp = "";
    
    esp8266.print(command); // send the read character to the esp8266
    esp8266.print(params); // send the read character to the esp8266
    
    
    // appkey data is HEX encoded in order send it without issues via Software Serial.
    // Hex encoded data is stored in hexdata.
    for(int i=0; i<16; i++) {
      Serial.print((byte)appkey[i],HEX);
      Serial.print(":");      
      tmp = String((byte)appkey[i],HEX);
      if (tmp.length() == 1) tmp = "0" + tmp;
      hexdata = hexdata + tmp;
    }
    Serial.println("hexdata");      
    Serial.println(hexdata);          
    esp8266.print(hexdata);    
    esp8266.print("\r\n\r\n");
   
    long int time = millis();
   
    while( (time+timeout) > millis()) {
      while(esp8266.available()) {  
        // The esp has data so display its output to the serial window
        char c = esp8266.read(); // read the next character.
        response+=c;
      }
    }   
    if(response.indexOf("SEND OK")>0){ // Increment the request id only if previous request id is sent successfully.
      sendOKFlag = 1;
     }
    if(debug) {
      Serial.print(response);
    }
    return response;
}

