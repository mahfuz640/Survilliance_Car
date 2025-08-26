#include <WiFi.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <Wire.h>
#include <math.h>

// ======= Wi-Fi AP =======
const char* ssid = "ESP32_AP";
const char* password = "12345678";
WiFiServer server(80);

// ======= Servos =======
#define SERVO1_PIN 14
#define SERVO2_PIN 12
#define SERVO3_PIN 13
Servo servo1, servo2, servo3;
int servo1Angle = 50; // limit 50-90
int servo2Angle = 90;
int servo3Angle = 0;

// ======= Metal Sensor =======
#define METAL_SENSOR 34

// ======= DHT11 =======
#define DHT_PIN 25
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ======= Magnetometer HMC5883L =======
#define HMC5883L_ADDR 0x1E
float mx, my, mz;
float cx, cy, cz;

// Calibration values
float minX = 1000000, minY = 1000000, minZ = 1000000;
float maxX = -1000000, maxY = -1000000, maxZ = -1000000;
float offsetX = 0, offsetY = 0, offsetZ = 0;
float scaleX = 1, scaleY = 1, scaleZ = 1;

unsigned long lastCompassMillis = 0;
float currentCompassAngle = 0;

// ======= L298N Motor Pins =======
#define IN1 26
#define IN2 27
#define IN3 32
#define IN4 33

void setup() {
  Serial.begin(115200);
  Wire.begin();
  dht.begin();

  // Attach servos
  servo1.attach(SERVO1_PIN); servo1.write(servo1Angle);
  servo2.attach(SERVO2_PIN); servo2.write(servo2Angle);
  servo3.attach(SERVO3_PIN); servo3.write(servo3Angle);

  // Metal sensor
  pinMode(METAL_SENSOR, INPUT);

  // HMC5883L init
  Wire.beginTransmission(HMC5883L_ADDR);
  Wire.write(0x00); Wire.write(0x70); Wire.endTransmission();
  Wire.beginTransmission(HMC5883L_ADDR);
  Wire.write(0x01); Wire.write(0xA0); Wire.endTransmission();
  Wire.beginTransmission(HMC5883L_ADDR);
  Wire.write(0x02); Wire.write(0x00); Wire.endTransmission();

  // Motor pins
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopCar();

  // Start Wi-Fi AP
  WiFi.softAP(ssid, password);
  Serial.println("ESP32 AP started");
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());
  server.begin();
}

void loop() {
  unsigned long now = millis();

  // Compass update every 20ms
  if (now - lastCompassMillis >= 20) {
    currentCompassAngle = readCompassAngle();  
    lastCompassMillis = now;
  }

  // Wi-Fi client handling
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r'); client.flush();
    Serial.println(request);

    // ======= Servo Control =======
    if (request.indexOf("/servo1?open") >= 0) { servo1Angle = min(90, servo1Angle+5); servo1.write(servo1Angle); }
    if (request.indexOf("/servo1?close") >= 0) { servo1Angle = max(50, servo1Angle-5); servo1.write(servo1Angle); }
    if (request.indexOf("/servo2?up") >= 0) { servo2Angle = min(180, servo2Angle+5); servo2.write(servo2Angle); }
    if (request.indexOf("/servo2?down") >= 0) { servo2Angle = max(0, servo2Angle-5); servo2.write(servo2Angle); }
    if (request.indexOf("/servo3short") >= 0) { servo3.write(180); delay(500); servo3.write(0); }

    // ======= Car Control =======
    if (request.indexOf("/car?forward") >= 0) moveForward();
    else if (request.indexOf("/car?back") >= 0) moveBackward();
    else if (request.indexOf("/car?left") >= 0) turnLeft();
    else if (request.indexOf("/car?right") >= 0) turnRight();
    else if (request.indexOf("/car?stop") >= 0) stopCar();

    // ======= JSON Data Endpoint =======
    if (request.indexOf("/data") >= 0) {
      bool metal = digitalRead(METAL_SENSOR);
      float temp = dht.readTemperature();
      float hum  = dht.readHumidity();
      String dir = headingToText(currentCompassAngle);

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"metal\":"); client.print(metal?"true":"false");
      client.print(",\"temp\":"); client.print(temp,1);
      client.print(",\"hum\":"); client.print(hum,1);
      client.print(",\"servo1\":"); client.print(servo1Angle);
      client.print(",\"servo2\":"); client.print(servo2Angle);
      client.print(",\"compassAngle\":"); client.print(currentCompassAngle,1);
      client.print(",\"compassDir\":\""); client.print(dir); client.print("\"}");
      client.stop();
      return;
    }

    // ======= Default HTML =======
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.println(getHTML());
    client.stop();
  }

  // Automatic servo3 trigger by metal
  if (digitalRead(METAL_SENSOR)) { servo3.write(180); delay(500); servo3.write(0); }
}

// ======= Compass Read with Calibration =======
float readCompassAngle() {
  Wire.beginTransmission(HMC5883L_ADDR);
  Wire.write(0x03); 
  Wire.endTransmission();

  Wire.requestFrom(HMC5883L_ADDR, 6);
  if (Wire.available() == 6) {
    int16_t x = (Wire.read() << 8) | Wire.read();
    int16_t z = (Wire.read() << 8) | Wire.read();
    int16_t y = (Wire.read() << 8) | Wire.read();

    mx = (float)x; my = (float)y; mz = (float)z;
  }

  // Update min/max
  if (mx < minX) minX = mx; if (mx > maxX) maxX = mx;
  if (my < minY) minY = my; if (my > maxY) maxY = my;
  if (mz < minZ) minZ = mz; if (mz > maxZ) maxZ = mz;

  // Calculate offsets
  offsetX = (maxX + minX) / 2.0;
  offsetY = (maxY + minY) / 2.0;
  offsetZ = (maxZ + minZ) / 2.0;

  // Calculate scales
  float avg_delta = ((maxX - minX) + (maxY - minY) + (maxZ - minZ)) / 3.0;
  scaleX = avg_delta / (maxX - minX);
  scaleY = avg_delta / (maxY - minY);
  scaleZ = avg_delta / (maxZ - minZ);

  // Apply calibration
  cx = (mx - offsetX) * scaleX;
  cy = (my - offsetY) * scaleY;
  cz = (mz - offsetZ) * scaleZ;

  // Heading angle
  float heading = atan2(cy, cx) * 180.0 / M_PI;
  if (heading < 0) heading += 360;
  return heading;
}

// Convert heading angle to compass direction
String headingToText(float heading) {
  if (heading >= 337.5 || heading < 22.5)   return "North";
  else if (heading >= 22.5 && heading < 67.5)   return "North-East";
  else if (heading >= 67.5 && heading < 112.5)  return "East";
  else if (heading >= 112.5 && heading < 157.5) return "South-East";
  else if (heading >= 157.5 && heading < 202.5) return "South";
  else if (heading >= 202.5 && heading < 247.5) return "South-West";
  else if (heading >= 247.5 && heading < 292.5) return "West";
  else if (heading >= 292.5 && heading < 337.5) return "North-West";
  return "Unknown";
}

// ======= Motor Functions =======
void moveForward() { digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW); digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW); }
void moveBackward(){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH); digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH); }
void turnLeft(){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH); digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW); }
void turnRight(){ digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW); digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH); }
void stopCar(){ digitalWrite(IN1,LOW); digitalWrite(IN2,LOW); digitalWrite(IN3,LOW); digitalWrite(IN4,LOW); }

// ======= HTML + JS =======
String getHTML() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 Control + Compass + Car</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body{background:#0d0d0d;color:#0ff;text-align:center;font-family:sans-serif;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;}";
  html += "button{padding:12px 20px;margin:5px;font-size:16px;border:none;border-radius:8px;color:#fff;cursor:pointer;";
  html += "background: linear-gradient(45deg, #ff6b6b, #f94d6a); transition:0.3s;}";
  html += "button:hover{background: linear-gradient(45deg, #6bc1ff, #4d94f9);}";
  html += "button:active{box-shadow:0 0 20px #fff inset;}";
  html += ".status{padding:10px;margin:5px;font-size:16px;border-radius:8px;display:inline-block;}";
  html += ".metal{background:#ff0080;color:#fff;} .nonmetal{background:#00ff80;color:#000;}";
  html += "</style></head><body>";

  html += "<h2>ESP32 Servo + Compass + Car Control</h2>";

  // Servo controls
  html += "<p>Servo1: <span id='servo1Val'>0</span></p>";
  html += "<button onclick=\"fetch('/servo1?open')\">Close</button>";
  html += "<button onclick=\"fetch('/servo1?close')\">Open</button><br>";
  html += "<p>Servo2: <span id='servo2Val'>0</span></p>";
  html += "<button onclick=\"fetch('/servo2?up')\">Up</button>";
  html += "<button onclick=\"fetch('/servo2?down')\">Down</button><br>";
  html += "<p><button onclick=\"fetch('/servo3short')\">Plate Short</button></p><hr>";

  // Metal & DHT
  html += "<div>Metal Status: <span id='metalStatus' class='status nonmetal'>Non-Metal</span></div>";
  html += "<div>Temp: <span id='temp'>--</span>°C, Hum: <span id='hum'>--</span>%</div>";

  // Compass
  html += "<div>";
  html += "<canvas id='compassCanvas' width='200' height='200' style='background:#111;border-radius:50%;display:block;margin:auto;'></canvas>";
  html += "<p>Angle: <span id='compassAngle'>--</span>°, Dir: <span id='compassDir'>--</span></p>";
  html += "</div>";

  // Car buttons
  html += "<h3>Car Control (Hold Button)</h3>";
  html += "<button onmousedown=\"fetch('/car?forward')\" onmouseup=\"fetch('/car?stop')\" ontouchstart=\"fetch('/car?forward')\" ontouchend=\"fetch('/car?stop')\">Forward</button><br>";
  html += "<button onmousedown=\"fetch('/car?back')\" onmouseup=\"fetch('/car?stop')\" ontouchstart=\"fetch('/car?back')\" ontouchend=\"fetch('/car?stop')\">Backward</button><br>";
  html += "<button onmousedown=\"fetch('/car?left')\" onmouseup=\"fetch('/car?stop')\" ontouchstart=\"fetch('/car?left')\" ontouchend=\"fetch('/car?stop')\">Right</button><br>";
  html += "<button onmousedown=\"fetch('/car?right')\" onmouseup=\"fetch('/car?stop')\" ontouchstart=\"fetch('/car?right')\" ontouchend=\"fetch('/car?stop')\">Left</button><br>";

  // JS auto-update (update every 0.1s)
  html += "<script>";
  html += "function update(){fetch('/data').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('servo1Val').innerText=d.servo1;";
  html += "document.getElementById('servo2Val').innerText=d.servo2;";
  html += "document.getElementById('metalStatus').innerText=d.metal?'Metal':'Non-Metal';";
  html += "document.getElementById('metalStatus').className='status '+(d.metal?'metal':'nonmetal');";
  html += "document.getElementById('temp').innerText=d.temp.toFixed(1);";
  html += "document.getElementById('hum').innerText=d.hum.toFixed(1);";
  html += "document.getElementById('compassAngle').innerText=d.compassAngle.toFixed(1);";
  html += "document.getElementById('compassDir').innerText=d.compassDir;";
  html += "drawCompass(d.compassAngle);";
  html += "});}";
  html += "let canvas=document.getElementById('compassCanvas');";
  html += "let ctx=canvas.getContext('2d');";
  html += "function drawCompass(angle){ctx.clearRect(0,0,200,200);ctx.save();ctx.translate(100,100);";
  html += "ctx.beginPath();ctx.arc(0,0,90,0,2*Math.PI);ctx.strokeStyle='#0ff';ctx.lineWidth=2;ctx.stroke();";
  html += "ctx.fillStyle='#0ff';ctx.font='16px sans-serif';ctx.textAlign='center';ctx.textBaseline='middle';";
  html += "ctx.fillText('N',0,-80);ctx.fillText('S',0,80);ctx.fillText('E',80,0);ctx.fillText('W',-80,0);";
  html += "ctx.rotate(angle*Math.PI/180);ctx.beginPath();ctx.moveTo(0,-70);ctx.lineTo(-7,0);ctx.lineTo(7,0);ctx.closePath();ctx.fillStyle='red';ctx.fill();ctx.restore();};";
  html += "setInterval(update,100);update();";
  html += "</script></body></html>";
  return html;
}
