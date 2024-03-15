#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
#define RF95_FREQ 915.0  // Change to 434.0 or other frequency, must match RX's freq!
#define MY_NODE_ID "A"

// Definitions of corresponding messageType integer values.
#define NEIGHBOUR_DISCOVERY_BROADCAST 1
#define NEIGHBOUR_ACKNOWLEDGE 2
#define FIRE_INTERUPT_BROADCAST 3
#define FIRE_INTERUPT_ACKNOWLEDGE 4

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

int16_t packetnum = 0;  // packet counter, we increment per transmission (Msg ID)

struct Message {
  char messageID;    // Message's UID
  char senderID;     // Sender's/Node's UID
  char messageType;  // 4 different integer value to signify message type.
  char payload[40];  // Adjust payload size as needed
  char checksum;
};

void (*resetFunc)(void) = 0;  //declare reset function at address 0

void setup() {
  Serial.begin(9600);
  delay(100);

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

  delay(2000);
  // To establish connections with any existing/present neighbours on the network. Broadcast & wait for acknowledgements.
  Serial.println("\n\nBroadcasting Existence....");
  sendMessage(NEIGHBOUR_DISCOVERY_BROADCAST);
  delay(4000);
}


void loop() {
  Serial.println("Listening...");
  delay(1200);

  if (rf95.available()) {
    Message receivedMessage;
    uint8_t len = sizeof(receivedMessage);

    if (rf95.recv((uint8_t *)&receivedMessage, &len)) {
      delay(2000);
      if (validateChecksum(receivedMessage)) {
        if (receivedMessage.senderID != MY_NODE_ID) {
          Serial.println((char)receivedMessage.messageType, DEC);
          // delay(5000);
          switch (receivedMessage.messageType) {
            case NEIGHBOUR_DISCOVERY_BROADCAST:
              Serial.println("\nA message to establish connection is found");
              Serial.println("Sending an acknowledement reply...");
              delay(4000);
              sendMessage(NEIGHBOUR_ACKNOWLEDGE);
              delay(4000);
              Serial.println("ACK SENT\n");
              break;

            case NEIGHBOUR_ACKNOWLEDGE:
              Serial.println("A reply message of successful neighbouring is found");
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
  delay(1000);
  // Create a Message instance
  Message message;

  message.messageID = packetnum;
  message.senderID = MY_NODE_ID;
  message.messageType = messageType;

  switch (messageType) {
    case 1:
      sprintf(message.payload, "I am new here! %d from %c", int(packetnum++), MY_NODE_ID);
      Serial.println("ENTERED msgtype case 1");
      break;
    case 2:
      sprintf(message.payload, "Welcome neighbour! ack %d from %c", int(packetnum++), MY_NODE_ID);
      Serial.println("ENTERED msgtype case 2: Reply");
      break;
  }

  message.checksum = calculateChecksum(message);

  rf95.send((uint8_t *)&message, sizeof(message));

  Serial.println("Wait for packet to complete...");
  rf95.waitPacketSent();
  Serial.println("Packet sent successfully");
  delay(1000);
}

//Checksum function for integrity purposes. * Security *
char calculateChecksum(const Message &message) {
  char checksum = 0;
  checksum ^= message.messageID;
  checksum ^= message.senderID;
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
  checksum ^= message.senderID;
  checksum ^= message.messageType;
  for (int i = 0; i < sizeof(message.payload); i++) {
    checksum ^= message.payload[i];
  }
  return checksum == message.checksum;
}
