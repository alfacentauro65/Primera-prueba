#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

/*
  Remapeo de pines del ESP8266
*/
const byte DP0 = 16;
const byte DP1 = 5;
const byte DP2 = 4;
const byte DP3 = 0;
const byte DP4 = 2; //BUILTIN_LED
const byte DP5 = 14;
const byte DP6 = 12;
const byte DP7 = 13;
const byte DP8 = 15;

/*
  Conexión eléctrica ESP8266-HC4067
  ESP8266 HC4067
  ------- ------
      DP8-S0 
      DP7-S1 
      DP6-S2 
      DP5-S3 
      A0 -SIG

  *Nota.- No usar pines de entrada del multiplexor adyacentes, ya que 
          se producen interacciones. Utilizar por ejemplo: 0, 3, 6, 9, 12, 15
          para seis entradas analógicas.
*/
const int muxSIG = A0;
const int muxS0 = DP8;
const int muxS1 = DP7;
const int muxS2 = DP6;
const int muxS3 = DP5;

DHT dht(DP3, DHT22);
const int PIRPin = DP4;

//Ajustes para fotoresistencia LDR (GL55)
const long A = 1000; //Resistencia en oscuridad en KΩ
const int B = 15;    //Resistencia a la luz (10 Lux) en KΩ
const int Rc = 10;   //Resistencia calibracion en KΩ

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
const String URL = "/zipato-web/remoting/attribute/set?serial=ZTC3C2D55612AC3238&apiKey=bf159956-0bdf-4d19-bf22-78e0b164cfbc&ep=acdf3066-d9a3-49b0-938c-df52334ba6d7&";
//MULTISENSOR2
//const String URL = "/zipato-web/remoting/attribute/set?serial=ZTC3C2D55612AC3238&apiKey=b0a0d214-3167-4748-be7c-3eda0253cca0&ep=de9a94f3-8cbb-4d87-b4e4-a0243b57c61b&";
//MULTISENSOR3
//const String URL = "/zipato-web/remoting/attribute/set?serial=ZTC3C2D55612AC3238&apiKey=abd05390-7703-4327-9edd-0cac4abb8d20&ep=1018dc46-cb88-412d-bcbb-ebdb8a6945cf";
const char *host = "my.zipato.com";
const char *ssid = "HALO_2003";
const char *password = "Mi_Abuelito_Tenia_Un_Reloj_De_Pared";
const int httpsPort = 443;

//Tiempo entre envios a Zipato
unsigned long timeBetweenSends = 0;
unsigned long miliSegsBetweenSends = 30000;

void EscribeLog(String txt)
{
  unsigned long momento = millis();
  String millis = String("00" + String(momento % 1000));
  millis = millis.substring(millis.length() - 3, millis.length());
  Serial.print(String(((int)momento / 1000)) + "." + millis);
  Serial.print(": -> ");
  txt.replace("\n", "\n           ");
  Serial.println(txt);
}

String strMejorWiFi()
{
  //Busca la mejor red WiFi de casa para conectarse. Todas ellas deben tener la misma pwd.

  int maxRSSI = -100;
  String maxSSID = "";
  unsigned long duration = millis();
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
    EscribeLog(String(n) + " nNetworks found");
    for (int i = 0; i < n; ++i)
    {
      if (WiFi.SSID(i).startsWith(String(ssid)) && WiFi.RSSI(i) > maxRSSI)
      {
        maxSSID = WiFi.SSID(i);
        maxRSSI = WiFi.RSSI(i);
        EscribeLog("Wifi encontrada: " + maxSSID + ": " + maxRSSI);
        delay(10);
      }
    }
  }
  EscribeLog("Wifi elegida: " + maxSSID + ": " + maxRSSI);
  EscribeLog("<--------Comminng out from strMejorWiFi() in: " + String(millis() - duration) + " milisegundos");
  return (maxSSID);
}

void reconnect()
{
  unsigned long duration = millis();
  EscribeLog("-------->Entering into reconnect()");

  unsigned long timeOut = millis();
  String maxSSID = "";
  char *ssid = "";
  while (maxSSID == "")
  {
    maxSSID = strMejorWiFi();
    delay(500);
  }

  maxSSID.toCharArray(ssid, 50);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && millis() < timeOut + 30000)
  {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    EscribeLog("WiFi connected: " + maxSSID + ". IP Address: " + WiFi.localIP().toString());
    EscribeLog(maxSSID);
  }
  else
  {
    EscribeLog("Error: WiFi not connected!");
  }

  EscribeLog("<--------Comminng out from reconnect() in: " + String(millis() - duration) + " milisegundos");
}

void getHttps(int atributo, double valor)
{
  /* Hace la llamada HTTPS */
  // Use WiFiClientSecure class to create TLS connection
  unsigned long duration = millis();
  EscribeLog("-------->Entering into getHttps()");

  WiFiClientSecure client;
  String myURL = URL + "value" + atributo + "=" + valor;
  EscribeLog("Connecting to: " + String(host));

  if (!client.connect(host, httpsPort))
  {
    EscribeLog("Connection failed!");
    return;
  }

  EscribeLog((String("GET ") + myURL + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "User-Agent: BuildFailureDetectorESP8266\r\n" +
              "Connection: close\r\n\r\n"));

  client.print(String("GET ") + myURL + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  EscribeLog("Request sent ...");

  // Esperamos por la respuesta si la queremos para algo
  /*
  String response = "";
  while (client.connected())
  {
    //MUY IMPORTANTE: Usar lo menos posible client.readString() Parece que repite la solicitud de request.
    response = client.readString();
    if (response.length() > 0)
    {
      EscribeLog("Headers received");
      if (response.indexOf(" 200 OK") > 0)
      {
        EscribeLog(".....OK......");
      }
      else
      {
        EscribeLog("...FAILLURE...");
      }
      break;
    }
  }

  EscribeLog("Response: " + response);
*/
  client.stop();
  EscribeLog("<--------Comminng out from getHttps() in: " + String(millis() - duration) + " milisegundos");
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
  //Control de la conectividad
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

  //Retardo entre envios
  while (millis() < timeBetweenSends + miliSegsBetweenSends)
  {
    delay(100);
  }
  timeBetweenSends = millis();

  //Lectura de Humedad
  float humedad = dht.readHumidity();

  //Lectura de Temperatura
  float temperatura = dht.readTemperature();

  //Lectura de Presencia
  int presencia = digitalRead(PIRPin); //Devuelve HIGH o LOW

  //Lectura de calidad de aire
  SetMuxChannel(0);
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
  SetMuxChannel(7);
  int V = analogRead(muxSIG);
  int luminosidad = ((long)V * A * 10) / ((long)B * Rc * (1024 - V)); //usar si LDR entre A0 y Vcc (como en el esquema anterior)

  //Lectura de Potencia
  SetMuxChannel(15);
  double potencia = 0;
  for (int i = 0; i < 1000; i++)
  {
    potencia += abs(analogRead(muxSIG) - 512);
  }
  potencia /= 1000;

  getHttps(1, temperatura);
  getHttps(2, humedad);
  getHttps(5, presencia);
  getHttps(6, luminosidad);
  getHttps(7, potencia);
  getHttps(8, concentration);

  EscribeLog("1 Temperatura: " + String(temperatura));
  EscribeLog("2 Humedad: " + String(humedad));
  //EscribeLog("3 Lluvia: " + String(humedad));
  //EscribeLog("4 Tension Bateria: " + String(humedad));
  if (presencia == HIGH)
  {
    EscribeLog("5 Presencia: Si");
  }
  else
  {
    EscribeLog("5 Presencia: No");
  }
  EscribeLog("6 Luminosidad: " + String(luminosidad));
  EscribeLog("7 Potencia: " + String(potencia));
  EscribeLog("8 PPM CO2: " + String(concentration));
  //EscribeLog("9 Nada: " + String(humedad));
  //EscribeLog("10 Nada: " + String(humedad));
  //EscribeLog("11 Nada: " + String(humedad));
  //EscribeLog("12 Nada: " + String(humedad));
  //EscribeLog("13 Nada: " + String(humedad));
  //EscribeLog("14 Nada: " + String(humedad));
  //EscribeLog("15 Nada: " + String(humedad));
  //EscribeLog("16 Arranque: 0" + String(humedad));
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
