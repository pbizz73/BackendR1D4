#include "StopWatch.h"
// Include the necessary libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

// GLOBAL VARIABLES
int SensorPins[] = {31, 33, 35, 37};  // The Sonar Ranging Module accepts sensor nbr in binary format only. LSB = 31 and MSB = 37. Put them in an array for easy access in code
const int Init = 47, Echo = 49, Binh = 51;  // Link pins 30, 44 & 46 to "Initialize", "Echo", and "BINH" signals respectively
StopWatch SW(StopWatch::MICROS); // StopWatch to measure time (in us)
volatile float TimeToEcho = 0; // This variable will be used to pass time values between ISR and code (in us)

// GLOBAL CONSTANTS FOR MOTOR CONTROL
const int motorPin1 = 2;
const int motorPin2 = 22;
const int motorPin3 = 4;
const int motorPin4 = 12;
const int motorPin5 = 14;
const int motorPin6 = 16; // Changed from 7
const int motorPin7 = 17; // Changed from 8
const int motorPin8 = 18; // Changed from 9
String state = "";

// WiFi Configuration
const char* ssid = "Paolosiphone";
const char* password = "12345678";


// SERVER SETUP
WiFiServer server(23); // telnet defaults to port 23

// COMMUNICATION MODE FLAG
bool useTelnet = true; // Set this to false if you want to use Serial instead

void setup() {
  Serial.begin(115200);

  // Initialize pins
  for (int i = 0; i < 4; i++) { pinMode(SensorPins[i], OUTPUT); }
  pinMode(Init, OUTPUT);
  pinMode(Echo, INPUT);
  pinMode(Binh, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(Echo), EchoDetected, RISING);

  // Motor control pins initialization
  analogWrite(motorPin1, OUTPUT);
  analogWrite(motorPin2, OUTPUT);
  analogWrite(motorPin3, OUTPUT);
  analogWrite(motorPin4, OUTPUT);
  analogWrite(motorPin5, OUTPUT);
  analogWrite(motorPin6, OUTPUT);
  analogWrite(motorPin7, OUTPUT);
  analogWrite(motorPin8, OUTPUT);

  if (useTelnet) {
    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting...");
    }

    Serial.println("Connected to WiFi.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();
    Serial.println("Telnet server started.");
  } else {
    Serial.println("Serial communication started. Please enter command here.");
  }
}

// MAIN LOOP
void loop() {
  //Serial.println(state);
  //delay(500);
  if (useTelnet) {
    // Handle Telnet Client
    WiFiClient client = server.available();
    if (client) {
      Serial.println("Client connected.");
      client.println("Hello, client! Please enter a command.");

      while (client.connected()) {
        Serial.println(state);
        delay(500);
        if (client.available()) {
          String ClientCommand = GetCommandFromClient(client);
          if(ClientCommand.equals("end")){
            client.stop();
          }
          ProcessCommand(ClientCommand, client);
        }else{
          if(Serial.available() > 0) {
            String serialLine = Serial.readStringUntil('\n');
            while (Serial.available() > 0) {
                Serial.read(); // Clear buffer before reading fresh data
            }
            if(serialLine.equals("schedule")){
              //Serial.println("Schedule Received!!");
              client.print("Schedule Received!!");
              delay(5000);
            }
        }
      }
      }
      client.stop();
      Serial.println("Client disconnected.");
    
  }
  } else {
    // Handle Serial Communication
    if (Serial.available() > 0) {
      Serial.println("Serial data detected!");
      String SerialCommand = Serial.readStringUntil('\n');
      SerialCommand.trim();
      if (SerialCommand.length() > 0) {
        Serial.print("Command received: ");
        Serial.println(SerialCommand);
        Serial.println("Processing command...");
        ProcessCommand(SerialCommand, Serial);
      } else {
        Serial.println("Received empty command.");
      }

      if (Serial.available() > 0) {
        Serial.println("Warning: Data still in buffer!");
      } else {
        Serial.println("Buffer empty as expected.");
      }
      Serial.println("Waiting for the next command...");
    } else {
      Serial.println("No serial data available.");
      delay(1000); // Avoid spamming the output
    }
  }
}

// FUNCTION TO SELECT COMMAND PROCESSING
void ProcessCommand(String command, Print &output) {
    if (command.indexOf("RFS") != -1) {
        //output.println(ReadFromSensors(command));
        state = "Read From Sensors";
    } else if (command.indexOf("SM") != -1) {
        //Serial.println("Stop Motor");
        state = "Stop Motors";
        stopMotor();
    } else {
        String argument = command.substring(command.indexOf(" ") + 1);
        argument.replace(" ", "");
        if (command.indexOf("MF") != -1) {
            moveForward(argument.toInt());
            // Serial.print("Moving Forward ");
            // Serial.print(argument.toInt());
            // Serial.println(" Meters");
            state = "Moving Forward ";
            state.concat(argument.toInt());
            state.concat(" Meters");
        } else if (command.indexOf("MB") != -1) {
            moveBackward(argument.toInt());
            // Serial.print("Moving Backward ");
            // Serial.print(argument.toInt());
            // Serial.println(" Meters");
            state = "Moving Backward ";
            state.concat(argument.toInt());
            state.concat(" Meters");
        } else if (command.indexOf("SCO") != -1) {
            // Serial.print("Spin Counter Clockwise ");
            // spinCounter(argument.toInt());
            // Serial.print(argument.toInt());
            // Serial.println(" Degrees");
            spinCounter(argument.toInt());
            state = "Spin Counter Clockwise";
            state.concat(argument.toInt());
            state.concat(" Degrees");
        } else if (command.indexOf("SC") != -1) {
          // Serial.print("Spin Clockwise ");
          //   spinClock(argument.toInt());
          //   Serial.print(argument.toInt());
          //   Serial.println(" Degrees");
            spinClock(argument.toInt());
            state = "Spin Clockwise";
            state.concat(argument.toInt());
            state.concat(" Degrees");
        }
    }
   // Serial.println("\rDone! Enter new command.");
}


// MAIN FUNCTIONS
String ReadFromSensors(String ClientCommand){
  // Decrypt Client Command and get the number of sensors to be echoed
  ClientCommand = ClientCommand.substring(ClientCommand.indexOf("<") + 1);
  ClientCommand = ClientCommand.substring(0, ClientCommand.indexOf(">"));
  ClientCommand.replace(" ", "");
  int SeperatorAtIndex = ClientCommand.indexOf(",");
  int NbrOfSeparators = 0;
  while (SeperatorAtIndex != -1) {
    NbrOfSeparators = NbrOfSeparators + 1;
    SeperatorAtIndex = ClientCommand.indexOf(",", SeperatorAtIndex + 1);
  }

  // Read sensor numbers and then read distance from each one of them and send back to client
  int SensorNbrs[NbrOfSeparators + 1];
  float SensorReadings[NbrOfSeparators + 1];
  String Output = "";
  if (NbrOfSeparators == 0){
    SensorNbrs[0] = ClientCommand.toInt();
    SensorReadings[0] = GetDistance(SensorNbrs[0]);
    Output = (String)SensorReadings[0];
  } else {
    for (int i = 0; i < NbrOfSeparators + 1; i++){
      SensorNbrs[i] = ClientCommand.substring(0, ClientCommand.indexOf(",")).toInt();
      SensorReadings[i] = GetDistance(SensorNbrs[i]);
      Output = Output + (String)SensorReadings[i] + " ";
      ClientCommand = ClientCommand.substring(ClientCommand.indexOf(",") + 1);
    }
  }
  return Output;
}

// HELPER FUNCTIONS
float GetDistance(int SensorNbr){
  // Decode the sensor nbr from decimal (0 to 15) to binary, pass it to corresponding pins on the board, then read distance from that sensor and return it (in meters)
  for (int i = 0; i < 4; i++){
    if (((SensorNbr/(int)pow(2, i)) % 2) == 1){digitalWrite(SensorPins[i], HIGH);} else {digitalWrite(SensorPins[i], LOW);}
  }

  // Declare needed variables
  const float BinhTime = 0.65; // BINH rise time (in ms). This signal reduces the blanking period
  const float SpeedOfSound = 340.29; // Speed of sound (in m/s)
  const float MaxDistance = 5; // Max wanted distance to be echoed by sensor (in meters). The max recommended value is 10m (for accuracy reasons)
  const float MaxHearingTime = 2*MaxDistance/SpeedOfSound; // This is the maximum "hearing" time allowed (in seconds)
  float Distance = 0;

  // Calculate distance between sensor and obstacle (in meters) and return the value
  digitalWrite(Init, LOW); digitalWrite(Binh, LOW);
  delay(1); // Reset the interface
  SW.reset(); TimeToEcho = 0; digitalWrite(Init, HIGH); SW.start();// Bring Init signal to high and start the timer
  do {} while (SW.value() < BinhTime*1000); digitalWrite(Binh, HIGH); // Bring BINH sinal to high after BinhTime to reduce Blanking time -> read shorter distances
  do {} while (SW.value() < MaxHearingTime*1000000); // Wait for rising edge on Echo signal
  Distance = SpeedOfSound*TimeToEcho/2000000;
  return Distance;
}

void EchoDetected(){
  // ISR for RISING edge on Echo signal, this detects when the sound wave gets back to the sensor and saves corresponding stopwatch value
  TimeToEcho = SW.value();
}

String GetCommandFromClient(WiFiClient client){
  // As long as the client hasn't pressed the 'Enter' key, the 'ClientCommand' string will keep building
  String ClientCommand = "";
  char IncomingChar = (char)client.read();
  while (IncomingChar != '\n'){
    if (IncomingChar != (char)-1) {ClientCommand = ClientCommand + (String)IncomingChar;}
    IncomingChar = (char)client.read();
  }
  ClientCommand.trim();
  return ClientCommand;
}

// MOTOR CONTROL FUNCTIONS
void stopMotor(){
  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 0);
  analogWrite(motorPin4, 0);
  analogWrite(motorPin5, 0);
  analogWrite(motorPin6, 0);
  analogWrite(motorPin7, 0);
  analogWrite(motorPin8, 0);
}

void moveForward(int distance){
  // we know from testing that robot speed is 36 cm/s
  //Serial.println("Moving Forward Function");
  analogWrite(motorPin1, 100);
  analogWrite(motorPin2, 100);
  analogWrite(motorPin3, 100);
  analogWrite(motorPin4, 0);
  analogWrite(motorPin5, 100);
  analogWrite(motorPin6, 100);
  analogWrite(motorPin7, 100);
  analogWrite(motorPin8, 100);
  if (distance != 0) {
    int delayTime; delayTime = 1000 * distance / 36;
    delay(delayTime);
    stopMotor();
  }
}

void moveBackward(int distance){
  // we know from testing that robot speed is 36 cm/s
  analogWrite(motorPin1, 100);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 100);
  analogWrite(motorPin4, 100);
  analogWrite(motorPin5, 100);
  analogWrite(motorPin6, 100);
  analogWrite(motorPin7, 100);
  analogWrite(motorPin8, 100);
  if (distance != 0) {
    int delayTime; delayTime = 1 * distance / 36;
    delay(delayTime); stopMotor();
  }
}

void spinClock(int angle){
  // we know from testing that a 90deg spin takes about 700ms
  int delayTime;
  delayTime = angle * 700 / 90;
  analogWrite(motorPin1, 100);
  analogWrite(motorPin2, 100);
  analogWrite(motorPin3, 100);
  analogWrite(motorPin4, 100);
  analogWrite(motorPin5, 100);
  analogWrite(motorPin6, 100);
  analogWrite(motorPin7, 100);
  analogWrite(motorPin8, 100);
  delay(delayTime); stopMotor();
}

void spinCounter(int angle){
  // we know from testing that a 90deg spin takes about 700ms
  int delayTime;
  delayTime = angle * 700 / 90;
  analogWrite(motorPin1, 100);
  analogWrite(motorPin2, 0);
  analogWrite(motorPin3, 100);
  analogWrite(motorPin4, 0);
  analogWrite(motorPin5, 0);
  analogWrite(motorPin6, 0);
  analogWrite(motorPin7, 0);
  analogWrite(motorPin8, 0);
  delay(delayTime); stopMotor();
}