//Current - previous >= interval
//tempTimerStart = millis();
//while(millis() - tempTimerStart >= 2000){}
 
#define DC_PIN   4 
#define CS_PIN   5 
#define RST_PIN  19 

#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

#define magDeclination 14

#define azLeftOutput 33
#define azLeftInput 15
#define azRightOutput 32
#define azRightInput 2

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <TinyGPS++.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <JY901.h>

const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

unsigned long tempTimerStart;

int antennaAz = -100;int antennaOldAz = antennaAz;
int antennaEl = -100;int antennaOldEl = antennaEl;
int alt = -1000;
int intMonth = -1;int intDay = -1;int intYear = -1;
int intHour = -1;int intMin = -1;

float latitude = -100;
float longitude = -100;

String strGrid = "******";
String strOldGrid = "";

byte azSpeed = 0;

unsigned long lastTime = 0;  
unsigned long timerDelay = 1000;

AsyncWebServer server(80);
AsyncEventSource events("/events");

//const char *ssid = "AntennaRotator";
//const char *password = "86753009";

const char *ssid = "Skidmark";
const char *password = "thisisminesonotouchy";

TinyGPSPlus gps;
Adafruit_SSD1351 display = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>WiFi Rotator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p { font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #50B8B4; color: white; font-size: 1rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 380px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
    .reading { font-size: 1.4rem; }
    
    .button {
    display: inline-block;
    padding: 7px 25px;
    font-size: 20px;
    cursor: pointer;
    text-align: center;
    text-decoration: none;
    outline: none;
    color: #fff;
    background-color: #4CAF50;
    border: none;
    border-radius: 15px;
    box-shadow: 0 4px #999;}

    .button:hover {background-color: #3e8e41}
    
    .button:active {
    background-color: #4CAF10;}
}
    
  </style>
</head>
<body>
  <div class="topnav">
    <h1>WiFi Antenna Rotator V0.1</h1>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card">
        <p><i class="grid-display" style="color:#059e8a;"></i>Grid Square: <span class="reading"><span id="grid">%GRIDSQUARE%</span></span></p>
        <p><i class="alt-display" style="color:#059e8a;"></i>Altitude: <span class="reading"><span id="alt">%ALTITUDE%</span>'</span></p>
        <p><i class="lat-display" style="color:#059e8a;"></i>Latitude: <span class="reading"><span id="lat">%LATITUDE%</span>&deg;</span></p>
        <p><i class="lng-display" style="color:#059e8a;"></i>Longitude: <span class="reading"><span id="lng">%LONGITUDE%</span>&deg;</span></p>
      </div>
      <div class="card">
        <p><i class="azimuth-display" style="color:#059e8a;"></i>Azimuth: <span class="reading"><span id="az">%AZIMUTH%</span>&deg</span>
        <i class="elevation-display" style="color:#059e8a;"></i>Elevation: <span class="reading"><span id="el">%AZIMUTH%</span>&deg</span></p>
      </div>
      <div class="card">
        <p><button class="button" onmousedown="toggleCheckbox('left');" ontouchstart="toggleCheckbox('left');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');"><-Left</button>
           <button class="button" onmousedown="toggleCheckbox('right');"ontouchstart="toggleCheckbox('right');"onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');">Right -></button></p>
      </div>
  </div>
<script>
if (!!window.EventSource) 
{
 var source = new EventSource('/events');

 source.addEventListener('gridsquare', function(e) {document.getElementById("grid").innerHTML = e.data;}, false);
 source.addEventListener('altitude', function(e) {document.getElementById("alt").innerHTML = e.data;}, false);
 source.addEventListener('latitude', function(e) {document.getElementById("lat").innerHTML = e.data;}, false);
 source.addEventListener('longitude', function(e) {document.getElementById("lng").innerHTML = e.data;}, false);
 source.addEventListener('azimuth', function(e) {document.getElementById("az").innerHTML = e.data;}, false);
 source.addEventListener('elevation', function(e) {document.getElementById("el").innerHTML = e.data;}, false);
}

  function toggleCheckbox(x) 
  {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/" + x, true);
     xhr.send();
  }
</script>

</body>
</html>)rawliteral";


void setup()
{
  Serial.begin(9600); //Compass Port
  Serial2.begin(9600); //GPS Port

  display.begin();
  display.fillScreen(BLACK);

  display.setTextColor(WHITE);  
  display.setTextSize(2);
  
  pinMode(azLeftInput, INPUT);
  pinMode(azRightInput, INPUT);
  pinMode(azLeftOutput, OUTPUT);
  pinMode(azRightOutput, OUTPUT);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  display.setCursor(0,0);
  display.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) 
  {
  }
  /*
  WiFi.setHostname(hostname);

  // try to connect to existing network
  WiFi.begin(WiFissid, WiFipassword);
  Serial.print("\n\nTry to connect to existing network");
  display.setCursor(0,0);
  display.print("Connecting to:");
  display.setCursor(1,1);
  display.print(WiFissid);

  {
    byte timeout = 5;

    display.setCursor(0,2);
    // Wait for connection, 5s timeout
    do {
      delay(1000);
      Serial.print(".");
      display.print(".");
      timeout--;
    } while (timeout && WiFi.status() != WL_CONNECTED);

    delay(2000);

    // not connected -> create hotspot
    display.clear();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("\n\nCreating hotspot");
      display.setCursor(0,0);
      display.print("Creating AP:");
      display.setCursor(1,1);
      display.print(APssid);
      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP("Antenna Rotator");

      timeout = 5;
      display.setCursor(0,2);
      do {
        delay(1000);
        Serial.print(".");
        display.print(".");
        timeout--;
      } while (timeout);
    } 

  delay(2000);

  display.clear();
  display.setCursor(0,0);
  display.print("WiFi parameters:");
  Serial.println("\n\nWiFi parameters:");
  display.setCursor(0,1);
  display.print("Mode: ");
  Serial.print("Mode: ");
  display.print(WiFi.getMode() == WIFI_AP ? "AP" : "Client");
  Serial.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
  display.setCursor(0,2);
  display.print("IP address: ");
  Serial.print("IP address: ");
  display.setCursor(0,3);
  display.print(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
  Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());

  delay(8000);
  */

  // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/right", HTTP_GET, [] (AsyncWebServerRequest *request) 
  {
    digitalWrite(azLeftInput, HIGH);
    request->send(200, "text/plain", "ok");
  });

  server.on("/left", HTTP_GET, [] (AsyncWebServerRequest *request) 
  {
    digitalWrite(azRightInput, HIGH);
    request->send(200, "text/plain", "ok");
  });

  server.on("/off", HTTP_GET, [] (AsyncWebServerRequest *request) 
  {
    digitalWrite(azLeftInput, LOW);
    digitalWrite(azRightInput, LOW);
    request->send(200, "text/plain", "ok");
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){});
  server.addHandler(&events);

  server.begin();
  display.fillScreen(BLACK);
  
  display.setCursor(0,121);
  display.setTextSize(1);
  display.print("IP: ");
  display.println(WiFi.localIP());

  //updateDate();updateTime();updateGrid();updateAlt();updateAz();
    //updateGotoAz();
  //updateEl();
    //updateGotoEl();
  //initDisplay();
}
void loop()
{
  while(Serial2.available() > 0)
  if(gps.encode(Serial2.read()))
  {
    if(gps.location.isValid())
    {
      checkGPSInfo();
    }
  }

  while(Serial.available() > 0)
  {
    JY901.CopeSerialData(Serial.read());
  }

  antennaAz = ((float)JY901.stcAngle.Angle[2]/32768*180);
  antennaEl = ((float)JY901.stcAngle.Angle[0]/32768*180);

  antennaAz = map(antennaAz,0,359,359,0)+magDeclination+1;
  
  antennaAz = degCheck(antennaAz);
  
  if(antennaOldAz != antennaAz)
  {
    events.send(String(antennaAz).c_str(), "azimuth");
    antennaOldAz = antennaAz;
  }

  if(antennaOldEl != antennaEl)
  {
    events.send(String(antennaEl).c_str(), "elevation");
    antennaOldEl = antennaEl;
  }

  if(digitalRead(azLeftInput) == HIGH)
  {
    digitalWrite(azRightOutput, LOW);
    digitalWrite(azLeftOutput, HIGH);
  }
  else
  {
    digitalWrite(azRightOutput, LOW);
    digitalWrite(azLeftOutput, LOW);
  }

  if(digitalRead(azRightInput) == HIGH)
  {
    digitalWrite(azLeftOutput, LOW);
    digitalWrite(azRightOutput, HIGH);
  }
  else
  {
    digitalWrite(azRightOutput, LOW);
    digitalWrite(azLeftOutput, LOW);
  }
}
String calcGrid(long tmpLat, long tmpLon)
{
  String strTemp;
  tmpLon = tmpLon + 18000000;
  tmpLat = tmpLat +  9000000;
  char GS[6] = {'A', 'A', '0', '0', 'a', 'a'  };
  GS[0] +=  tmpLon / 2000000;
  GS[1] +=  tmpLat / 1000000;
  GS[2] += (tmpLon % 2000000) / 200000;GS[3] += (tmpLat % 1000000) / 100000;
  GS[4] += (tmpLon %  200000) /   8333;GS[5] += (tmpLat %  100000) /   4166;
  int indx = 0;
  while (indx < 6)
  {
    strTemp += (GS[indx]);
    indx++; 
  }
  return strTemp;
}
void updateGrid()
{
  display.setCursor(1,1);
  display.print(strGrid);
}
void updateAlt()
{
  display.setCursor(14,1);
  if(alt > 0 && alt < 10)
  {
    display.print("    ");
  }
  else if(alt >9 && alt < 100)
  {
    display.print("   ");
  }
  else if(alt > 99 && alt < 1000)
  {
    display.print("  ");
  }
  else if(alt > 999 && alt < 10000)
  {
    display.print(" ");
  }
  if (alt != -1000)
  {
    display.print(alt);
  }
  else
  {
    display.print("*****");
  }
}            
void updateTime()
{
  display.setCursor(2,0);
  if(intHour >= 0 && intHour < 10)
  {
    display.print("0");
  }
  if(intHour != -1)
  {
    display.print(intHour);
  }
  else
  {
    display.print("**");
  }
  display.setCursor(5,0);
  if(intMin > 0 && intMin < 10)
  {
    display.print("0");
  }
  if(intMin != -1)
  {
    display.print(intMin);
  }
  else
  {
    display.print("**");
  }
}
void updateDate()
{
  display.setCursor(10,0);
  if(intMonth > 0 && intMonth < 10)
  {
    display.print(" ");
  }
  if(intMonth != -1)
  {
    display.print(intMonth);
  }
  else
  {
    display.print("**");
  }
  display.setCursor(13,0);
  if(intDay > 0 && intDay < 10)
  {
    display.print("0");
  }
  if(intDay != -1)
  {
    display.print(intDay);
  }
  else
  {
    display.print("**");
  }
  display.setCursor(16,0);
  if(intYear > 0 && intYear < 10)
  {
    display.print("0");
  }
  if(intYear != -1)
  {
    display.print(intYear-2000);
  }
  else
  {
    display.print("**");
  }
}
void updateAz()
{
  display.setCursor(0,2);
  antennaAz = checkDegrees(antennaAz);
  if(antennaAz != -100)
  {
    display.print(antennaAz);
  }
}
void updateEl()
{
  display.setCursor(17,2);
  if(antennaEl >= 10)
  {
    display.print(" ");
  }
  else if(antennaEl >= 0 && antennaEl < 10)
  {
    display.print("  ");
  }
  else if(antennaEl > -10 && antennaEl < 0)
  {
    display.print(" ");
  }
  if(antennaEl != -100)
  {
    display.print(antennaEl);
  }
  else
  {
    display.print(" **");
  }
}
int checkDegrees(int intTemp)
{
  if (intTemp == -100)
  {
    display.print("***");
    return intTemp;
  }
  else if(intTemp < 0)
  {
    intTemp += 360;
  }
  else if(intTemp >= 360)
  {
    intTemp -= 360;
  }
  if(intTemp >= 0 && intTemp < 10)
  {
    display.print("00");
  }
  else if(intTemp > 9 && intTemp < 100) 
  {
    display.print("0");
  }
  return intTemp;
}
void checkGPSInfo()
{
  int intTemp;
  float fltTempLat;
  float fltTempLon;
  if(gps.date.isUpdated())
  {
    intTemp = gps.date.month();
    if(intTemp != intMonth)
    {
      intMonth = intTemp;
    }
    intTemp = gps.date.day();
    if(intTemp != intDay)
    {
      intDay = intTemp;
    }
    intTemp = gps.date.year();
    if(intTemp != intYear)
    {
      intYear = intTemp;
    }
  }
  if(gps.time.isUpdated())
  {
    intTemp = gps.time.hour();
    if(intTemp != intHour)
    {
      intHour = intTemp;
    }
    intTemp = gps.time.minute();
    if(intTemp != intMin)
    {
      intMin = intTemp;
    }
  } 
  if(gps.altitude.isUpdated())
  {
    intTemp = gps.altitude.feet();
    if(intTemp != alt)
    {
      alt = intTemp;
      events.send(String(alt).c_str(), "altitude");
    }
  }
  if(gps.location.isUpdated())
  {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
        
    strGrid = calcGrid((gps.location.lat()*100000), (gps.location.lng()*100000));
    if(strGrid!=strOldGrid)
    {
      strOldGrid = strGrid;
      events.send(String(strGrid).c_str(), "gridsquare");
    }

    events.send(String(latitude,4).c_str(),"latitude");
    events.send(String(longitude,4).c_str(),"longitude");
    
  }
  updateDate();
  //updateTime();
  //updateAlt();
  //updateGrid();
}
String processor(const String& var)
{
  if(var == "LATITUDE")
  {
    if(latitude == -100){return "**.****";}else{return String(latitude);}
    return String(latitude);
  }
  else if(var == "LONGITUDE")
  {
    if(longitude == -100){return "**.****";}else{return String(longitude);}
    return String(longitude);
  }
  else if(var == "GRIDSQUARE")
  {
    return strGrid;
  }
  else if(var == "ALTITUDE")
  {
    if(alt == -1000){return "*****";}else{return String(alt);}
  }
  else if(var == "AZIMUTH")
  {
    if(antennaAz == -100){return "***";}else{return String(antennaAz);}
  }
  else if(var == "ELEVATION")
  {
    if(antennaEl == -100){return "***";}else{return String(antennaEl);}
  }
}
int degCheck(int temp)
{
  if(temp < 0)
  {
    temp += 360;
  }
  else if(temp >= 360)
  {
    temp -= 360;
  }

  return temp;
}
void initDisplay()
{
  display.setCursor(4,0);
  display.print(":");
  display.setCursor(7,0);
  display.print("z");
  display.setCursor(12,0);
  display.print("/");
  display.setCursor(15,0);
  display.print("/");
  display.setCursor(9,1);
  display.print("Alt:");
  display.setCursor(4,2);
  display.print("<-Az");
  display.setCursor(13,2);
  display.print("El->");
}
