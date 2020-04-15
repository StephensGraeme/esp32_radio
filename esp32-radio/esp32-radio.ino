/**
    Copyright 2020 Phil Schatzmann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#define NDEBUG

#include <vector>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "ESPAsyncTunnel.h"
#include "ArduinoJson.h"
#include "Radio.hpp"


// WIFI
const String ssid = "your ssid";
const String password = "password";

// Web Server & Sevices
AsyncWebServer server(80);
String path = "/esp32_radio/vue-radio/dist";
String pathMatch = "/esp32_radio/*";

String indexPath = path+"/index.html";
String iconPath = path+"/favicon.ico";
String serverPathHttp = "http://github.pschatzmann.ch";
String serverPath = "https://pschatzmann.github.io";

// Radio
Radio radio;

void setupWifi() {
  Serial.begin(115200);
  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}


void setupServer() {
  // rewrites
  server.rewrite( "/", indexPath.c_str()) ;
  server.rewrite( "/index.html", indexPath.c_str());
  server.rewrite( "/favicon.ico", iconPath.c_str());


  // Generic Services
  server.on("/service/info", HTTP_GET, [](AsyncWebServerRequest *request){
      ESP_LOGI("[eisp32_radio]","info");    
      radio.sendResponse(request);
  });

  //Shut down server
  server.on("/service/shutdown", HTTP_POST, [](AsyncWebServerRequest *request){
      ESP_LOGI("[eisp32_radio]","shutdown");    
      esp_deep_sleep_start();
  });

  server.on("/service/bluetooth/start", HTTP_POST, [](AsyncWebServerRequest *request){
      radio.startBluetooth();       
      radio.sendResponse(request);
  });

  server.on("/service/bluetooth/stop", HTTP_POST, [](AsyncWebServerRequest *request){
      radio.stopBluetooth();
      radio.sendResponse(request);
  });


  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if (request->url() == "/service/streaming/start") {
        String url = radio.getMusicURL(data, len);        
        radio.startStreaming(url);
        radio.sendResponse(request);
    }
  });


  server.on("/service/streaming/stop", HTTP_POST, [](AsyncWebServerRequest *request){
      radio.stopStreaming();
      radio.sendResponse(request);
  });

  server.on("/service/stop", HTTP_POST, [](AsyncWebServerRequest *request){
    radio.stopStreaming();
    radio.stopBluetooth();
  });

  
  // tunnel the index.html request. We need to avoid https because this is consuming too much memory
  // so we get the index.html from a http source
  server.on(serverPathHttp.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request){
      ClientRequestTunnel tunnel; 
      if (tunnel.open(serverPath.c_str(), request->url())) {
          String result = tunnel.getString();
          //request->send(200, "text/html", result); 
          AsyncWebServerResponse *response = request->beginResponse(200, "text/html", result);
          response->addHeader("Access-Control-Allow-Origin:","*");
          request->send(response);
         
      } else {
          request->send(tunnel.getHttpCode());
      }
  });

  server.on(pathMatch.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request){
    String moved_url = serverPath+request->url();
    request->redirect(moved_url);
  });


  // start server
  server.begin();
  // automatically start Bluetooth ?
  //radio.startBluetooth();

}

void setup(){  
  audioLogger = &Serial;
  setupWifi();
  setupServer();

  ESP_LOGI("[eisp32_radio]","free heep: %u", ESP.getFreeHeap());    
  
  Serial.println();
  Serial.print("You can connect to ");
  Serial.println(WiFi.localIP());  
}


void loop(){
  radio.loop();

}
