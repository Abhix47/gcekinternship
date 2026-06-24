#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Ritwin's S24";
const char* password = "123456788";

const int LED_PIN = 2;

// PWM Settings
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8; // 0-255

WebServer server(80);

void handleRoot()
{
    String page = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>ESP32 Smart LED</title>

<style>

*{
margin:0;
padding:0;
box-sizing:border-box;
font-family:Arial,sans-serif;
}

body{

background:#111827;

display:flex;
justify-content:center;
align-items:center;

height:100vh;

color:white;

}

.container{

background:#1f2937;

padding:30px;

border-radius:20px;

width:90%;
max-width:420px;

text-align:center;

box-shadow:0 0 25px rgba(0,0,0,0.5);

}

h1{

margin-bottom:15px;

color:#60a5fa;

}

.status{

width:20px;
height:20px;

border-radius:50%;

background:red;

margin:15px auto;

box-shadow:0 0 10px red;

transition:0.3s;

}

.brightness{

margin-top:25px;

}

input[type=range]{

width:100%;

margin-top:15px;

}

.value{

margin-top:10px;

font-size:22px;

font-weight:bold;

color:#34d399;

}

.buttons{

margin-top:25px;

display:flex;

justify-content:center;

gap:15px;

}

button{

padding:15px 25px;

border:none;

border-radius:12px;

font-size:18px;

cursor:pointer;

color:white;

transition:0.3s;

}

.on{

background:#10b981;

}

.off{

background:#ef4444;

}

button:hover{

transform:scale(1.05);

}

</style>

</head>

<body>

<div class="container">

<h1>ESP32 LED Control</h1>

<div id="status" class="status"></div>

<div class="brightness">

<h3>Brightness</h3>

<input
type="range"
min="0"
max="255"
value="0"
id="slider"
oninput="updateBrightness(this.value)">

<div class="value" id="value">

0

</div>

</div>

<div class="buttons">

<button
class="on"
onclick="turnOn()">

ON

</button>

<button
class="off"
onclick="turnOff()">

OFF

</button>

</div>

</div>

<script>

function updateBrightness(val){

document.getElementById("value").innerHTML = val;

fetch("/brightness?value=" + val);

if(val > 0){

document.getElementById("status").style.background="#10b981";
document.getElementById("status").style.boxShadow=
"0 0 15px #10b981";

}
else{

document.getElementById("status").style.background="#ef4444";
document.getElementById("status").style.boxShadow=
"0 0 15px #ef4444";

}

}

function turnOn(){

fetch("/on");

document.getElementById("slider").value = 255;
document.getElementById("value").innerHTML = 255;

document.getElementById("status").style.background="#10b981";
document.getElementById("status").style.boxShadow=
"0 0 15px #10b981";

}

function turnOff(){

fetch("/off");

document.getElementById("slider").value = 0;
document.getElementById("value").innerHTML = 0;

document.getElementById("status").style.background="#ef4444";
document.getElementById("status").style.boxShadow=
"0 0 15px #ef4444";

}

</script>

</body>
</html>

)rawliteral";

    server.send(200, "text/html", page);
}

void handleOn()
{
    ledcWrite(PWM_CHANNEL, 255);
    Serial.println("LED ON");

    server.send(200, "text/plain", "ON");
}

void handleOff()
{
    ledcWrite(PWM_CHANNEL, 0);
    Serial.println("LED OFF");

    server.send(200, "text/plain", "OFF");
}

void handleBrightness()
{
    if(server.hasArg("value"))
    {
        int brightness = server.arg("value").toInt();

        brightness = constrain(brightness, 0, 255);

        ledcWrite(PWM_CHANNEL, brightness);

        Serial.print("Brightness: ");
        Serial.println(brightness);
    }

    server.send(200, "text/plain", "OK");
}

void setup()
{
    Serial.begin(115200);

    ledcSetup(
        PWM_CHANNEL,
        PWM_FREQ,
        PWM_RESOLUTION
    );

    ledcAttachPin(
        LED_PIN,
        PWM_CHANNEL
    );

    ledcWrite(PWM_CHANNEL, 0);

    Serial.println();
    Serial.println("Connecting to WiFi...");

    WiFi.begin(ssid, password);

    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Connected!");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/on", handleOn);
    server.on("/off", handleOff);
    server.on("/brightness", handleBrightness);

    server.begin();

    Serial.println("Web Server Started");
}

void loop()
{
    server.handleClient();
}