/*
 Name:		LightStrip.ino
 Created:	08.11.2022 23:24:34
 Author:	maily
*/
#include <dummy.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <DataStore.h>
#include <Piece.h>
#include <LightsMgr.h>
#include <iostream> 

using namespace std;

#define LED_PIN     4
#define LED_COUNT  300

#define wifi_ssid "Angeldrive Studios 2,4"
#define wifi_password "Garten123!"
#define mqtt_server "192.168.178.183"
#define mqtt_user "mqtt"         
#define mqtt_password "Garten123!"

#define ESPHostname "esp8266_lightstrip"
#define data_topic "esp01/data"
#define inTopic "esp01/inTopic"
#define outTopic "esp01/outTopic"

#define lightstip_topic_play "lightstrip/value/play"
#define lightstip_topic_prog "lightstrip/value/progNr"
#define lightstip_topic_red "lightstrip/value/red"
#define lightstip_topic_green "lightstrip/value/green"
#define lightstip_topic_blue "lightstrip/value/blue"
#define lightstip_topic_speed "lightstrip/value/speed"
#define lightstip_topic_length "lightstrip/value/length"
#define lightstip_topic_startPos "lightstrip/value/startPos"
#define lightstip_topic_endPos "lightstrip/value/endPos"

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LightsMgr lightsMgr(&strip, LED_PIN, LED_COUNT);
Piece pice(&lightsMgr);
DataStore mqttData;
WiFiClient espClient;
PubSubClient client(espClient);

std::vector<Piece> pieceArray;


void setup() {
	Serial.begin(115200);

	setup_wifi();

	ArduinoOTA.setHostname("esp8266_lightstrip");
	ArduinoOTA.begin();

	client.setServer(mqtt_server, 1883);
	client.setCallback(callback);

	strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
	strip.show();            // Turn OFF all pixels ASAP
	strip.setBrightness(100);

	lightsMgr.Init();
	lightsMgr.setBackgroundColor(0, 0, 0);
	lightsMgr.clear();



}


void loop() {
	if (!client.connected()) {
		reconnect();
	}
	client.loop();

	if (play()) {
		Serial.println("[PLAY:");
		mqttData.serialPrint();
		Piece lightPiece = Piece(&lightsMgr);
		lightPiece.init(&mqttData);
		pieceArray.push_back(lightPiece);
		Serial.println("]");
	}
	
	if (mqttData.getValue("play") == "reset") { pieceArray.clear(); lightsMgr.clear(); }
	if (mqttData.getValue("play") == "data") { Serial.println("[Debug Data:"); mqttData.serialPrint(); mqttPublish("NULL", "lightstrip/value/play"); Serial.println("]"); }

	lightsMgr.clear(false);
	for (size_t i = 0; i < pieceArray.size(); i++)
	{
		pieceArray[i].process();
	}

	lightsMgr.show();

}

bool play() {
	if (mqttData.getValue("play") == "play") return true; 
	else return false;
}

void reconnect() {
	// Loop until we're reconnected
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Create a random client ID
		String clientId = "ESP01-";
		clientId += String(random(0xffff), HEX);
		// Attempt to connect
		if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
			Serial.println("connected");
			// ... and resubscribe

			client.subscribe(lightstip_topic_play);
			client.subscribe(lightstip_topic_prog);
			client.subscribe(lightstip_topic_red);
			client.subscribe(lightstip_topic_green);
			client.subscribe(lightstip_topic_blue);
			client.subscribe(lightstip_topic_speed);
			client.subscribe(lightstip_topic_length);
			client.subscribe(lightstip_topic_startPos);
			client.subscribe(lightstip_topic_endPos);


		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void callback(char* topic, unsigned char *payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");

	string paramName = string(topic).substr(string(topic).find("value/")+6, string(topic).npos);

	mqttData.erase(paramName);
	mqttData.set(paramName, convertToString(payload).erase(length));

	Serial.println(mqttData.getValue(paramName).c_str());

	Serial.print(convertToString(payload).erase(length).c_str());

	Serial.println();
}

string convertToString(unsigned char *a)
{
	string s = "";
	s = (reinterpret_cast<char const*>(a));
	return s;
}
/*
void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);

	}
	Serial.println();
}
*/

void setColor(uint8_t r, uint8_t g, uint8_t b) {
	for (int i = 0; i <= LED_COUNT; i++) {
		strip.setPixelColor(i, strip.Color(r, g, b));
		Serial.println(i);
	}
	strip.show();
}

void setup_wifi() {
	delay(10);
	// We start by connecting to a WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(wifi_ssid);
	WiFi.begin(wifi_ssid, wifi_password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}


void mqttPublish(string payload, string topic) {
	static string printValue;
	static string lastPrint;

	if (!client.connected()) reconnect();

	if (lastPrint != payload) {
		client.publish(topic.c_str(), payload.c_str(), false);

		lastPrint = payload;
	}

}