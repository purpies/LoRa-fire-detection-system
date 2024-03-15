#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!
// #define NODE_UUID "A"
char NODE_UUID[37];  // 32 hex, 4 hypens (128bits)

// Definitions of corresponding messageType integer values.
#define NEIGHBOUR_DISCOVERY_BROADCAST 1
#define NEIGHBOUR_ACKNOWLEDGE 2
#define FIRE_INTERUPT_BROADCAST 3
#define FIRE_INTERUPT_ACKNOWLEDGE 4

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

int16_t packetnum = 0;  // packet counter, we increment per transmission (Msg ID)

struct Message {
  char messageID;          // Message's UID
  char senderUUID[37];     // Sender's/Node's UUID
  // char recipientUUID[37];  // recipientUUID used for reply purposes.
  char messageType;        // 4 different integer value to signify message type.
  char payload[80];        // Adjust payload size as needed
  char checksum;
};

void (*resetFunc)(void) = 0;  //declare reset function at address 0

void setup() {

  Serial.begin(9600);
  delay(100);

  // //Assigning this node a UUID
  randomUUID(NODE_UUID);
  Serial.print("UUID Generated: ");
  Serial.println(NODE_UUID);

  // Future work to setup a device alias (simple name) here for easy understanding e.g. LT-2A, DR-3Q etc. //

  // Manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  // Initializing of LoRa
  while (!rf95.init()) {
    Serial.println("LoRa radio init has failed");
    while (1)
      ;
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("Setting of frequency has failed");
    while (1)
      ;
  }

  Serial.print("Setting frequency to:");
  Serial.println(RF95_FREQ);
  rf95.setTxPower(13, false);

  delay(500);
  // To establish connections with any existing/present neighbours on the network. Broadcast & wait for acknowledgements.
  Serial.println("Broadcasting Existence....");
  sendMessage(NEIGHBOUR_DISCOVERY_BROADCAST);
  delay(4500);
  rf95.setModeRx();
}


void loop() {
  Serial.println("Listening...");
  delay(1200);

  if (rf95.available()) {
    Message receivedMessage;
    uint8_t len = sizeof(receivedMessage);
    Serial.println("Found a message!");

    if (rf95.recv((uint8_t *)&receivedMessage, &len)) {
      delay(2000);
      if (validateChecksum(receivedMessage)) {
        if (receivedMessage.senderUUID != NODE_UUID) {
          // Serial.println((char)receivedMessage.messageType, DEC);
          // delay(5000);
          switch (receivedMessage.messageType) {
            case NEIGHBOUR_DISCOVERY_BROADCAST:
              Serial.println("\nA message to establish connection is found");
              Serial.println(receivedMessage.payload);
              Serial.println("Sending an acknowledement reply...");
              delay(7500);
              sendMessage(NEIGHBOUR_ACKNOWLEDGE);
              break;

            case NEIGHBOUR_ACKNOWLEDGE:
              Serial.println("A reply message of successful neighbouring is found");
              Serial.println(receivedMessage.payload);
              break;
          }
        } else {
          // Ignore message as sender is ownself.
          Serial.println("THIS is a message broadcasted by own device.");
        }
      } else {
        Serial.println("Checksum validation failed");
      }

    } else {
      Serial.println("Receive failed");
    }
  }
}



void sendMessage(char messageType) {
  rf95.setModeTx();
  delay(2000);
  // Create a Message instance
  Message message;

  message.messageID = packetnum;
  strncpy(message.senderUUID, NODE_UUID, sizeof(message.senderUUID));
  message.messageType = messageType;

  switch (messageType) {
    case 1:  // Case 1 == Neighbour Broadcast Message
      // strncpy(message.recipientUUID, "0xFF", sizeof(message.recipientUUID));
      sprintf(message.payload, "I am new here! %d from %s", int(packetnum++), NODE_UUID);
      Serial.println("Sending a broadcast message to find neighbours.....");
      break;
    case 2:  // Case 2 == Reply from nodes that are returning the handshake to being neighbours
      // strncpy(message.recipientUUID, "0xAA", sizeof(message.recipientUUID));
      sprintf(message.payload, "Welcome neighbour! ack %d from %s", int(packetnum++), NODE_UUID);
      Serial.println("Sending a reply message back to neighbour......");
      break;
  }

  message.checksum = calculateChecksum(message);

  rf95.send((uint8_t *)&message, sizeof(message));

  Serial.println("\tWait for packet to complete...");
  rf95.waitPacketSent();
  delay(2000);
  Serial.println("\tPacket sent successfully");
  rf95.setModeRx();
}

//Checksum function for integrity purposes. * Security *
char calculateChecksum(const Message &message) {
  char checksum = 0;
  checksum ^= message.messageID;
  // checksum ^= message.senderUUID;
  for (int i = 0; i < sizeof(message.senderUUID); i++) {
    checksum ^= message.senderUUID[i];
  }
  // for (int i = 0; i < sizeof(message.recipientUUID); i++) {
  //   checksum ^= message.recipientUUID[i];
  // }
  checksum ^= message.messageType;
  for (int i = 0; i < sizeof(message.payload); i++) {
    checksum ^= message.payload[i];
  }
  return checksum;
}

// Validation of checksum for security purposes
bool validateChecksum(const Message &message) {
  char checksum = 0;
  checksum ^= message.messageID;
  // checksum ^= message.senderUUID;
  for (int i = 0; i < sizeof(message.senderUUID); i++) {
    checksum ^= message.senderUUID[i];
  }
  // for (int i = 0; i < sizeof(message.recipientUUID); i++) {
  //   checksum ^= message.recipientUUID[i];
  // }
  checksum ^= message.messageType;
  for (int i = 0; i < sizeof(message.payload); i++) {
    checksum ^= message.payload[i];
  }
  return checksum == message.checksum;
}

void randomUUID(char *uuid) {
  char characters[] = "0123456789abcdef";
  randomSeed(analogRead(0));
  for (int i = 0; i < 36; i++) {
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      uuid[i] = '-';
    } else {
      uuid[i] = characters[random(16)];
    }
  }
  uuid[36] = '\0';  // null terminator
}
