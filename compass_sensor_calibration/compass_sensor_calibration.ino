#include <WiFi.h>
#include <Wire.h>

// ======= Wi-Fi AP Configuration =======
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// ======= Magnetometer (GY-271 / HMC5883L) =======
#define HMC5883L_ADDR 0x1E

float smoothX = 0, smoothY = 0;
float alpha = 0.1;  // smoothing factor

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize HMC5883L
  Wire.beginTransmission(HMC5883L_ADDR);
  Wire.write(0x00); Wire.write(0x70); Wire.endTransmission(); // Config A
  Wire.beginTransmission(HMC5883L_ADDR);
  Wire.write(0x01); Wire.write(0xA0); Wire.endTransmission(); // Config B
  Wire.beginTransmission(HMC5883L_ADDR);
  Wire.write(0x02); Wire.write(0x00); Wire.endTransmission(); // Continuous mode

  // Start Wi-Fi in AP mode
  WiFi.softAP(ssid, password);
  Serial.println("ESP32 AP started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET /data") >= 0) {
      float angle = readAngle();
      String dir = getDirection(angle);
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"angle\":");
      client.print(angle, 1);
      client.print(",\"dir\":\"");
      client.print(dir);
      client.println("\"}");
    } 
    else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();
      client.println(getHTML());
    }
    delay(1);
    client.stop();
  }
}

// ======= Read Magnetometer and return angle =======
float readAngle() {
  Wire.beginTransmission(HMC5883L_ADDR);
  Wire.write(0x03);
  Wire.endTransmission();
  Wire.requestFrom(HMC5883L_ADDR, 6);

  int16_t x = Wire.read()<<8 | Wire.read();
  int16_t z = Wire.read()<<8 | Wire.read();
  int16_t y = Wire.read()<<8 | Wire.read();

  smoothX = alpha * x + (1-alpha) * smoothX;
  smoothY = alpha * y + (1-alpha) * smoothY;

  // === FIX: Flip X axis to correct East/West ===
  float angle = atan2(smoothY, -smoothX) * 180 / PI;

  if (angle < 0) angle += 360;
  return angle;
}

// ======= Convert Angle to N/S/E/W =======
String getDirection(float angle) {
  if (angle >= 337.5 || angle < 22.5) return "North";
  else if (angle >= 22.5 && angle < 67.5) return "NE";
  else if (angle >= 67.5 && angle < 112.5) return "East";
  else if (angle >= 112.5 && angle < 157.5) return "SE";
  else if (angle >= 157.5 && angle < 202.5) return "South";
  else if (angle >= 202.5 && angle < 247.5) return "SW";
  else if (angle >= 247.5 && angle < 292.5) return "West";
  else return "NW";
}

// ======= HTML + JS =======
String getHTML() {
  float angle = readAngle();
  String dir = getDirection(angle);

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 Compass</title>";
  html += "<style>";
  html += "canvas{background:#f0f0f0;display:block;margin:0 auto;border-radius:50%;box-shadow:0 0 10px #aaa;}";
  html += "body{text-align:center;font-family:sans-serif;}";
  html += ".label{position:absolute;font-size:18px;font-weight:bold;}";
  html += "#north{top:20px;left:50%;transform:translateX(-50%);}";
  html += "#south{bottom:20px;left:50%;transform:translateX(-50%);}";
  html += "#west{top:50%;left:20px;transform:translateY(-50%);}";
  html += "#east{top:50%;right:20px;transform:translateY(-50%);}";
  html += "</style></head><body>";
  html += "<h2>ESP32 Compass</h2>";
  html += "<div style='position:relative;width:300px;height:300px;margin:auto;'>";
  html += "<canvas id='compass' width='300' height='300'></canvas>";
  html += "<div id='north' class='label'>N</div>";
  html += "<div id='south' class='label'>S</div>";
  html += "<div id='west' class='label'>W</div>";
  html += "<div id='east' class='label'>E</div>";
  html += "</div>";
  html += "<p>Angle: <span id='angle'>" + String(angle,1) + "</span>°</p>";
  html += "<h3>Direction: <span id='dir'>" + dir + "</span></h3>";
  html += "<script>";
  html += "let canvas=document.getElementById('compass');";
  html += "let ctx=canvas.getContext('2d');";
  html += "function drawCompass(a){ctx.clearRect(0,0,300,300);ctx.save();ctx.translate(150,150);ctx.rotate(a*Math.PI/180);ctx.beginPath();ctx.moveTo(0,-100);ctx.lineTo(-10,0);ctx.lineTo(10,0);ctx.closePath();ctx.fillStyle='red';ctx.fill();ctx.restore();}";
  html += "function update(){fetch('/data').then(r=>r.json()).then(d=>{drawCompass(d.angle);document.getElementById('angle').innerText=d.angle.toFixed(1);document.getElementById('dir').innerText=d.dir;});}";
  html += "setInterval(update,500);update();";
  html += "</script></body></html>";
  return html;
}
