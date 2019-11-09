// TTGO LoRa32-OLED V1

#include <ESPmDNS.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

static constexpr uint8_t OFF = 0;
static constexpr uint8_t ON = 1;
static constexpr uint8_t LEDpin = LED_BUILTIN;

static constexpr const char* HOST = "esp32";
// Rainer (STP)
// const char* SSID = "HierKoennteIhreWerbungStehen_o";
// const char* SSID = "HierKoennteIhreWerbungStehen_d";
// const char* PASSWORD = "...";
// WZ ASUS Z3
// const char* SSID = "AsusZ3";
// const char* PASSWORD = "...";
// WZ Home
static constexpr const char* SSID = "WLAN.Tele2.net";
static constexpr const char* PASSWORD = "...";

// WebUpdate Page (ReFlash)
static constexpr const char* SERVER_INDEX =
    "<script "
    "src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></"
    "script>"
    "<form method='POST' action='#' enctype='multipart/form-data' "
    "id='upload_form'>"
    "  <input type='file' name='update'>"
    "  <input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<br><div><a href=\"/\">MyApp</a></div>" // wz
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";

static WebServer server(80);
static bool LED1status = LOW;

static String sendHTML(uint8_t led1stat)
{
  String ptr = "<!DOCTYPE html> <html>\n"
               "<head><meta name=\"viewport\" content=\"width=device-width, "
               "initial-scale=1.0, "
               "user-scalable=no\">\n"
               "<title>LED Control</title>\n"
               "<style>html { font-family: Helvetica; display: inline-block; margin: "
               "0px auto; "
               "text-align: center;}\n"
               "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} "
               "h3 {color: "
               "#444444;margin-bottom: 50px;}\n"
               ".button "
               "{display:block;width:80px;background-color:#3498db;border:none;color:"
               "white;padding:13px "
               "30px;text-decoration:none;font-size:25px;margin:0px auto "
               "35px;cursor:pointer;border-radius:4px;}\n"
               ".button-on {background-color: #3498db;}\n"
               ".button-on:active {background-color: #2980b9;}\n"
               ".button-off {background-color: #34495e;}\n"
               ".button-off:active {background-color: #2c3e50;}\n"
               "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n"
               "</style>\n"
               "</head>\n"
               "<body>\n"
               "<h1>ESP32 Web Server</h1>\n"
               "<h3>Using Station(STA) Mode</h3>\n";

  if (led1stat)
  {
    ptr += "<p>LED1 Status: ON</p><a class=\"button button-off\" "
           "href=\"/off\">OFF</a>\n";
  }
  else
  {
    ptr += "<p>LED1 Status: OFF</p><a class=\"button button-on\" "
           "href=\"/on\">ON</a>\n";
  }

  ptr += "<br><div><a href=\"/serverIndex\">WebUpdate</a></div>"
         "</body>\n"
         "</html>\n";
  return ptr;
}

static void handleOnConnect()
{
  Serial.print("LED Status: ");
  Serial.println(LED1status ? "ON" : "OFF");
  server.send(200, "text/html", sendHTML(LED1status));
}

static void handleLed1On()
{
  LED1status = HIGH;
  Serial.println("LED Status: ON");
  server.send(200, "text/html", sendHTML(true));
}

static void handleLed1Off()
{
  LED1status = LOW;
  Serial.println("LED Status: OFF");
  server.send(200, "text/html", sendHTML(false));
}

static void handleNotFound()
{
  server.send(404, "text/plain", "Not found");
}

static void setupIO()
{
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, OFF);
  LED1status = LOW;
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println();
  Serial.println("TTGO LoRa32-OLED V1");
  Serial.println("ESP32 WebApplication with OTAWebUpdater");
}

static void setupNetwork()
{
  Serial.println("Connecting to ");
  Serial.println(SSID);

  // connect to your local wi-fi network
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");

  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /*use mdns for host name resolution*/
  if (!MDNS.begin(HOST))
  { // http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
}

static void setupWebServer()
{
  server.on("/", handleOnConnect);
  server.on("/on", handleLed1On);
  server.on("/off", handleLed1Off);
  server.onNotFound(handleNotFound);

  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", SERVER_INDEX);
  });
  /*handling uploading firmware file */
  server.on(
      "/update", HTTP_POST,
      []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        delay(1000);
        ESP.restart();
      },
      []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  server.begin();
  Serial.println("HTTP server started");
}

void setup()
{
  setupIO();
  setupNetwork();
  setupWebServer();
}

void loop()
{
  server.handleClient();
  digitalWrite(LEDpin, LED1status ? HIGH : LOW);
}
