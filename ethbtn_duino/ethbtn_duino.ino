// Includes
#include <SPI.h>  //aduino core
#include <Ethernet.h> // arduino CORE V>2.0

#include <EthernetUdp.h> // arduino core
#include <OSCMessage.h> //CNMAT
#include <OSCBundle.h> //CNMAT

EthernetUDP Udp;

// Definitions
// UID (unique identifier)
#define UID "agir"
// SERIAL_DEBUG
#define SERIAL_DEBUG 0
// OSC_OUT_ADDRESS
// use publish subscribe methode? /broadcast?
// OSC_OUT_PORT
#define OSC_OUT_PORT 52022
// OSC_IN_PORT
#define OSC_IN_PORT 52021
// DHCP
// YES (Default)
// MAC ADDRESS
// Tweak UID to MAC address
// LED_1_PIN
#define LED_1_PIN 5
byte LED_1_BRIGHTNESS = 0;
// PIN_INPUT_BTN
#define BTN_1_PIN 3
bool btn_1_state = 0;
bool btn_1_flag = 0;
bool send_message_flag = 0;

// GLOBAL VARs
//https://www.arduino.cc/en/Tutorial/LibraryExamples/UDPSendReceiveString
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,


// ETHERNET_ERROR_STATE
// 0 = clean
// 1 = undefinde error
// 2 = Failed to configure Ethernet using DHCP
// 3 = EthernetNoHardware
//
int ethernet_error_state = 1;
char *ethernet_error_msg[] = {
  "Linked",
  "Ethernet:undefined_error",
  "Ethernet:Failed_to_configure_using_DHCP",
  "Ethernet:Hardware_was_not_found",
  "Ethernet:cable_is_not_connected"
};

// Mac Address
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x09};

void setup()
{
  if (SERIAL_DEBUG)
  {
    Serial.begin(9600);
  }
  test_hardware();
  attachInterrupt(digitalPinToInterrupt(BTN_1_PIN), raise_btn_1_flag, CHANGE);
  init_DHCP();
  print_status();
  broadcast_message();
}

void loop()
{
  update_btn_1_state();
  ethernet_maintain();
  OSC_get();
  OSC_send();
}


void raise_btn_1_flag()
{
  btn_1_flag = 1;
}

void update_btn_1_state()
{
  if (btn_1_flag)
  {
    if (digitalRead(BTN_1_PIN) != btn_1_state)
    {
      btn_1_state = digitalRead(BTN_1_PIN);
      btn_1_flag = 0;
      send_message_flag = 1;
      if (SERIAL_DEBUG)
      {
        String payload = "/";
        payload += UID;
        payload += "/";
        payload += "btn_1_state/";
        payload += btn_1_state;
        Serial.println(payload);
      }
    }
  }
}

void init_DHCP()
{
  //todo : faire fonctionner adéquatement les msg erreurs.
  if (Ethernet.begin(mac) == 0) //start DHCP
  {
    ethernet_error_state = 2; //DHCP failed to init
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      ethernet_error_state = 3; //"Ethernet: Hardware was not found"
    } else if (Ethernet.linkStatus() == LinkOFF) {
      ethernet_error_state = 4; //"cable is not connected"
    }
  } else {
    ethernet_error_state = 0; //"connected"
    Udp.begin(OSC_IN_PORT);
  }
}

String boot_report = "";

void print_status()
{
  // amettre dans un string
  boot_report += "/UID=";
  boot_report += UID;
  boot_report += "/BTN_state=";
  boot_report += btn_1_state;
  boot_report += "/ETH_status=";
  boot_report += ethernet_error_msg[ethernet_error_state];
  boot_report += "/My_IP=";
  boot_report += ip_to_string(Ethernet.localIP());
  boot_report += "/Listening=";
  boot_report += OSC_IN_PORT;
  boot_report += "/Output=";
  boot_report += OSC_OUT_PORT;
  boot_report += "/";
  if (SERIAL_DEBUG)
  {
    Serial.println(boot_report);
  }
}

String ip_to_string(IPAddress address)
{
  return String(address[0]) + "." +
         String(address[1]) + "." +
         String(address[2]) + "." +
         String(address[3]);
}


void test_hardware()
{

  //led
  int delay_ms = 5;
  for (int i = 0; i < 255; i++)
  {
    analogWrite(LED_1_PIN, i);
    delay(delay_ms);
  }
  for (int i = 255; i >= 0; i--)
  {
    analogWrite(LED_1_PIN, i);
    delay(delay_ms);
  }
  //button
  pinMode(BTN_1_PIN, INPUT_PULLUP);
  btn_1_state = digitalRead(BTN_1_PIN);
}


void ethernet_maintain()
{
  //https://www.arduino.cc/en/Reference/EthernetMaintain
  //byte:
  //0: nothing happened
  //1: renew failed
  //2: renew success
  //3: rebind fail
  //4: rebind success

  switch (Ethernet.maintain()) {
    case 1://renewed fail
      if (SERIAL_DEBUG)
      {
        Serial.println("Error: renewed fail");
      }
      break;

    case 2: //renewed success
      if (SERIAL_DEBUG)
      {
        //print your local IP address:
        print_status();
        Serial.println("Renewed success");
      }

      break;

    case 3: //rebind fail
      if (SERIAL_DEBUG)
      {
        Serial.println("Error: rebind fail");
      }
      break;

    case 4://rebind success
      if (SERIAL_DEBUG)
      {
        //print your local IP address:
        print_status();
        Serial.println("Rebind success");
      }

      break;

    default://nothing happened
      break;
  }
}




void OSC_get()
{
  OSCBundle bundleIN;
  int size;
  if ( (size = Udp.parsePacket()) > 0)
  {
    while (size--)
      bundleIN.fill(Udp.read());

    if (!bundleIN.hasError()) {}
    bundleIN.route("/led", routeLed);
  }
}

void routeLed(OSCMessage &msg, int addrOffset )
{
  LED_1_BRIGHTNESS = byte(msg.getFloat(0) * 255);
  if (SERIAL_DEBUG)
  {
    Serial.print("route_led ");
    Serial.println(LED_1_BRIGHTNESS);
  }

  analogWrite(LED_1_PIN, LED_1_BRIGHTNESS);
}

void OSC_send()
{
  if (send_message_flag)
  {
    String str_payload = "/";
    str_payload += UID;
    str_payload += "/";
    str_payload += "btn_state/";

    char payload [str_payload.length()];
    str_payload.toCharArray(payload, str_payload.length());
    OSCMessage msg(payload);

    IPAddress ip(255, 255, 255, 255);
    Udp.beginPacket(ip , OSC_OUT_PORT);
    msg.add(int(btn_1_state));
    msg.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message
    send_message_flag = 0;
  }
}




void broadcast_message()
{
  OSCMessage msg("/broadcast");

  IPAddress ip(255, 255, 255, 255);
  Udp.beginPacket(ip , OSC_OUT_PORT);

  char payload [boot_report.length()];
  boot_report.toCharArray(payload, boot_report.length());
  msg.add(payload);
  Udp.beginPacket(ip , OSC_OUT_PORT);
  msg.send(Udp); // send the bytes to the SLIP stream
  Udp.endPacket(); // mark the end of the OSC Packet
  msg.empty(); // free space occupied by message
}

void route_led_1_brightness (OSCMessage & msg, int addrOffset )
{

}
