#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

DHT dht(D3, DHT22);

const int PIRPin = D4;

const int muxSIG = A0;
const int muxS0 = D8;
const int muxS1 = D7;
const int muxS2 = D6;
const int muxS3 = D5;

//Ajustes para MQ-135
const int RL_VALUE = 20; // Resistencia RL del modulo en Kilo ohms
const int R0 = 40;       // Resistencia R0 del sensor en Kilo ohms

// Datos para lectura multiple
const int READ_SAMPLE_INTERVAL = 100; // Tiempo entre muestras
const int READ_SAMPLE_TIMES = 5;      // Numero muestras

// Ajustar estos valores para vuestro sensor según el Datasheet
// (opcionalmente, según la calibración que hayáis realizado)
const float X0 = 10;
const float Y0 = 1.2;
const float X1 = 200;
const float Y1 = 0.6;

// Puntos de la curva de concentración {X, Y}
const float punto0[] = {log10(X0), log10(Y0)};
const float punto1[] = {log10(X1), log10(Y1)};

// Calcular pendiente y coordenada abscisas
const float scope = (punto1[1] - punto0[1]) / (punto1[0] - punto0[0]);
const float coord = punto0[1] - punto0[0] * scope;

//MULTISENSOR1
//const String URL = "/zipato-web/remoting/attribute/set?serial=ZTC3C2D55612AC3238&apiKey=bf159956-0bdf-4d19-bf22-78e0b164cfbc&ep=acdf3066-d9a3-49b0-938c-df52334ba6d7&";
//MULTISENSOR2
const String URL = "/zipato-web/remoting/attribute/set?serial=ZTC3C2D55612AC3238&apiKey=b0a0d214-3167-4748-be7c-3eda0253cca0&ep=de9a94f3-8cbb-4d87-b4e4-a0243b57c61b&";
//MULTISENSOR3
//const String URL = "/zipato-web/remoting/attribute/set?serial=ZTC3C2D55612AC3238&apiKey=abd05390-7703-4327-9edd-0cac4abb8d20&ep=1018dc46-cb88-412d-bcbb-ebdb8a6945cf";
const char *host = "my.zipato.com";
const char *ssid = "HALO_2003";
const char *password = "Mi_Abuelito_Tenia_Un_Reloj_De_Pared";
const int httpsPort = 443;

void EscribeLog(String txt)
{
  Serial.print(millis());
  Serial.print(": -> ");
  Serial.println(txt);
}

String strMejorWiFi()
{
  //Busca la mejor red WiFi de casa para conectarse. Todas ellas deben tener la misma pwd.

  int maxRSSI = -100;
  String maxSSID = "";
  EscribeLog("-------->Entering into strMejorWiFi()");

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  EscribeLog("WiFi.mode(WIFI_STA)");
  // WiFi.disconnect();
  delay(100);
  EscribeLog("Scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  EscribeLog("Scan done");
  if (n == 0)
  {
    EscribeLog("NO networks found");
    return ("");
  }
  else
  {
    Serial.print(n);
    Serial.println("Networks found");
    for (int i = 0; i < n; ++i)
    {
      if (WiFi.SSID(i).startsWith("HALO_2003") && WiFi.RSSI(i) > maxRSSI)
      {
        maxSSID = WiFi.SSID(i);nn
        maxRSSI = WiFi.RSSI(i);
        EscribeLog("Wifi encontrada: " + maxSSID + ": " + maxRSSI);
        delay(10);
      }
    }
  }
  EscribeLog("Wifi elegida: " + maxSSID + ": " + maxRSSI);
  EscribeLog("<--------Comminng out from strMejorWiFi()");
  return (maxSSID);
}

void reconnect(char *ssid, char *password)
{
  EscribeLog("-------->Entering into reconnect()");

  unsigned long timeOut = millis();
  char *ssid = "";
  String maxSSID = "";
  while (maxSSID == "")
  {
    maxSSID = strMejorWiFi();
    delay(500);
  }

  maxSSID.toCharArray(ssid, 50);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED || millis() > timeOut + 30000)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  EscribeLog("WiFi connected: " + maxSSID + ". IP Address: " + WiFi.localIP());
  EscribeLog(maxSSID);
   EscribeLog("<--------Comminng out from reconnect()");
}

void getHttps(int atributo, double valor)
{
  /* Hace la llamada HTTPS */
  // Use WiFiClientSecure class to create TLS connection
  EscribeLog("-------->Entering into getHttps()");

  WiFiClientSecure client;
  String myURL = URL + "value" + atributo + "=" + valor;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort))
  {
    Serial.println("connection failed");
    return;
  }

  Serial.print("requesting URL: ");
  Serial.println(myURL);

  client.print(String("GET ") + myURL + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\""))
  {
    Serial.println("esp8266/Arduino CI successfull!");
  }
  else
  {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
  client.stop();
  EscribeLog("<--------Comminng out from getHttps()");
}

void SetMuxChannel(byte channel)
{

  digitalWrite(muxS0, bitRead(channel, 0));
  digitalWrite(muxS1, bitRead(channel, 1));
  digitalWrite(muxS2, bitRead(channel, 2));
  digitalWrite(muxS3, bitRead(channel, 3));
}

void setup()
{
  Serial.begin(9600);

  dht.begin();

  pinMode(PIRPin, INPUT);

  pinMode(muxS0, OUTPUT);
  pinMode(muxS1, OUTPUT);
  pinMode(muxS2, OUTPUT);
  pinMode(muxS3, OUTPUT);
}

void loop()
{

  while (WiFi.status() != WL_CONNECTED)
  {
    reconnect();
    Serial.println(WiFi.status());
    if (WiFi.status() == WL_CONNECTED)
    {
      getHttps(16, 1);
      getHttps(16, 0);
      EscribeLog("Sending message to Zipato of Start Device.");
    }
    delay(100);
  };
  //Lectura de Humedad
  float humedad = dht.readHumidity();

  //Lectura de Temperatura
  float temperatura = dht.readTemperature();

  //Lectura de Presencia
  int presencia = digitalRead(PIRPin); //Devuelve HIGH o LOW

  //Lectura de calidad de aire
  SetMuxChannel(13);
  //  float rs_med = readMQ(muxSIG);      // Obtener la Rs promedio
  //  float concentration = getConcentration(rs_med / R0); // Obtener la concentración
  long concentration = 0;

  for (double i = 0; i < 10000; i++)
  {
    concentration += analogRead(A0);
  }
  concentration /= 10000;
  concentration = map(concentration, 0, 1023, 0, 9000);

  //Lectura de Luminosidad
  SetMuxChannel(14);
  int luminosidad = analogRead(muxSIG);

  //Lectura de Potencia
  SetMuxChannel(15);
  double potencia = 0;
  for (int i = 0; i < 1000; i++)
  {
    potencia += abs(analogRead(muxSIG) - 512);
  }
  potencia /= 1000;
  /*
  getHttps(1, temperatura);
  getHttps(2, humedad);
  getHttps(5, presencia);
  getHttps(6, luminosidad);
  getHttps(7, potencia);
  getHttps(8, concentration);
*/
  getHttps(1, 36.5 + random(1, 3));
  getHttps(2, 75.40 + random(1, 3));
  getHttps(5, 1);
  getHttps(6, 28 + random(1, 3));
  getHttps(7, 185 + random(1, 3));
  getHttps(8, 875 + random(1, 3));

  Serial.print("humedad: ");
  Serial.print(humedad);
  Serial.print(", temperatura: ");
  Serial.print(temperatura);
  Serial.print(", presencia: ");
  if (presencia == HIGH)
  {
    Serial.print("Sí");
  }
  else
  {
    Serial.print("No");
  }
  Serial.print(", concentration: ");
  Serial.println(concentration);
  Serial.print(", luminosidad: ");
  Serial.print(luminosidad);
  Serial.print(", potencia: ");
  Serial.println(potencia);
}
// Obtener resistencia a partir de la lectura analogica
float getMQResistance(int raw_adc)
{
  return (((float)RL_VALUE / 1000.0 * (1023 - raw_adc) / raw_adc));
}
// Obtener la resistencia promedio en N muestras
float readMQ(int mq_pin)
{
  float rs = 0;
  for (int i = 0; i < READ_SAMPLE_TIMES; i++)
  {
    rs += getMQResistance(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  return rs / READ_SAMPLE_TIMES;
}

// Obtener concentracion 10^(coord + scope * log (rs/r0)
float getConcentration(float rs_ro_ratio)
{
  return pow(10, coord + scope * log(rs_ro_ratio));
}
