#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

//-------NTPサーバー時刻取得引数初期化-----------------------------
IPAddress _NtpServerIP;
const int NTP_PACKET_SIZE = 48;     // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
const int timeZone = 9;             // Tokyo
WiFiUDP Udp;
unsigned int localPort = 8888; // local port to listen for UDP packets
//--------------------------------------------------------

void getTime()
{
    //NTPサーバーから時刻を取得---------------------------
    const char *NtpServerName = "ntp.nict.jp";
    WiFi.hostByName(NtpServerName, _NtpServerIP);
    Serial.print(NtpServerName);
    Serial.print(": ");
    Serial.println(_NtpServerIP);
    Udp.begin(localPort);
    Serial.print(F("now="));
    Serial.println(now());
}

//*********************** NTP Time ************************************
void sendNTPpacket(IPAddress &address)
{
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

//*********************** NTP Time **************************************
time_t getNtpTime()
{
    while (Udp.parsePacket() > 0)
        ; // discard any previously received packets
    Serial.println("Transmit NTP Request");
    sendNTPpacket(_NtpServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 5000)
    {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE)
        {
            Serial.println("Receive NTP Response");
            Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
            unsigned long secsSince1900;
            // convert four bytes starting at location 40 to a long integer
            secsSince1900 = (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
        }
    }
    Serial.println("No NTP Response :-(");
    ESP.restart();
    return 0; // return 0 if unable to get the time
}

void beginUdp(){
    Udp.begin(localPort);
    setSyncProvider(getNtpTime);
}