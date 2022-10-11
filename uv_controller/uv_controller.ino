#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

// Initialize Telegram BOT
#define BOTtoken ""  // your Bot Token (Get from Botfather)
#define Master "" // chat_id to get notification on boot
// SSID to connect to
const char* ssid = ""; //Wifi SSID
const char* password = ""; //Wifi Password

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define LED_PIN LED_BUILTIN

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

const int ledPin = LED_BUILTIN;
bool ledState = LOW;

bool glow_no_timer = false;
bool safe_door = false;
bool door_status = false;
bool start_timer = false;
long exposure_time = 0;
long exposure_timer = 0;
int exposure_dose = 10;
int distance = 10;
float radiance_1m = 45;
#define relay_pin 5
#define sensor_pin 4

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
  
    // If the type is a "callback_query", a inline keyboard button was pressed
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (bot.messages[i].type ==  F("callback_query")) {
      Serial.print("Call back button pressed with text: ");
      Serial.println(text);

      if (text == "/Start"){
        glow_no_timer = true;
        exposure_timer = 0;
      }
      else if (text == "/Stop"){
        glow_no_timer = false;
        exposure_timer = 0;
      }
      else if (text == "/timer"){
        start_timer=true;
        bot.sendMessage(chat_id, "Started timer " + String(exposure_time) + " s which is equivalent to the dose of " + String(exposure_dose) + " mJ/cm² at the distance " + String(distance) + " cm", "");
      }
      else if (text == "/door_on"){
        safe_door = true;
      }
      else if (text == "/door_off"){
        safe_door = false;
      }
    
    } 


    else 
    {
      String from_name = bot.messages[i].from_name;
      
      if (text == F("/options")) { 
        String keyboardJson = F("[[{ \"text\" : \"Start\", \"callback_data\" : \"/Start\" }],");
        keyboardJson += F("[{ \"text\" : \"Door sensor on\", \"callback_data\" : \"/door_on\" }, { \"text\" : \"Door sensor off\", \"callback_data\" : \"/door_off\" }],");
        keyboardJson += F("[{ \"text\" : \"Start the timer\", \"callback_data\" : \"/timer\" }],");
        keyboardJson += F("[{ \"text\" : \"Stop\", \"callback_data\" : \"/Stop\" }]]");
        bot.sendMessageWithInlineKeyboard(chat_id, "Nice buttons", "", keyboardJson);
      }
      else if (text == F("/start")) {
        bot.sendMessage(chat_id, "Welcome to UV lamp bot! Send /help to get started", "Markdown");
      }
      else if (text == F("/help")) {
        bot.sendMessage(chat_id, "/options : returns the inline keyboard\n/t seconds : allows to set up **exposure** time in seconds\n/dis : allows to set the distance from the source in cm\n/dose number : sets up exposure dose in mJ/cm²(130 to kill the yeast)", "Markdown");
      }
      //  130mJ/cm2 required to kill normal yeast
      else if (text.startsWith("/t "))
      {
        text.remove(0,3);
        exposure_time=(text.toInt() ? text.toInt() : 0);
        exposure_dose=exposure_time*(radiance_1m*10/(distance*distance));
        bot.sendMessage(chat_id, "Exposure time is set to: " + String(exposure_time) + " s which is equivalent to the dose of " + String(exposure_dose) + " mJ/cm² at the distance " + String(distance) + " cm", "");
      }
      else if (text.startsWith("/dose "))
      {
        text.remove(0,6);
        exposure_dose=(text.toInt() ? text.toInt() : 0);
        exposure_time = exposure_dose/(radiance_1m*10/(distance*distance));
        bot.sendMessage(chat_id, "Dose is set to: " + String(exposure_dose) + "mJ/cm² and exposure time to " + String(exposure_time) + " at the distance " + String(distance) + " cm", "");
      }
      else if (text.startsWith("/dis "))
      {
        text.remove(0,5);
        distance=(text.toInt() ? text.toInt() : 0);
        exposure_time = exposure_dose/(radiance_1m*10/(distance*distance));
        bot.sendMessage(chat_id, "Distance is set to: " + String(distance) + "cm and exposure time to "+ String(exposure_time) + " seconds which will reach the dose of " + String(exposure_dose) + "mJ/cm²", "");
      }
      else 
      {
          bot.sendPhoto(chat_id, "https://www.pacificlicensing.com/sites/default/files/brand/shrek.jpg", "Unknown command\nShame on you!");
      }
    }
  }
}


void setup() {

  WiFi.mode(WIFI_STA);
  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif
  

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, HIGH);
  pinMode(sensor_pin, INPUT);
  digitalWrite(LED_BUILTIN, ledState);
  Serial.begin(115200);
  delay(1000);
  Serial.printf("SDK version: %s\n", system_get_sdk_version());
  Serial.printf("Free Heap: %4d\n",ESP.getFreeHeap());
  

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, ledState);
    ledState!=ledState;
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  for (int i=0; i<10;i++){
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
  bot.sendMessage(Master, "Bot woke up", "");
}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    
    Serial.println("Real delay: " + String((millis() - lastTimeBotRan - botRequestDelay)/1000.0));
    lastTimeBotRan = millis();
  }
  if (safe_door){
    door_status = !digitalRead(sensor_pin);
  }
  if (start_timer){
    start_timer=false;
    exposure_timer = millis() + exposure_time*1000;
  }
  if (glow_no_timer){
    digitalWrite(LED_PIN, LOW);
    digitalWrite(relay_pin, LOW);
    }
  else if ((exposure_timer > millis())){
    if (safe_door){
      if (door_status){
        digitalWrite(LED_PIN, LOW);
        digitalWrite(relay_pin, LOW);
      }
      else {
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(relay_pin, HIGH);
      }
    }
    else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(relay_pin, LOW);
    }
  }
  else {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(relay_pin, HIGH);
  }
}
