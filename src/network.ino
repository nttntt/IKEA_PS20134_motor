/* マルチタスクでOTA待ち受け */
/*void OTATask(void *pvParameters)
{
  connectToWifi();  // Wi-Fiルーターに接続する
//  startMDNS();      // Multicast DNS
  startOTA();

 while (true)
  {
    ArduinoOTA.handle();
    delay(1);
  }
}

/* Wi-Fiルーターに接続する */
void connectToWifi()
{
  Serial.print("Connecting to Wi-Fi ");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // モニターにローカル IPアドレスを表示する
  Serial.println("WiFi connected.");
  Serial.print("  *IP address: ");
  Serial.println(WiFi.localIP());
}

/* Multicast DNSを開始する */
void startMDNS()
{
  Serial.print("mDNS server instancing ");
  while (!MDNS.begin(DEVICE_NAME))
  {
    Serial.print(".");
    delay(100);
  }
  Serial.print("  mDNS NAME:");
  Serial.print(DEVICE_NAME);
  Serial.println(".local");
}

void startOTA()
{
  ArduinoOTA.setHostname(DEVICE_NAME)
      .onStart([]()
               {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else  // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
        // using SPIFFS.end()
        Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed"); });
  ArduinoOTA.begin();
  Serial.println("OTA Started.");
}