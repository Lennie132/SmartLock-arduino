/*
   SmartLock by Lennart Veringmeier
   Student-number: 0893738
   Email:          0893738@hr.nl

   NEEDED:
   - Ethernet Shield
   - RFID Reader (RFID-RC522)
   - Servo
   - 3 LEDs
   - Server
*/

#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <Ethernet.h>

/* Pinlayout for RFID reader
  RFID  	Arduino UNO
  SDA        7
  SCK        13
  MOSI       11
  MISO       12
  IRQ        not used
  GND        GND
  RST        6
  +3.3V      3V3
*/

// EHTERNET
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte server[] = {192, 168, 178, 107 };
EthernetClient client;

// RFID
#define RST_PIN 7
#define SS_PIN 6
MFRC522 rfid(SS_PIN, RST_PIN);

// SERVO
Servo servo;

// LEDs
#define ledAccess     2  // Access led on pin 2
#define ledClosed     3  // CLosed led on pin 3
#define ledDenied     4  // Denied led on pin 4

void setup() {
  Serial.begin(9600);
  Serial.println("---------------------------------");
  Serial.println("***SETUP***");

  SPI.begin();

  rfid.PCD_Init();

  Serial.println("connect via DHCP");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("DHCP failed");
    Serial.println("Restart the system");
    while (1 == 1) { }
  }
  Serial.print("IP adress: ");
  Serial.println(Ethernet.localIP());

  pinMode(ledAccess, OUTPUT);
  pinMode(ledClosed, OUTPUT);
  pinMode(ledDenied, OUTPUT);

  servo.attach(5);
  servo.write(0);

  Serial.println("READY: present card...");
  Serial.println("---------------------------------");
}

void loop() {
  digitalWrite(ledClosed, HIGH); //Red led on ("Default")

  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String code = "";
  String response = "";
  boolean access = false;

  Serial.println("***CARD DETECTED***");

  code = readTag();
  Serial.print("Code: ");
  Serial.println(code);

  if (authRequest(code)) {
    Serial.println("Request: success");

    response = readClientData();

    access = checkAuthorization(response);

    if (access) {
      Serial.println("***ACCESS ALLOWED***");
      accessAllowed();
      Serial.println("***DOOR CLOSED***");
    } else {
      Serial.println("***ACCESS DENIED***");
      accessDenied();
    }
  } else {
    Serial.println("Request: failed");
  }

  Serial.println("---------------------------------");

  delay(500);
}

String readTag() {
  String content = "";
  byte letter;
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content;
}

boolean authRequest(String code) {
  /**
     GET request with paramaters.

     client.print writes the request to the server

     @return boolean
  */
  if (client.connect(server, 80)) {
    client.print("GET /SmartLock/api/authenticate.php?lock_id=1&rfid_code=");
    client.print(code);
    client.println(" HTTP/1.1");
    client.println("Host: 192.168.178.107");
    client.println();
    return true;
  } else {
    return false;
  }
}

String readClientData() {
  String content = "";
  // wait for another packet
  while (client.connected()) {
    // read this packet
    while (client.available()) {
      char c = client.read();
      content.concat(c);
    }
  }
  // close the connection after the server closes its end
  client.stop();
  return content;
}

boolean checkAuthorization(String content) {
  if (content.indexOf("access:true") >= 0) {
    return true;
  } else if (content.indexOf("access:false") >= 0) {
    return false;
  } else {
    return false;
  }
}

void accessAllowed() {
  digitalWrite(ledAccess, HIGH);
  digitalWrite(ledClosed, LOW);
  servo.write(90);                  //Servo open (reverse clock turn)
  delay(5000);
  digitalWrite(ledAccess, LOW);
  digitalWrite(ledClosed, HIGH);
  servo.write(0);                   //Servo close (clock turn)
}

void accessDenied() {
  digitalWrite(ledDenied, HIGH);
  delay(2000);
  digitalWrite(ledDenied, LOW);
}

