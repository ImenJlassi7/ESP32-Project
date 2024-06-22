#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>
#include <SPIFFS.h>
#include <Firebase_ESP_Client.h>
//#include <addons/TokenHelper.h>

#define echoPin 2
#define trigPin 4
#define buzzerPin 5
#define led 16
const char *ssid = "Hahah";
const char *password = "imen0000";

// Configuration Firebase
#define FIREBASE_PROJECT_ID "fierstoreproject-8e4b5"
#define FIREBASE_API_KEY "AIzaSyAPZ07ilX3yovn392MehLQXSAshhhpONaw"
//#define FIREBASE_DATABASE_URL "https://fierstoreproject-8e4b5-default-rtdb.firebaseio.com"
#define USER_EMAIL "jlassiimen@gmail.com"
#define USER_PASSWORD "12345imen"
WebServer server(80);
DHT dht(26, DHT11);

// Objets de données Firebase
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth auth;

// Seuil de distance
float temperatureThreshold = 0;
float distanceThreshold = 0;
void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(led, OUTPUT);
    // Configuration SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Une erreur est survenue lors du montage de SPIFFS");
        return;
    }

    // Configuration Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connecté à ");
    Serial.println(ssid);
    Serial.print("Adresse IP : ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32")) {
        Serial.println("Répondeur MDNS démarré");
    }

    server.on("/", handleRoot);
    server.on("/threshold", HTTP_POST, handleThresholdForm);
    server.on("/distance", HTTP_POST, handleDistanceForm);  // Add this in setup()

    server.begin();
    Serial.println("Serveur HTTP démarré");

    // Configuration Firebase
    firebaseConfig.api_key = FIREBASE_API_KEY;
 auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
    Firebase.begin(&firebaseConfig, &auth);
     Firebase.reconnectWiFi(true);
}

void loop() {
    server.handleClient();
    delay(2);

    float distance = readDistance();
    float temperature = readDHTTemperature();
    float humidity = readDHTHumidity();
    String collectionPath  = "EspData";
    long timestamp = millis(); // Use current time as a unique identifier

    if (distance >= distanceThreshold) {
      digitalWrite(led, HIGH);
      
  } else{
        digitalWrite(led, LOW); // Buzzer désactivé
    }


    if(temperature>=temperatureThreshold){
      
      digitalWrite(buzzerPin, HIGH); // Buzzer activé
    } else{
        digitalWrite(buzzerPin, LOW); // Buzzer désactivé
        
    }

   
  

    // Envoyer les données à Firebase
    sendToFirebase(firebaseData,FIREBASE_PROJECT_ID,collectionPath ,temperature, humidity, distance,timestamp);

    delay(2000); // Éviter des mises à jour trop fréquentes
}

void sendToFirebase(FirebaseData &fbdo, const String &projectID, const String &collectionPath, float temperature, float humidity,float distance, long timestamp) {
 
  if (!isnan(temperature) && !isnan(humidity) && !isnan(distance) ) {
    FirebaseJson content;  // Initialize FirebaseJson object

    // Set the 'Temperature' and 'Humidity' fields
    content.set("fields/Temperature/doubleValue", temperature);
    content.set("fields/Humidity/doubleValue", humidity);
  content.set("fields/Distance/doubleValue", distance);
        String documentPath = collectionPath + "/" + String(timestamp); // Unique document path

  //  Serial.print("Update/Add DHT Data... ");

    // Use patchDocument to update Firestore document
    bool success = Firebase.Firestore.patchDocument(
      &fbdo,
      projectID,
      "",  // Not using a transaction ID
      documentPath.c_str(),
      content.raw(),
      ""  // No specific field mask, updates whole object
    );

    if (success) {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    } else {
      
Serial.print("Error: ");
Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("Failed to read DHT data.");
  }
}


void handleThresholdForm() {
    if (server.hasArg("threshold")) {
        temperatureThreshold = server.arg("threshold").toFloat();
    }
    server.send(200, "text/plain", "Seuil température mis à jour avec succès !");
}
void handleDistanceForm() {
    if (server.hasArg("distance")) {
        distanceThreshold = server.arg("distance").toFloat();
    }
    server.send(200, "text/plain", "Seuil distance mis à jour avec succès !");
}

float readDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    return duration / 58.2;
}

void handleRoot() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Erreur de serveur");
        return;
    }

    String html = file.readString();
    file.close();

    html.replace("{{temperature}}", String(readDHTTemperature()));
    html.replace("{{humidity}}", String(readDHTHumidity()));
    html.replace("{{distance}}", String(readDistance()));

    server.send(200, "text/html", html);
}

float readDHTTemperature() {
    float t = dht.readTemperature();
    return isnan(t) ? -1 : t;
}

float readDHTHumidity() {
    float h = dht.readHumidity();
    return isnan(h) ? -1 : h;
}
