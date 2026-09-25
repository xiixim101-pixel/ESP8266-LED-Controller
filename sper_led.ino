Projekt Led Wi-fi Marcin Słowik 20.09.2026


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid     = "TWOJA_NAZWA_WIFI";      // Zmień
const char* password = "TWOJE_HASLO_WIFI";          // Zmień


ESP8266WebServer server(80);

const int pwmPins[4] = {D5, D6, D7, D8};  // GPIO14, 12, 13, 15

int pwmValues[4] = {0, 0, 0, 0};
bool animActive[4] = {false, false, false, false};
unsigned long animStartTime[4] = {0, 0, 0, 0};
unsigned long baseAnimDuration[4] = {4000, 4000, 4000, 4000};
float speedMultiplier[4] = {1.0, 1.0, 1.0, 1.0};

int minAnimPercent[4] = {0, 0, 0, 0};
int maxAnimPercent[4] = {100, 100, 100, 100};

const char* colors[4] = {"#d32f2f", "#388e3c", "#1976d2", "#7b1fa2"};
const char* names[4] = {"Czerwony", "Zielony", "Niebieski", "Fioletowy"};

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Sterowanie PWM - NodeMCU</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background: #2e2e2e; color: #e0e0e0; margin: 0; padding: 20px; }
    h1 { color: #fff; margin-bottom: 20px; }

    /* Górne suwaki jasności */
    .main-sliders { 
      display: flex; 
      flex-wrap: wrap; 
      justify-content: center; 
      gap: 20px; 
      margin: 30px 0; 
      padding: 20px; 
      background: #383838; 
      border-radius: 15px; 
      box-shadow: 0 6px 15px rgba(0,0,0,0.5);
    }
    .slider-group { 
      flex: 1; 
      min-width: 200px; 
      max-width: 300px; 
    }
    .slider-group label { 
      display: block; 
      margin-bottom: 10px; 
      font-size: 18px; 
      font-weight: bold; 
    }
    .slider { 
      width: 100%; 
      height: 28px; 
      border-radius: 14px; 
      outline: none; 
    }

    /* 4 przyciski pod suwakami */
    .channel-buttons {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 15px;
      margin: 4px 0 5px;
    }
    .channel-btn {
      padding: 6px 5px;
      font-size: 10px;
      border: none;
      border-radius: 12px;
      cursor: pointer;
      color: white;
      min-width: 60px;
      box-shadow: 0 4px 10px rgba(0,0,0,0.4);
      transition: transform 0.2s;
    }
    .channel-btn:hover { transform: translateY(-3px); }
    .channel-btn:nth-child(1) { background: #d32f2f; }
    .channel-btn:nth-child(2) { background: #388e3c; }
    .channel-btn:nth-child(3) { background: #1976d2; }
    .channel-btn:nth-child(4) { background: #7b1fa2; }

    /* Panel opcji dla każdego kanału */
    .options-panel {
      max-height: 0;
      overflow: hidden;
      transition: max-height 0.5s ease;
      margin: 20px auto;
      padding: 0 20px;
      background: #424242;
      border-radius: 15px;
      max-width: 700px;
      box-shadow: 0 8px 20px rgba(0,0,0,0.6);
    }
    .options-panel.open { max-height: 600px; padding: 30px 20px; }

    .panel-title { font-size: 22px; margin-bottom: 2px; color: #fff; }

 .buttons { margin: 15px 0; }
    button.ctrl { padding: 1px 32px; font-size: 20px; margin: 22px; border: none; border-radius: 12px; cursor: pointer; }
    button.play { background: #aba4a8; color: black; }
    button.pause { background: #141314; color: white; }
    button.reset { background: #6a755d; color: yellow; }

    .range-sliders { display: flex; justify-content: center; gap: 40px; margin: 30px 0; }
    .range-group { text-align: center; }
    .range-group span { display: block; margin: 10px 0 5px; font-size: 16px; font-weight: bold; }
  </style>
</head>
<body>
  

  <!-- 4 suwaki jasności na górze -->
  <div class="main-sliders">
    <div class="slider-group">
      
      <input type="range" min="0" max="100" value="0" class="slider" id="s0" oninput="updateManual(0,this.value)">
    </div>
    <div class="slider-group">
      
      <input type="range" min="0" max="100" value="0" class="slider" id="s1" oninput="updateManual(1,this.value)">
    </div>
    <div class="slider-group">
      
      <input type="range" min="0" max="100" value="0" class="slider" id="s2" oninput="updateManual(2,this.value)">
    </div>
    <div class="slider-group">
      <label>GLOBUS</label>
      <input type="range" min="0" max="100" value="0" class="slider" id="s3" oninput="updateManual(3,this.value)">
    </div>
  </div>

  <!-- 4 przyciski otwierające opcje -->
  <div class="channel-buttons">
    <button class="channel-btn" onclick="openPanel(0)">Opcje Czerwony</button>
    <button class="channel-btn" onclick="openPanel(1)">Opcje Zielony</button>
    <button class="channel-btn" onclick="openPanel(2)">Opcje Niebieski</button>
    <button class="channel-btn" onclick="openPanel(3)">Opcje Globus</button>
  </div>

  <!-- Panele opcji (domyślnie zwinięte) -->
  <div class="options-panel" id="panel0">
   
    <div class="buttons">
      <button class="ctrl play" id="btn0" onclick="toggleAnim(0)">Play</button>
      <button class="ctrl reset" onclick="resetChannel(0)">Reset</button>
    </div>
    <div class="label">PWM CZERWONY: <span id="speed0">1.0</span>x</div>
    <input type="range" min="5" max="40" value="10" step="1" class="slider" id="sp0" oninput="updateSpeed(0,this.value)">
    <div class="label">Zakres animacji</div>
    <div class="range-sliders">
      <div class="range-group">
        Min: <span id="minVal0">0</span>%
        <input type="range" min="0" max="100" value="0" class="slider" id="min0" oninput="updateMin(0,this.value)">
      </div>
      <div class="range-group">
        Max: <span id="maxVal0">100</span>%
        <input type="range" min="0" max="100" value="100" class="slider" id="max0" oninput="updateMax(0,this.value)">
      </div>
    </div>
  </div>

  <div class="options-panel" id="panel1">
    
    <div class="buttons">
      <button class="ctrl play" id="btn1" onclick="toggleAnim(1)">Play</button>
      <button class="ctrl reset" onclick="resetChannel(1)">Reset</button>
    </div>
    <div class="label">PWM ZIELONY: <span id="speed1">1.0</span>x</div>
    <input type="range" min="5" max="40" value="10" step="1" class="slider" id="sp1" oninput="updateSpeed(1,this.value)">
    <div class="label">Zakres animacji</div>
    <div class="range-sliders">
      <div class="range-group">
        Min: <span id="minVal1">0</span>%
        <input type="range" min="0" max="100" value="0" class="slider" id="min1" oninput="updateMin(1,this.value)">
      </div>
      <div class="range-group">
        Max: <span id="maxVal1">100</span>%
        <input type="range" min="0" max="100" value="100" class="slider" id="max1" oninput="updateMax(1,this.value)">
      </div>
    </div>
  </div>

  <div class="options-panel" id="panel2">
    
    <div class="buttons">
      <button class="ctrl play" id="btn2" onclick="toggleAnim(2)">Play</button>
      <button class="ctrl reset" onclick="resetChannel(2)">Reset</button>
    </div>
    <div class="label">PWM NIEBIESKI: <span id="speed2">1.0</span>x</div>
    <input type="range" min="5" max="40" value="10" step="1" class="slider" id="sp2" oninput="updateSpeed(2,this.value)">
    <div class="label">Zakres animacji</div>
    <div class="range-sliders">
      <div class="range-group">
        Min: <span id="minVal2">0</span>%
        <input type="range" min="0" max="100" value="0" class="slider" id="min2" oninput="updateMin(2,this.value)">
      </div>
      <div class="range-group">
        Max: <span id="maxVal2">100</span>%
        <input type="range" min="0" max="100" value="100" class="slider" id="max2" oninput="updateMax(2,this.value)">
      </div>
    </div>
  </div>

  <div class="options-panel" id="panel3">
    
    <div class="buttons">
      <button class="ctrl play" id="btn3" onclick="toggleAnim(3)">Play</button>
      <button class="ctrl reset" onclick="resetChannel(3)">Reset</button>
    </div>
    <div class="label">PWM GLOBUS: <span id="speed3">1.0</span>x</div>
    <input type="range" min="5" max="40" value="10" step="1" class="slider" id="sp3" oninput="updateSpeed(3,this.value)">
    <div class="label">Zakres animacji</div>
    <div class="range-sliders">
      <div class="range-group">
        Min: <span id="minVal3">0</span>%
        <input type="range" min="0" max="100" value="0" class="slider" id="min3" oninput="updateMin(3,this.value)">
      </div>
      <div class="range-group">
        Max: <span id="maxVal3">100</span>%
        <input type="range" min="0" max="100" value="100" class="slider" id="max3" oninput="updateMax(3,this.value)">
      </div>
    </div>
  </div>

  <script>
    const colors = ["#d32f2f", "#388e3c", "#1976d2", "#7b1fa2"];

    // Kolorowanie suwaków
    for (let i = 0; i < 4; i++) {
      ["s"+i, "sp"+i, "min"+i, "max"+i].forEach(id => {
        let el = document.getElementById(id);
        if (el) el.style.accentColor = colors[i];
      });
    }

    function openPanel(pin) {
      // Zamknij wszystkie panele
      for (let i = 0; i < 4; i++) {
        document.getElementById("panel"+i).classList.remove("open");
      }
      // Otwórz wybrany
      document.getElementById("panel"+pin).classList.add("open");
    }

    function updateManual(pin, val) {
      fetch(`/set?pin=${pin}&value=${val}`);
    }

    function toggleAnim(pin) {
      let btn = document.getElementById("btn"+pin);
      fetch(`/anim?pin=${pin}`).then(r => r.text()).then(state => {
        if (state === "ON") {
          btn.innerHTML = "Pause";
          btn.classList.remove("play"); btn.classList.add("pause");
        } else {
          btn.innerHTML = "Play";
          btn.classList.remove("pause"); btn.classList.add("play");
          fetch("/status").then(r => r.json()).then(data => {
            let percent = Math.round((data.pwm[pin] / 255) * 100);
            document.getElementById("s"+pin).value = percent;
          });
        }
      });
    }

    function resetChannel(pin) {
      fetch(`/reset?pin=${pin}`);
      document.getElementById("s"+pin).value = 0;
      let btn = document.getElementById("btn"+pin);
      btn.innerHTML = "Play";
      btn.classList.remove("pause"); btn.classList.add("play");
    }

    function updateSpeed(pin, val) {
      let speed = (val / 10).toFixed(1);
      document.getElementById("speed"+pin).innerHTML = speed;
      fetch(`/speed?pin=${pin}&value=${speed}`);
    }

    function updateMin(pin, val) {
      document.getElementById("minVal"+pin).innerHTML = val;
      fetch(`/range?pin=${pin}&min=${val}&max=${document.getElementById("max"+pin).value}`);
    }

    function updateMax(pin, val) {
      document.getElementById("maxVal"+pin).innerHTML = val;
      fetch(`/range?pin=${pin}&min=${document.getElementById("min"+pin).value}&max=${val}`);
    }
  </script>
</body>
</html>
)rawliteral";



void handleRoot() { server.send_P(200, "text/html", index_html); }

void handleSet() {
  if (server.hasArg("pin") && server.hasArg("value")) {
    int pin = server.arg("pin").toInt();
    int val = server.arg("value").toInt();
    if (pin >= 0 && pin < 4 && val >= 0 && val <= 100) {
      pwmValues[pin] = map(val, 0, 100, 0, 255);
      analogWrite(pwmPins[pin], pwmValues[pin]);
      animActive[pin] = false;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleAnim() {
  if (server.hasArg("pin")) {
    int pin = server.arg("pin").toInt();
    if (pin >= 0 && pin < 4) {
      animActive[pin] = !animActive[pin];
      if (animActive[pin]) animStartTime[pin] = millis();
    }
  }
  server.send(200, "text/plain", animActive[server.arg("pin").toInt()] ? "ON" : "OFF");
}

void handleReset() {
  if (server.hasArg("pin")) {
    int pin = server.arg("pin").toInt();
    if (pin >= 0 && pin < 4) {
      pwmValues[pin] = 0;
      analogWrite(pwmPins[pin], 0);
      animActive[pin] = false;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("pin") && server.hasArg("value")) {
    int pin = server.arg("pin").toInt();
    float s = server.arg("value").toFloat();
    if (pin >= 0 && pin < 4) speedMultiplier[pin] = constrain(s, 0.5, 4.0);
  }
  server.send(200, "text/plain", "OK");
}

void handleRange() {
  if (server.hasArg("pin") && server.hasArg("min") && server.hasArg("max")) {
    int pin = server.arg("pin").toInt();
    int mn = server.arg("min").toInt();
    int mx = server.arg("max").toInt();
    if (pin >= 0 && pin < 4) {
      minAnimPercent[pin] = constrain(mn, 0, 100);
      maxAnimPercent[pin] = constrain(mx, 0, 100);
      if (minAnimPercent[pin] > maxAnimPercent[pin]) std::swap(minAnimPercent[pin], maxAnimPercent[pin]);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String json = "{\"pwm\":[";
  for (int i = 0; i < 4; i++) json += String(pwmValues[i]) + (i<3?",":"");
  json += "]}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  analogWriteFreq(1000);
  for (int i = 0; i < 4; i++) pinMode(pwmPins[i], OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nIP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/anim", handleAnim);
  server.on("/reset", handleReset);
  server.on("/speed", handleSpeed);
  server.on("/range", handleRange);
  server.on("/status", handleStatus);

  server.begin();
  Serial.println("Serwer uruchomiony");
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  for (int i = 0; i < 4; i++) {
    if (animActive[i]) {
      unsigned long dur = baseAnimDuration[i] / speedMultiplier[i];
      unsigned long elapsed = now - animStartTime[i];
      unsigned long cycle = elapsed % (dur * 2);

      int minV = map(minAnimPercent[i], 0, 100, 0, 255);
      int maxV = map(maxAnimPercent[i], 0, 100, 0, 255);
      int rangeV = maxV - minV;

      int value;
      if (cycle < dur) {
        value = minV + map(cycle, 0, dur, 0, rangeV);
      } else {
        value = maxV - map(cycle - dur, 0, dur, 0, rangeV);
      }

      analogWrite(pwmPins[i], value);
      pwmValues[i] = value;
    }
  }
}
