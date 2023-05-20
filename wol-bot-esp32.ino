#include "WiFiClientSecure.h"
#include "WiFiMulti.h"
#include "WiFiUDP.h"

#include "ArduinoJson.h"
#include "UniversalTelegramBot.h"
#include "WakeOnLan.h"

#include "config.h"

// LED Colors
#define WHITE 0xFFFFFF
#define RED 0x800000
#define GREEN 0x008000
#define PURPLE 0x800080
#define AMBER 0xFF4000
#define OFF 0x000000

WiFiMulti wifiMulti;
WiFiClientSecure secured_client;
WiFiUDP UDP;
WakeOnLan WOL(UDP);

const unsigned long BOT_MTBS = 1000; // time between scan messages
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done

void sendWOL()
{
    WOL.sendMagicPacket(MAC_ADDR); // send WOL on default port (9)
    delay(300);
}

void handleNewMessages(int numNewMessages)
{
    Serial.print("handleNewMessages ");
    Serial.println(numNewMessages);

    for (int i = 0; i < numNewMessages; i++) {
        Serial.println(bot.messages[i].from_id);
        if (bot.messages[i].from_id != ALLOWED_ID)
            continue;

        String chat_id = bot.messages[i].chat_id;
        String text = bot.messages[i].text;

        String from_name = bot.messages[i].from_name;
        if (from_name == "")
            from_name = "Guest";

        if (text == "/wol") {
            sendWOL();
            bot.sendMessage(chat_id, "Magic Packet sent!", "");
        } else if (text == "/ping") {
            bot.sendMessage(chat_id, "Pong.", "");
        } else if (text == "/start") {
            String welcome = "Welcome to **WoL Bot**, " + from_name + ".\n";
            welcome += "Use is restricted to the bot owner.\n\n";
            welcome += "/wol : Send the Magic Packet\n";
            welcome += "/ping : Check the bot status\n";
            bot.sendMessage(chat_id, welcome, "Markdown");
        }
    }
}

void connectToWifi()
{
    while (wifiMulti.run() != WL_CONNECTED) { // wait for WiFi connection.
        delay(500);
        Serial.print(".");
    }
    Serial.print("OK");
}

/* =================================== SETUP =================================== */

void setup()
{
    // Configure WiFI
    wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
    // Clear the serial port buffer and set the serial port baud rate to 115200.
    // Do not Initialize I2C. Initialize the LED matrix.

    delay(25);

    // Add root certificate for api.telegram.org
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    Serial.print("Connecting to WiFI...");
    connectToWifi();

    // Attention: 255.255.255.255 is denied in some networks
    WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());

    delay(100);

    Serial.print("Retrieving time...");
    configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
    time_t now = time(nullptr);
    while (now < 24 * 3600) {
        Serial.print(".");
        delay(150);
        now = time(nullptr);
    }
    Serial.println(now);

    delay(100);
}

/* =================================== LOOP =================================== */

void loop()
{
    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected. Attempting to reconnect...");
        connectToWifi();
    }

    long numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
        Serial.println("Response received");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();

    delay(BOT_MTBS);
}
