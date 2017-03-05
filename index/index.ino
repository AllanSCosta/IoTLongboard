#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <SFE_LSM9DS0.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266.h>
Adafruit_SSD1306 display(4);
#define IOT 1
#define wifiSerial Serial1          // for ESP chip
#define LSM9DS1_M  0x1E //
#define LSM9DS1_AG  0x6B //
String MAC = "";
//#define SSID "6S08B"       // network SSID and password
//#define PASSWORD "6S086S08"
#define SSID "MIT"       // network SSID and password
#define PASSWORD ""
//#define SSID "EECS-MTL-RLE"       // network SSID and password
//#define PASSWORD ""
#include <Adafruit_NeoPixel.h>
#define PIN 23
#define number_of_LED 30
ESP8266 wifi = ESP8266(true);  //Change to "true" or nothing for verbose serial output
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(54, PIN);
uint8_t  mode   = 1, // Current animation effect
         offset = 0; // Position of spinny eyes
#define serialYes false             // print to serial port
#define CALIBRATE true
#define CALIBRATION_MILLIS 10000  //Calibrate for 10s if CALIBRATE==true

#include <Adafruit_GPS.h>
#define GPSSerial Serial2
Adafruit_GPS GPS(&GPSSerial);

LSM9DS0 imu(MODE_I2C, LSM9DS1_M, LSM9DS1_AG);

//TO SKIP CALIBRATION, DEFINE OFF_X and OFF_Y and set CALIBRATE to false
#define OFF_X 0
#define OFF_Y 0
#define OFF_Z 0 //You can leave this one as 0, we don't calibrate this anyway


String targetPlace = "";
int red = 0,
    green = 220,
    blue = 180,
    path_index = 0; // where you are in the path
//float path[1],
float angle,
      delta_angle,
      center_LED;

// for navigateTo
float gpsCoords[2];
double path[200][2]; // ultimately will be reallocated to be path[numVertices][2]

// for main loop
int timeBetweenLastCalls = 30000;
int lastCallTime = 0;

int timeBetweenOrientations = 100;
int lastOrientationTime = 0;

int lastStatusTime = 0;
int timeBetweenStatusUpdates = 30000;

String lastCall;

float ax, ay, az, magAccel=0;         // acceleration components
float magVelocity=0;                  
int timeOfMagAccel=0;
int timeOfLastPrint = 0;



class GPSParser
{
  public:
    String hrs, min, sec;
    String valid;
    String lat, NS, lon, EW;
    String day, month, year;


    GPSParser() {
      hrs = "";
      min = "";
      sec = "";
      valid = "";   //Should contain "A" or "V"
      lat = "";
      NS = "";
      lon = "";
      EW = "";
      day = "";
      month = "";
      year = "";
    }

    //Finds commas in "s" and fills in pre-initialized array commaLoc with
    //the indices of those commas, up to a total of "maxCommas" commas.
    //If there are fewer that "maxCommas" commas, this function pads the
    //remainder of commaLoc (up to index "maxCommas") with the value -1
    void findCommas(String s, int commaLoc[], int maxCommas) {
      commaLoc[0] = s.indexOf(",");
      for (int i = 1; i < maxCommas; i++) {
        commaLoc[i] = s.indexOf(",", commaLoc[i - 1] + 1);
      }
    }

    // take the string d
    // and return the x and y coordinates
    void extractData(String d)
    {

      int maxCommas = 12;

      int commaLoc[maxCommas];

      findCommas(d, commaLoc, maxCommas);

      String hhmmss = d.substring(commaLoc[0] + 1, commaLoc[1]);
      hrs = hhmmss.substring(0, 2);
      min = hhmmss.substring(2, 4);
      sec = hhmmss.substring(4, 6);

      valid = d.substring(commaLoc[1] + 1, commaLoc[2]);

      lat = d.substring(commaLoc[2] + 1, commaLoc[3]);
      NS = d.substring(commaLoc[3] + 1, commaLoc[4]);
      lon = d.substring(commaLoc[4] + 1, commaLoc[5]);
      EW = d.substring(commaLoc[5] + 1, commaLoc[6]);

      String ddmmyy = d.substring(commaLoc[8] + 1, commaLoc[9]);
      day = ddmmyy.substring(0, 2);
      month = ddmmyy.substring(2, 4);
      year = ddmmyy.substring(4, 6);

      // update global gps coords
      gpsCoords[0] = lat.toFloat();
      gpsCoords[1] = lon.toFloat();


    }
};


class Compass {
  public:
    int offx, offy, offz;
    float heading;
    float const declination = -14.75; //For Cambridge, MA area
    Compass() {
      offx = 1596; //You can change use this as default and forego calibration, if you want
      offy = -732; //You can change use this as default and forego calibration, if you want
      offz = 0; //You can change use this as default and forego calibration, if you want
      heading = 0;
    }

    //Use this if you want to programatically set the offsets, and forego calibration
    void setOffsets(int x, int y, int z) {
      offx = x;
      offy = y;
      offz = z;
    }

    void calibrate(uint32_t calibrationTime) {
      //Initial values for min_x, max_x, etc, will all get overwritten for sure
      int min_x = 32767;
      int min_y = 32767;
      int min_z = 32767;
      int max_x = -32768;
      int max_y = -32768;
      int max_z = -32768;
      uint32_t startTime = millis();
      int lastSecondsLeft = -1; //initial value doesn't mean anything
      while (millis() - startTime < calibrationTime) {
        imu.readMag();
        min_x = min(min_x, imu.mx);
        max_x = max(max_x, imu.mx);
        min_y = min(min_y, imu.my);
        max_y = max(max_y, imu.my);
        min_z = min(min_z, imu.mz);
        max_z = max(max_z, imu.mz);

        //update display only once per second
        int secondsLeft = (calibrationTime - (millis() - startTime)) / 1000;
        if (secondsLeft != lastSecondsLeft) {
          lastSecondsLeft = secondsLeft;
        }
      }
      offx = (min_x + max_x) / 2;
      offy = (min_y + max_y) / 2;
      offz = 0; //With flat turning, we can't tell "hard" interference with earth magnetism
      Serial.println("Magnetic offsets found");
      Serial.print("Offx: ");
      Serial.println(offx);
      Serial.print("Offy: ");
      Serial.println(offy);
      Serial.print("Offz: ");
      Serial.println(offz);
    }
    void update() {
      // Get magnetometer reading
      imu.readMag();
      int mx = -1 * (imu.mx - offx); // Aligning with diagram on the board
      int my = imu.my - offy;
      int mz = imu.mz - offz;

      // Get accelerometer reading
      imu.readAccel();
      int ax = -1 * imu.ax; // Aligning with diagram on the board
      int ay = -1 * imu.ay; // Aligning with diagram on the board
      int az = -1 * imu.az; // Aligning with diagram on the board

      // Calculate component of magnetism that is perpendicular to gravity
      float gravMagnitude = 1.0 * (mx * ax + my * ay + mz * az) / (ax * ax + ay * ay + az * az);
      float northx = mx - gravMagnitude * ax;
      float northy = my - gravMagnitude * ay;
      //float northz = mz - gravMagnitude * az;

      //note: northAngle is "math-y", so it's 0 on +x-axis and increases counterclockwise
      float northAngle = -1 * atan2(northy, northx) * 180 / PI; //Heading is based on X,Y components of North
      northAngle += declination; //Account for declination
      heading = -northAngle;  // Find our heading, given northAngle
      heading += 90; //Change axes: Now 0 degrees is straight ahead (+y-axis)
      heading = int(heading + 360) % 360 + (heading - int(heading)); //Hack-y mod operation for floats
    }


};

GPSParser gps_data;
Compass compass;

void setup() {







  Serial.begin(9600);
  delay(1000);
  Serial.print("IOTLONG-ON");


  uint16_t status = imu.begin();
  
  // Set up Neopixel strip
  pixels.begin();
  pixels.setBrightness(50); // 1/3 brightness
  delay(2500);

  Serial.println("Pixels Started");
  
  if (!imu.begin())
  {
    while (1) {
      Serial.println("Comm Failure with LSM9DS1");
      delay(1000);
    }
  }
  Serial.println("IMU STARTED");
  


  // Set up GPS
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  GPS.begin(9600);
  GPSSerial.begin(9600);
  // RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Set the update rate to 1 Hz
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);


  Serial.println("GPS STARTED");

  // Calibrate
  Serial.print("IoT-Long...");
  if (CALIBRATE) { //Perform calibration routine
    Serial.print("Calibration");

    // turn on lights
    for (int i = 0; i < 30; i++) {
      pixels.setPixelColor(i, 255, 0, 0);
    }
    pixels.show();

    // calibrate
    compass.calibrate(CALIBRATION_MILLIS); //Calibrate for a set # of milliseconds


    // turn off lights
    for (int i = 0; i < 30; i++) {
      pixels.setPixelColor(i, 0, 0, 0);
    }
    pixels.show();

  } else { //Use hard-coded defaults instead
    compass.setOffsets(OFF_X, OFF_Y, OFF_Z);
  }



  // Set up Wifi
  if (IOT) {
    wifi.begin();
    wifi.connectWifi(SSID, PASSWORD);
    while (!wifi.isConnected()); //wait for connection
    MAC = wifi.getMAC();
  }
  Serial.println("WIFI STARTED");
  randomSeed(analogRead(0));//seed random number
  pinMode(PIN, OUTPUT);
}

void loop() {

  delay(20);

  // to view user interface, go to
  // http://iesc-s2.mit.edu/student_code/anshula/dev1/sb2.py?username=iotlong&password=damnanshula

  int curTime = millis();
  
  // continue updating orientation if we're navigating somewhere
  if ((targetPlace != "") && (curTime - lastOrientationTime > timeBetweenOrientations)) {
    lastOrientationTime = curTime;
    updateOrientation();
    //    Serial.println("finished updating orientation");
  }

  // check for new updates from the website
  if (curTime - lastCallTime > timeBetweenLastCalls) {
    lastCallTime = curTime;
    lastCall = checkForLastCall(); executeLastCall(lastCall);
    //Serial.println("finished executing last call");
  }

  // update distance and velocity
  if (curTime - lastStatusTime > timeBetweenStatusUpdates) {
    lastStatusTime = curTime;
    send_status_update();
  }


}

String checkForLastCall() {
  if (wifi.isConnected() && !wifi.isBusy()) { //Check if we can send request
    Serial.print("Sending request at t=");
    Serial.println(millis());

    String domain = "iesc-s2.mit.edu";
    int port = 80;
    String folder = "/student_code/anshula/dev1/sb3.py";

    String getParams = "recipient=iotlong";
    wifi.sendRequest(GET, domain, port, folder, getParams);

    //wait for response
    unsigned long t = millis();
    while (!wifi.hasResponse() && millis() - t < 10000);

    // print out response
    if (wifi.hasResponse()) {
      String resp = wifi.getResponse(); //prints to screen too
      String lastCall = resp.substring(resp.indexOf("<p>>> ") + 6, resp.indexOf("</p></html>"));

      Serial.println("\nGot last call:");
      Serial.print(lastCall);

      return lastCall;
    } else {
      Serial.println("No timely response");
      return "";
    }


  }
  else {
    return "";
  }
}

void executeLastCall(String lastCall) {
  if (lastCall == "light(ON)") {
    Serial.println("\n\n************\nTurning On Lights\n*************");
    turnOnLights(); // turn on all lights on LED strip
  } else if (lastCall == "light(OFF)") {
    Serial.println("\n\n************\nTurning Off Lights\n*************");
    turnOffLights(); // turn off all lights on LED strip
  } else if (lastCall.indexOf("color(") >= 0) {
    changeLightColor(lastCall); // change color of LED strip
  } else if (lastCall.indexOf("navigateTo(") == 0) {
    Serial.println("\n\n************\nNavigating\n*************"); // navigate to destination
    navigateTo(lastCall);
  } 
//  else if (lastCall.indexOf("distance()") == 0) {
//    Serial.println("\n\n************\nTotal Distance Traveled Is:\n*************\n\n"); // print out total distance traveled
//  } else if (lastCall.indexOf("velocity()") == 0) {
//    Serial.println("\n\n************\nCurrent Velocity Is:\n*************\n\n"); // print out current instantaneous velocity
//  } else if (lastCall == "long(OFF)") {
//    // put teensy in deep sleep
//  } else if (lastCall == "long(ON)") {
//    // wake teensy
//  }
}

void turnOnLights() {
  for (int i = 0; i < 30; i++) {
    pixels.setPixelColor(i, red, green, blue);
  }
  pixels.show();
}

void turnOffLights() {
  for (int i = 0; i < 30; i++) {
    pixels.setPixelColor(i, 0, 0, 0);
  }
  pixels.show();
}

void changeLightColor(String lastCall) {
  red = (lastCall.substring(lastCall.indexOf("color(") + 6, lastCall.indexOf(","))).toInt();
  green =  (lastCall.substring(lastCall.indexOf(",") + 1,  lastCall.indexOf(',', lastCall.indexOf(',') + 1))).toInt();
  blue = (lastCall.substring(lastCall.indexOf(',', lastCall.indexOf(',') + 1) + 1,  lastCall.indexOf(')'))).toInt();
  for (int i = 0; i < 30; i++) {
    pixels.setPixelColor(i, red, green, blue);
  }
  pixels.show();
}




// ---------------------------- UPDATING GPS DATA ---------------------------------------
void getCurrentGPSCoords(float gpsCoords[]) {
  // MIT Media Lab
  gpsCoords[0] = 42.360353;
  gpsCoords[1] = -71.087591;
}


float start_lat=0;
float start_lon=0;
float distance = 0;
void printGPS()
{
  if (gps_data.valid == "A") {

    Serial.println("Got new GPS data!  Here are the coordinates:");
    Serial.print(gpsCoords[0]); Serial.print(", "); Serial.println(gpsCoords[1]);
    
    if (start_lat==0 && start_lon==0) {
      //convert degrees and minutes to flat decimals
      start_lat=gps_data.lat.substring(0, 2).toFloat()+1/60*gps_data.lat.substring(2).toFloat();
      start_lon=gps_data.lon.substring(0, 3).toFloat()+1/60*gps_data.lon.substring(3).toFloat();
    }
    else {
      // calculate distance using Haversine formula
      // http://stackoverflow.com/questions/27928/calculate-distance-between-two-latitude-longitude-points-haversine-formula

      float lat2=gps_data.lat.substring(0, 2).toFloat()+1/60*gps_data.lat.substring(2).toFloat();
      float lon2=gps_data.lon.substring(0, 3).toFloat()+1/60*gps_data.lon.substring(3).toFloat();

      float lat1=start_lat;
      float lon1=start_lon;
      
      int R = 6371; // Radius of the earth in km
      float dLat = (lat2-lat1)* (PI/180);  // convert to radians below
      float dLon = (lon2-lon1)* (PI/180); 
      float a = sin(dLat/2) * sin(dLat/2) + cos(lat1* (PI/180)) * cos(lat2* (PI/180)) * sin(dLon/2) * sin(dLon/2) ;
      float c = 2 * atan2(sqrt(a), sqrt(1-a)); 
      distance = R * c; // Distance in km
      Serial.print("Distance is");Serial.print(distance);Serial.println("km");
    }
  }
  else if (gps_data.valid == "V") {
    Serial.println("No valid fix");
  }
}


int doneWithFirstLoop=0;
String data = "";
void updateGPS() {

  // read data from the GPS in the 'main loop'
  char c = GPS.read();

  if (c)  {

    // if there's a new $, then we're done with the last line
    if (c == '$') {

      if (doneWithFirstLoop == 0) {
        Serial.println("Just got first line");
        doneWithFirstLoop = 1;
      }
      
      else {

        if (data.indexOf("GPRMC") != -1) {

          Serial.println("Extracting data");
          gps_data.extractData(data);

          printGPS(); //prints new gps coords and updates distance
        }

        // clear out the data string
        // so it doesn't take up too much memory
        Serial.println("\n\nGot this GPS data:");
        Serial.print(data);
        Serial.print("\n\n");
        data = "";
      }
    }

    // otherwise, just add the char to the data string
    Serial.print(c);
    data = data + c;

  }
}

// ---------------------------- UPDATING ACCELEROMETER DATA ---------------------------------------

void updateAccel()
{
    // To read from the accelerometer, you must first call the
  // readAccel() function. When this exits, it'll update the
  // imu.ax, imu.ay, and imu.az variables with the most current data.

  imu.readAccel();
  
  // calculate acceleration in g's and store in ax, ay, and az
  ax = (float) imu.ax * 2 / 32767;
  ay = (float) imu.ay * 2 / 32767;
  az = (float) imu.az * 2 / 32767;
  
  // calculate difference in accelerations
  float magAccel2 = sqrt(ax * ax + ay * ay)*9.81; // in meters/second^2
  float magAccel1 = magAccel;
  float da = magAccel2-magAccel1;
  
  //calculate difference in time
  int time2 = millis();
  int time1 = timeOfMagAccel;
  int dt = time2-time1;
  
  // multiply to get mag velocity
  magVelocity = da*dt;

  // we can't have good data the first time
  if (time1==0) {
    magVelocity=0;
  }
  
  
  // print to serial port (aka USB port)
  

  // update last time and last magaccel
  magAccel = magAccel2;
  timeOfMagAccel = time2;

}
void printAccel()
{
  if ((millis() - timeOfLastPrint) > 2000) {
    Serial.print("Acceleration in x,y Direction:");
    Serial.println(magAccel, 4);
    
    Serial.print("Velocity in x,y Direction:");
    Serial.println(magVelocity, 4);
    
    timeOfLastPrint=millis();
  }
  
}


// ---------------------------- UPDATING VELOCITY AND DISTANCE ---------------------------------------

void send_status_update() {


  Serial.println("Sending status update of velocity and distance to website");
      
  String msg = "";
  msg+="Traveled  " + String(distance) + " km. ";
  msg+="Velocity is " + String(magVelocity) + " m/s. ";

  String domain = "iesc-s2.mit.edu";
  int port = 80;
  // http://iesc-s2.mit.edu/student_code/anshula/lab05/L05B.py
  String path = "/student_code/anshula/lab05/L05B.py";
  
  String postParams = "&platform=teensy&recipient=person1&message=" + msg;

  // true means auto-retry after certain amount of time
  wifi.sendRequest(POST, domain, port, path, postParams, false); 

  // results of post request can be seen here
  // http://iesc-s2.mit.edu/student_code/anshula/lab05/L05B.py?recipient=person1&platform=browser

  // or from the main page here
  // http://iesc-s2.mit.edu/student_code/anshula/dev1/sb2.py?username=iotlong&password=damnanshula
  
}



// ---------------------------- NAVIGATION FUNCTIONS ---------------------------------------

String getPath(String destination, float gpsCoords[]) {
  String originLat = String(gpsCoords[0]);
  String originLon = String(gpsCoords[1]);

  // wait for wifi to stop being busy
  unsigned long t = millis();
  while (wifi.isBusy() && millis() - t < 10000);

  //  if (wifi.isConnected() && !wifi.isBusy()) {
  //send out request
  // http://iesc-s2.mit.edu/student_code/anshula/dev1/sb1.py
  String domain = "iesc-s2.mit.edu";
  int port = 80;
  String path = "/student_code/anshula/dev1/sb1.py";

  String getParams = "origin_lat=" + originLat +
                     "&origin_lon=" + originLon +
                     "&destination=place_id:" + destination;
  Serial.println("Get params are these");
  Serial.print(getParams);

  wifi.sendRequest(GET, domain, port, path, getParams, false);

  while (!wifi.hasResponse());

  Serial.println("Getting response");
  String resp = wifi.getResponse();
  Serial.println("Done getting response");

  return resp;

  //wait for response
  //    unsigned long t = millis();
  //    Serial.println("Waiting for response");


  // print out response
  //    if (wifi.hasResponse()) {
  //      Serial.println("\n\n GETTING RESPONSE FROM WIFI \n\n");
  //      String resp = wifi.getResponse(); //prints to screen too
  //      Serial.println("\n\n DONE GETTING RESPONSE FROM WIFI \n\n");
  //      return resp;
  //    } else {
  //      Serial.println("\n\n WIFI DOESN'T HAVE RESPONSE \n\n");
  //      return "";
  //    }
  //  }
  //  else {
  //    Serial.println("Wifi was busy or not connected.");
  //    return "";
  //  }
}
void getPathArrayFromString(String pathstring, double path[][2], int numVertices) {
  String verticesstring = pathstring.substring(pathstring.indexOf("<vertices>[") + 11, pathstring.indexOf("]</vertices>"));
  verticesstring = verticesstring.replace("(", "");
  verticesstring = verticesstring.replace("), ", "\n");
  verticesstring = verticesstring.replace(")", "");

  Serial.println("Vertices string is:"); Serial.println(verticesstring);

  // splitting by newlines
  int i;

  int newline_a = 0;
  int newline_b = verticesstring.indexOf("\n");

  double lat;
  double lon;

  for (i = 0; i < numVertices; i++) {

    // get the lat, lon string
    String vertex = verticesstring.substring(newline_a, newline_b).replace("\n", "");

    // load them into character arrays into string types
    char latchar[100];
    char lonchar[100];
    vertex.substring(0, vertex.indexOf(',')).toCharArray(latchar, 100);
    vertex.substring(vertex.indexOf(',') + 1).toCharArray(lonchar, 100);

    // convert to doubles
    lat = atof(latchar);
    lon = atof(lonchar);

    // load into path array
    path[i][0] = lat;
    path[i][1] = lon;

    // increment the indices between which the next vertex is
    newline_a = newline_b;
    newline_b = verticesstring.indexOf("\n", newline_b + 2);
  }

  //print the path
  Serial.print("Printing path from array\n");
  for (i = 0; i < numVertices; i++) {
    Serial.print(path[i][0], 7); Serial.print(", "); Serial.println(path[i][1], 7);
  }

}

void checkPath(double next_vertex[], float gpsCoords[]) {
  // Increases Index of path to do in case it's within range
  double dist_next_vertex = sqrt(pow((next_vertex[1] - gpsCoords[1]) * 82850.73, 2) + pow((next_vertex[0] - gpsCoords[0]) * 111073.25, 2));
  if (dist_next_vertex <= 50) {
    path_index++;
  }
}


void orientLight(float orientation, float gpsCoords[], double next_vertex[]) {
  // Gets the angle between the current coordinate and the next vertex
  angle = atan(next_vertex[1] - gpsCoords[1]) / (next_vertex[0] - gpsCoords[0]) * 180 / PI;
  if (next_vertex[0] < gpsCoords[0] && next_vertex[1] < gpsCoords[1]) {
    angle += 90;
  } else if (next_vertex[0] < gpsCoords[0] && next_vertex[1] > gpsCoords[1]) {
    angle *= -1;
    angle += 180;
  } else if (next_vertex[0] > gpsCoords[0] && next_vertex[1] < gpsCoords[1]) {
    angle *= -1;
  } else if (next_vertex[0] > gpsCoords[0] && next_vertex[1] > gpsCoords[1]) {
    angle += 270;
  }
  // Referencing north, conmpares compass angle and angle to path to do
//  Serial.println(angle);
  if (orientation - angle > 180) {
    delta_angle = (angle + 300 - orientation);
  } else {
    delta_angle =  300 - (orientation - angle);
  }
//  Serial.print("DELTA_ANGLE:");
//  Serial.println(delta_angle);
  // Based on the difference of angles, lights LED's according to direction
  center_LED = (delta_angle * number_of_LED / 360);
  for (int i = 0; i < 30; i++) {
    pixels.setPixelColor(i, 0, 0, 0);
  }
  for (int i = 0; i < 7; i++) {
    if (center_LED + i <= number_of_LED - 1) {
      pixels.setPixelColor(center_LED + i, (red / pow(i + 1, 2)), (green / pow(i + 1, 2)), (blue / pow(i + 1, 2)));
    }
    if (center_LED - i >= 0) {
      pixels.setPixelColor(center_LED - i, (red / pow(i + 1, 2)), (green / pow(i + 1, 2)), (blue / pow(i + 1, 2)));
    }
  }
  pixels.setPixelColor(center_LED, red, green, blue);
  pixels.show();
}



void navigateTo(String lastCall) {

  String newPlace = lastCall.substring(lastCall.indexOf("navigateTo(") + 11, lastCall.indexOf(")"));

  // Requests Google Maps API to orient to a given place

  if (targetPlace != newPlace) {

    targetPlace = newPlace;
    Serial.println(targetPlace);

    // get current GPS coordinates
    getCurrentGPSCoords(gpsCoords);
    Serial.print("Current GPS Coords: "); Serial.print(gpsCoords[0]); Serial.print(" "); Serial.println(gpsCoords[1]);

    // send place ID and current GPS coordinates to maps.py
    // get back directions
    String pathstring = getPath(targetPlace, gpsCoords);

    const int numVertices = pathstring.substring(pathstring.indexOf("<num_vertices>") + 14, pathstring.indexOf("</num_vertices>")).toInt();

    // load path into array
    Serial.print("Creating array with num vertices: "); Serial.println(numVertices);
    //    // double path[numVertices][2]; memset(path, 0, numVertices * 2 * sizeof(double));
    memset(path, 0, numVertices * 2 * sizeof(double)); // allocate space for the array
    getPathArrayFromString(pathstring, path, numVertices);

    // function that uses path to light up appropriate skateboard panel
    // will be called in the next run through the main loop

  }

  else {
    Serial.println("Same location as before");
  }

}

void updateOrientation() {

  // update orientation angle
  compass.update();
  int orientation_angle = 360 - compass.heading;
//  Serial.print("ORIENTATION: "); Serial.println(orientation_angle);

  updateGPS(); // get next char in gps coordinates, update distance measurements
  updateAccel(); printAccel();// get new instantaneous velocity

  checkPath(path[path_index], gpsCoords);
  orientLight(orientation_angle, gpsCoords, path[path_index]);

}




