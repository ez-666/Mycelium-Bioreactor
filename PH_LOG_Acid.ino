/**
  pH Regulator
  
  The circuit:
  - Peristaltic pump 
    https://www.dfrobot.com/product-1698.html
    https://wiki.dfrobot.com/Gravity__Digital_Peristaltic_Pump_SKU__DFR0523
    Power supply: 5-6v
    Interface: Black GND; Red 5V; Green: PPM digital (9 on my arduino uno)
    * Pump rate with tubing around 30ml/min
    * Docs say: 85ml/min
    0   -> clockwise maximum speed rotation
    90  -> stop
    180 -> counterclockwise maximum speed rotation
  - Analog pH sensor
    https://www.dfrobot.com/product-1110.html
    https://wiki.dfrobot.com/PH_meter_SKU__SEN0161_
    Power supply: 3.3-5v
    Interface: Black GND; Red 5V; Blue: analog 
    * Calibration:
    * 1) Put the pH sensor in 7.00 pH calibration liquid.
    * 2) Uncomment phCalibration() and upload the sketch.
    * 3) Update const phOffset with the result. 
    * 4) Put the pH sensor in 4.00 pH calibration liquid and let the value stabilize. 
*/

// setup peristaltic pump
#include <Servo.h>

Servo waterPump;

// digital ppm pin
const int WATER_PUMP_PIN = 9;
// analog input pin
const int PH_SENSOR_PIN = 0;
// dose solution for 2 seconds
const int doseDuration = 2000;
// wait 30 seconds before reading pH again
const int sensorReadInterval = 5000; //every 30 seconds

// Change to your desired pH
const float phTarget = 10;  //if measured pH below this, pump will start to pump acid until pHtarget reached
const float phToleration = 0.25;
// Calibrate your pH sensor in 7.0 solution. 
// The difference 7.0 and the pH read is your offset. 
// More instructions can be found in the wiki
const float phOffset = -7.90;
boolean isDosing = false;

// Serial data variables ------------------------------------------------------
//Incoming Serial Data Array
const byte kNumberOfChannelsFromExcel = 6; 

// Comma delimiter to separate consecutive data if using more than 1 sensor
const char kDelimiter = ',';    
// Interval between serial writes
const int kSerialInterval = 50;   
// Timestamp to track serial interval
unsigned long serialPreviousTime; 

char* arr[kNumberOfChannelsFromExcel];

void setup() {
  Serial.begin(9600);
  waterPump.attach(WATER_PUMP_PIN);
  //Serial.println("Starting");
}

void loop() {     
  mainLoop();
//  phCalibration();
}

void mainLoop() {
  float ph = getPh();
  Serial.println("pH: " + String(ph));
  // Can also do reverse check ph > phTarget + phToleration
  if (ph < phTarget) {
    if (ph < phTarget - phToleration) {
      // start to raise the pH
      //Serial.println("pH lower than target, beginning to raise acidity");
      isDosing = true;
    }
    if (isDosing) {
      dose(); 
    }
  } else {
    isDosing = false;
  }
  
  delay(sensorReadInterval);
}

/*void phCalibration() {
  float ph = getPh();
  Serial.println("pH: " + String(ph));
  Serial.println("Change pH offset to: " + String(7.00 - ph - phOffset));
  delay(sensorReadInterval);
}*/

void dose() {
  waterPumpOn();
  delay(doseDuration);
  waterPumpOff();
}

void waterPumpOn() {
  // change to 0 if you want water to flow otherway
  // doesn't do much until it reaches 150. 
  //Serial.println("Peristaltic pump on");
  waterPump.write(180);
}

void waterPumpOff() {
  //Serial.println("Peristaltic pump off");
  waterPump.write(90);
}

float getPh() {
  unsigned long int avgValue;  //Store the average value of the sensor feedback
  int buf[10],temp;
  
  for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(PH_SENSOR_PIN);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  float phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue=3.5*phValue+phOffset;                      //convert the millivolt into pH value
  return phValue;
}

//-----------------------------------------------------------------------------
// DO NOT EDIT ANYTHING BELOW THIS LINE
//-----------------------------------------------------------------------------

// OUTGOING SERIAL DATA PROCESSING CODE----------------------------------------
void processOutgoingSerial()
{
   // Enter into this only when serial interval has elapsed
  if((millis() - serialPreviousTime) > kSerialInterval) 
  {
    // Reset serial interval timestamp
    serialPreviousTime = millis(); 
    //sendDataToSerial(); 
  }
}

// INCOMING SERIAL DATA PROCESSING CODE----------------------------------------
void processIncomingSerial()
{
  if(Serial.available()){
    parseData(GetSerialData());
  }
}

// Gathers bytes from serial port to build inputString
char* GetSerialData()
{
  static char inputString[64]; // Create a char array to store incoming data
  memset(inputString, 0, sizeof(inputString)); // Clear the memory from a pervious reading
  while (Serial.available()){
    Serial.readBytesUntil('\n', inputString, 64); //Read every byte in Serial buffer until line end or 64 bytes
  }
  return inputString;
}

// Seperate the data at each delimeter
void parseData(char data[])
{
    char *token = strtok(data, ","); // Find the first delimeter and return the token before it
    int index = 0; // Index to track storage in the array
    while (token != NULL){ // Char* strings terminate w/ a Null character. We'll keep running the command until we hit it
      arr[index] = token; // Assign the token to an array
      token = strtok(NULL, ","); // Conintue to the next delimeter
      index++; // incremenet index to store next value
    }
}