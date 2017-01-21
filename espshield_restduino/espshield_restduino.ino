/*
  WiFiEsp example: WebServer

  A simple web server that shows the value of the analog input
  pins via a web page using an ESP8266 module.
  This sketch will print the IP address of your ESP8266 module (once connected)
  to the Serial monitor. From there, you can open that address in a web browser
  to display the web page.
  The web page will be automatically refreshed each 20 seconds.

  For more details see: http://yaab-arduino.blogspot.com/p/wifiesp.html
*/

#include "WiFiEsp.h"

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif

char ssid[] = "X";            // your network SSID (name)
char pass[] = "X";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status
int reqCount = 0;                // number of requests received

WiFiEspServer server(80);


void setup()
{
  // initialize serial for debugging
  Serial.begin(115200);
  // initialize serial for ESP module
  Serial1.begin(115200);
  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWifiStatus();

  // start the web server on port 80
  server.begin();
}

//  url buffer size
#define BUFSIZE 255

// Toggle case sensitivity
#define CASESENSE true

#define DEBUG true

void loop()
{
  // listen for incoming clients
  WiFiEspClient client = server.available();
  int index = 0;
  char clientline[BUFSIZE];
  if (client) {
    //  reset input buffer
    index = 0;
    Serial.println("New client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //  fill url the buffer
        if (c != '\n' && c != '\r' && index < BUFSIZE) { // Reads until either an eol character is reached or the buffer is full
          clientline[index++] = c;
          continue;
        }

#if DEBUG
        Serial.print("client available bytes before flush: "); Serial.println(client.available());
        Serial.print("request = "); Serial.println(clientline);
#endif

        // Flush any remaining bytes from the client buffer
        client.flush();

#if DEBUG
        // Should be 0
        Serial.print("client available bytes after flush: "); Serial.println(client.available());
#endif

        //  convert clientline into a proper
        //  string for further processing
        String urlString = String(clientline);

        //  extract the operation
        String op = urlString.substring(0, urlString.indexOf(' '));

        //  we're only interested in the first part...
        urlString = urlString.substring(urlString.indexOf('/'), urlString.indexOf(' ', urlString.indexOf('/')));

        //  put what's left of the URL back in client line
#if CASESENSE
        urlString.toUpperCase();
#endif
        urlString.toCharArray(clientline, BUFSIZE);

        //  get the first two parameters
        char *pin = strtok(clientline, "/");
        char *value = strtok(NULL, "/");

        //  this is where we actually *do something*!
        char outValue[10] = "MU";
        String jsonOut = String();

        if (pin != NULL) {
          if (value != NULL) {

#if DEBUG
            //  set the pin value
            Serial.println("setting pin");
#endif

            //  select the pin
            int selectedPin = atoi (pin);
#if DEBUG
            Serial.println(selectedPin);
#endif

            //  set the pin for output
            pinMode(selectedPin, OUTPUT);

            //  determine digital or analog (PWM)
            if (strncmp(value, "HIGH", 4) == 0 || strncmp(value, "LOW", 3) == 0) {

#if DEBUG
              //  digital
              Serial.println("digital");
#endif

              if (strncmp(value, "HIGH", 4) == 0) {
#if DEBUG
                Serial.println("HIGH");
#endif
                digitalWrite(selectedPin, HIGH);
              }

              if (strncmp(value, "LOW", 3) == 0) {
#if DEBUG
                Serial.println("LOW");
#endif
                digitalWrite(selectedPin, LOW);
              }

            }
            else {

#if DEBUG
              //  analog
              Serial.println("analog");
#endif
              //  get numeric value
              int selectedValue = atoi(value);
#if DEBUG
              Serial.println(selectedValue);
#endif
              analogWrite(selectedPin, selectedValue);

            }

            //  return status
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Access-Control-Allow-Origin: *");
            client.println();

          }
          else {
#if DEBUG
            //  read the pin value
            Serial.println("reading pin");
#endif

            //  determine analog or digital
            if (pin[0] == 'a' || pin[0] == 'A') {

              //  analog
              int selectedPin = pin[1] - '0';

#if DEBUG
              Serial.println(selectedPin);
              Serial.println("analog");
#endif

              sprintf(outValue, "%d", analogRead(selectedPin));

#if DEBUG
              Serial.println(outValue);
#endif

            }
            else if (pin[0] != NULL) {

              //  digital
              int selectedPin = atoi (pin);
//            int selectedPin = pin[0] - '0';

#if DEBUG
              Serial.println(selectedPin);
              Serial.println("digital");
#endif

              pinMode(selectedPin, INPUT);

              int inValue = digitalRead(selectedPin);

              if (inValue == 0) {
                sprintf(outValue, "%s", "LOW");
                //sprintf(outValue,"%d",digitalRead(selectedPin));
              }

              if (inValue == 1) {
                sprintf(outValue, "%s", "HIGH");
              }

#if DEBUG
              Serial.println(outValue);
#endif
            }

            //  assemble the json output
            jsonOut += "{\"";
            jsonOut += pin;
            jsonOut += "\":\"";
            jsonOut += outValue;
            jsonOut += "\"}";

            //  return value with wildcarded Cross-origin policy
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Access-Control-Allow-Origin: *");
            client.println();
            client.println(jsonOut);
          }
        }
        else {

          //  error
#if DEBUG
          Serial.println("erroring");
#endif
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();

        }
        break;
      }
    }
    // give the web browser time to receive the data
    delay(10);

    // close the connection:
    client.stop();
      while(client.status() != 0){
      delay(5);
    }
    Serial.println("Client disconnected");
  }
}


void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in the browser
  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}
