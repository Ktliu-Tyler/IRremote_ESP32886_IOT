#include <ESP8266WiFi.h> // 使用 ESP8266WiFi 库
#include <PubSubClient.h>
#include <IRremoteESP8266.h> // 使用适用于 ESP8266 的 IRremote 库
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

// WiFi 配置
// const char* ssid = "APRDC-Public";   
// const char* password = "23456789";  

const char* ssid = "AndroidAPFF94";     
const char* password = "lank5516"; 



// MQTT Broker
const char* mqttServer = "192.168.43.53"; // 你的本地 IP
const int mqttPort = 1883;

// MQTT 
WiFiClient espClient;
PubSubClient client(espClient);

const uint16_t recvPin = 2; // D2
const uint16_t sendPin = 4; // D1
IRrecv irrecv(recvPin);
IRsend irsend(sendPin);
decode_results results;

void setup() {
  Serial.begin(115200);

  // 设置 WiFi 
  setup_wifi();

  // 设置 MQTT 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
#if ESP8266
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(115200, SERIAL_8N1);
#endif  // ESP8266
  irrecv.enableIRIn(); 
  irsend.begin();  
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char msg[length];
  for (unsigned int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
  }
  msg[length] = '\0';
  const unsigned long long value = strtoull(msg, NULL, 0);
  irsend.sendSymphony(value, 12);
  Serial.println(msg);

  if (strcmp(topic, "home/ir/send/esp1") == 0) {
    unsigned long irCode = strtoul(msg, NULL, 10);
    irsend.sendNEC(irCode, 32);
    Serial.println("IR code sent");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("home/ir/send/esp1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // Serial.printf("Running..\n");
  // if (irrecv.decode(&results)) {
  //   unsigned long irCode = results.value;
  //   char msg[32];
  //   sprintf(msg, "%lu", irCode);
  //   // client.publish("home/ir/receive/esp1", msg);
  //   irrecv.resume();
  // }
  if (irrecv.decode(&results)) {
    Serial.println("Received IR signal");
    Serial.print("Raw Data: ");
    Serial.println(results.value, DEC);  // 打印原始數據
    Serial.print("Bits: ");
    Serial.println(results.bits);
    Serial.print("Protocol: ");
    Serial.println(typeToString(results.decode_type));
    Serial.print("Adress: ");
    Serial.println(results.address);
    if(results.decode_type != UNKNOWN) {
      char msg[32];
    // sprintf(msg, "%u-%u-%u-%s", results.value,results.bits,results.address, typeToString(results.decode_type));
    sprintf(msg, "%llu", results.value);
        client.publish("home/ir/receive/esp1", msg);
    }
    
    irrecv.resume();  // 接收下一個信號
    Serial.println(results.value, HEX);
    Serial.println();
  }
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n'); 
    Serial.print("Publishing message: ");
    Serial.println(message);
    client.publish("home/ir/receive/device1", message.c_str());
  }
}
