#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <Wire.h>

AsyncWebServer server(80);

const char *ssid = "ESP32-Access-Point";
const char *password = "123456789";

const char *ssidhs = "ESP32-Access-Point";
const char *passwordhs = "123456789";

// Entradas Digitales
const int Entrada_configuracion = 12;

// Salidas Digitales
const int Salida[4] = {5, 4, 0, 2};

// Inicialización Variables
unsigned long horas = 3600000;
unsigned long minutos = 60000;

unsigned long temppruebas = 0;
unsigned long tempinicio = 0;
unsigned long tempfin = 0;
unsigned long tempciclo = 0;
int Valor_entrada_configuracion = 0;

int Estado[4] = {0, 0, 0, 0}; // 0: INICIANDO, 1: ACTIVO, 2: EN ESPERA, 3: PARADO
boolean start[4] = {false, false, false, false};
boolean stop[4] = {false, false, false, false};

unsigned long Tiempo_inicio = 0;          // minutos hasta primer inicio
unsigned long Duracion[4] = {0, 0, 0, 0}; // minutos activo
unsigned long Ciclo = 0;                  // horas entre inicios

String sc[4] = {"s1", "s2", "s3", "s4"};
String sctext[4] = {"s1", "s2", "s3", "s4"};

// Nombres equipos y señales
int numero_puerto_MQTT = 1883;
const char *nmqtt = "ptg43.mooo.com";

const char *nombre_dispositivo = "wemo_riego_1";

String nombre_estado = "Salida_1";
String nombre_completo_salida[4] = {"Salida_1", "Salida_2", "Salida_3", "Salida_4"};
String nombre_completo_entrada_configuracion = "Entrada_Configuracion";
String nombre_completo_tiempo_inicio = "Salida_1";
String nombre_completo_duracion[4] = {"Salida_1", "Salida_2", "Salida_3", "Salida_4"};
String nombre_completo_ciclo = "Salida_1";

boolean conf = false;

// Valores funcionamiento normal wifi y mqtt
WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];
int value = 0;

#include <servidor.txt>

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory())
    {
        Serial.println("- empty file or failed to open file");
        return String();
    }
    Serial.println("- read from file:");
    String fileContent;
    while (file.available())
    {
        fileContent += String((char)file.read());
    }
    file.close();
    Serial.println(fileContent);
    return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);
    File file = LittleFS.open(path, "w");
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
    file.close();
}

String processor(const String &var)
{
    Serial.print("var: ");
    Serial.println(var);

    if (var == "inputssid")
    {
        return readFile(LittleFS, "/inputssid.txt");
    }
    else if (var == "inputpassword")
    {
        return readFile(LittleFS, "/inputpassword.txt");
    }
    else if (var == "servidor_MQTT")
    {
        return readFile(LittleFS, "/servidor_MQTT.txt");
    }
    else if (var == "puerto_MQTT")
    {
        return readFile(LittleFS, "/puerto_MQTT.txt");
    }
    else if (var == "dispositivo")
    {
        return readFile(LittleFS, "/dispositivo.txt");
    }
    else if (var == "tiempo_inicio")
    {
        return readFile(LittleFS, "/tiempo_inicio.txt");
    }
    else if (var == "duracion_1")
    {
        return readFile(LittleFS, "/duracion_1.txt");
    }
    else if (var == "duracion_2")
    {
        return readFile(LittleFS, "/duracion_2.txt");
    }
    else if (var == "duracion_3")
    {
        return readFile(LittleFS, "/duracion_3.txt");
    }
    else if (var == "duracion_4")
    {
        return readFile(LittleFS, "/duracion_4.txt");
    }
    else if (var == "ciclo")
    {
        return readFile(LittleFS, "/ciclo.txt");
    }
    else if (var == "estado_señal_1")
    {
        if (digitalRead(Salida[0]))
        {
            return "On";
        }
        else
        {
            return "Off";
        }
    }
    else if (var == "estado_señal_2")
    {
        if (digitalRead(Salida[1]))
        {
            return "On";
        }
        else
        {
            return "Off";
        }
    }
    else if (var == "estado_señal_3")
    {
        if (digitalRead(Salida[3]))
        {
            return "On";
        }
        else
        {
            return "Off";
        }
    }
    else if (var == "estado_señal_4")
    {
        if (digitalRead(Salida[4]))
        {
            return "On";
        }
        else
        {
            return "Off";
        }
    }
    else if (var == "estado_wifi")
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return "Conectada";
        }
        else
        {
            return "Desconectada";
        }
    }
    else if (var == "estado_MQTT")
    {
        if (client.connected())
        {
            return "Conectado";
        }
        else
        {
            return "Desconectado";
        }
    }

    return String();
}

void setup_wifi()
{
    delay(10);

    Serial.println();
    Serial.print("Conectando a red: ");

    Serial.println(ssid);
    WiFi.hostname(nombre_dispositivo);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(250);
    }
    Serial.println("");
    Serial.println("WiFi Conectada");
    Serial.println("Direccion IP: ");
    Serial.println(WiFi.localIP());
}

void s1_on(int i)
{
    start[i] = true;
}

void s1_off(int i)
{
    stop[i] = true;
}

void s2(String j, String tiempo)
{
    int i = j.toInt();
    int duracion = tiempo.toInt();
    String nombre = "/duracion_" + j + ".txt";
    Duracion[i] = duracion * minutos;
    Serial.println("Duracion: " + Duracion[i]);
    writeFile(LittleFS, nombre.c_str(), tiempo.c_str());
    if (Duracion[i] == 0)
    {
        Estado[i] = 3;
    }
}

void s3(String tiempo)
{
    int ciclo = tiempo.toInt();
    Ciclo = ciclo * horas;
    Serial.println("Ciclo: " + Ciclo);
    writeFile(LittleFS, "/ciclo.txt", tiempo.c_str());
}

void s4(String tiempo)
{
    int tiempo_inicio = tiempo.toInt();
    Tiempo_inicio = tiempo_inicio * minutos;
    Serial.println("Tiempo inicio: " + Tiempo_inicio);
    writeFile(LittleFS, "/tiempo_inicio.txt", tiempo.c_str());

    for (int i = 0; i <= 3; i++)
    {
        Estado[i] = 0;
    }
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.println("");
    Serial.print("Mensaje recibido en topic: ");
    Serial.println(topic);
    Serial.println("Mensaje: ");

    String mensaje;

    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        mensaje += (char)message[i];
    }
    Serial.println();

    if (String(topic) == nombre_completo_salida[0])
    {
        if (mensaje == "on")
        {
            s1_on(0);
        }
        else if (mensaje == "off")
        {
            s1_off(0);
        }
    }

    if (String(topic) == nombre_completo_salida[1])
    {
        if (mensaje == "on")
        {
            s1_on(1);
        }
        else if (mensaje == "off")
        {
            s1_off(1);
        }
    }

    if (String(topic) == nombre_completo_salida[2])
    {
        if (mensaje == "on")
        {
            s1_on(2);
        }
        else if (mensaje == "off")
        {
            s1_off(2);
        }
    }

    if (String(topic) == nombre_completo_salida[3])
    {
        if (mensaje == "on")
        {
            s1_on(3);
        }
        else if (mensaje == "off")
        {
            s1_off(3);
        }
    }

    if (String(topic) == nombre_completo_duracion[0])
    {
        s2("0", mensaje);
    }

    if (String(topic) == nombre_completo_duracion[1])
    {
        s2("1", mensaje);
    }

    if (String(topic) == nombre_completo_duracion[2])
    {
        s2("2", mensaje);
    }

    if (String(topic) == nombre_completo_duracion[3])
    {
        s2("3", mensaje);
    }

    if (String(topic) == nombre_completo_ciclo)
    {
        s3(mensaje);
    }

    if (String(topic) == nombre_completo_tiempo_inicio)
    {
        s4(mensaje);
    }
}

void reconnect()
{
    // Loop until we're reconnected

    while (!client.connected())
    {
        Serial.print("Conectando a servidor MQTT...");
        // Attempt to connect
        String clientId = "WemoClient-";
        clientId += String(random(0xffff), HEX);

        if (client.connect(clientId.c_str()))
        {
            Serial.println("Conectado");
            client.publish("Puerta/Estado", "MQTT conectado");

            Serial.println("");
            Serial.println("Salidas");
            for (int i = 0; i <= 3; i++)
            {
                Serial.println(nombre_completo_salida[i]);
            }
            for (int i = 0; i <= 3; i++)
            {
                Serial.println(nombre_completo_duracion[i]);
            }
            Serial.println(nombre_completo_ciclo);
            Serial.println(nombre_completo_tiempo_inicio);
            Serial.println(nombre_estado);

            Serial.println("");

            for (int i = 0; i <= 3; i++)
            {
                client.subscribe(nombre_completo_salida[i].c_str(), 1);
            }
            for (int i = 0; i <= 3; i++)
            {
                client.subscribe(nombre_completo_duracion[i].c_str(), 1);
            }
            client.subscribe(nombre_completo_ciclo.c_str(), 1);
            client.subscribe(nombre_completo_tiempo_inicio.c_str(), 1);
        }
        else
        {
            Serial.print("Fallo, rc=");
            Serial.print(client.state());
            Serial.println(" reintantando en 5 segundos");
            delay(5000);
        }
    }
}

void servidorhttp()
{

    // Send web page with input fields to client
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      String inputMessage;
      // GET paramssid value on <ESP_IP>/get?inputssid=<inputMessage>
      if (request->hasParam("inputssid")) {
        inputMessage = request->getParam("inputssid")->value();
        writeFile(LittleFS, "/inputssid.txt", inputMessage.c_str());
      }
      // GET parampassword value on <ESP_IP>/get?inputpassword=<inputMessage>
      else if (request->hasParam("inputpassword")) {
        inputMessage = request->getParam("inputpassword")->value();
        writeFile(LittleFS, "/inputpassword.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("servidor_MQTT")) {
        inputMessage = request->getParam("servidor_MQTT")->value();
        writeFile(LittleFS, "/servidor_MQTT.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("puerto_MQTT")) {
        inputMessage = request->getParam("puerto_MQTT")->value();
        writeFile(LittleFS, "/puerto_MQTT.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("dispositivo")) {
        inputMessage = request->getParam("dispositivo")->value();
        writeFile(LittleFS, "/dispositivo.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("tiempo_inicio")) {
        inputMessage = request->getParam("tiempo_inicio")->value();
        writeFile(LittleFS, "/tiempo_inicio.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("duracion")) {
        inputMessage = request->getParam("duracion")->value();
        writeFile(LittleFS, "/duracion.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("ciclo")) {
        inputMessage = request->getParam("ciclo")->value();
        writeFile(LittleFS, "/ciclo.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("reiniciar")) {
        inputMessage = request->getParam("reiniciar")->value();
        if (inputMessage == "1") {
          ESP.restart();
        }
      } 
      else {
        inputMessage = "No message sent";
      }
      Serial.println(inputMessage);
      request->send(200, "text/text", inputMessage); });
    server.onNotFound(notFound);
    server.begin();
}

void setup()
{

    Serial.begin(115200);

    pinMode(Entrada_configuracion, INPUT);

    Valor_entrada_configuracion = digitalRead(Entrada_configuracion);

    // Initialize LittleFS
    if (!LittleFS.begin())
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    if (Valor_entrada_configuracion == HIGH)
    {
        conf = true;
    }
    else
    {
        conf = false;
    }

    if (conf == true)
    {
        // Inicializa Hotspot
        for (int i = 0; i <= 3; i++)
        {
            pinMode(Salida[i], OUTPUT);
            digitalWrite(Salida[i], LOW);
        }

        WiFi.softAP(ssidhs, passwordhs);
        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP);

        String ssida = readFile(LittleFS, "/inputssid.txt");
        String passworda = readFile(LittleFS, "/inputpassword.txt");
        ssid = ssida.c_str();
        password = passworda.c_str();
        setup_wifi();
        servidorhttp();
    }

    else
    {
        String ssida = readFile(LittleFS, "/inputssid.txt");
        String passworda = readFile(LittleFS, "/inputpassword.txt");
        String servidor_MQTTa = readFile(LittleFS, "/servidor_MQTT.txt");
        String puerto_MQTTa = readFile(LittleFS, "/puerto_MQTT.txt");
        String dispositivoa = readFile(LittleFS, "/dispositivo.txt");
        String tiempo_inicioa = readFile(LittleFS, "/tiempo_inicio.txt");
        String duraciona[4] = {"a", "d", "d", "f"};
        for (int i = 0; i <= 3; i++)
        {
            String nombre = "/duracion_" + String(i) + ".txt";
            duraciona[i] = readFile(LittleFS, nombre.c_str());
        }

        String cicloa = readFile(LittleFS, "/ciclo.txt");
        Tiempo_inicio = atol(tiempo_inicioa.c_str());

        for (int i = 0; i <= 3; i++)
        {
            Duracion[i] = atol(duraciona[i].c_str());
            Duracion[i] = Duracion[i] * minutos;
            if (Duracion[i] == 0)
            {
                Estado[i] = 3;
            }
        }
        Ciclo = atol(cicloa.c_str());
        Tiempo_inicio = Tiempo_inicio * minutos;
        Ciclo = Ciclo * horas;

        ssid = ssida.c_str();
        password = passworda.c_str();
        int str_len = servidor_MQTTa.length() + 1;
        char n1[str_len];
        servidor_MQTTa.toCharArray(n1, str_len);

        Serial.print("n1: ");
        Serial.println(n1);

        numero_puerto_MQTT = puerto_MQTTa.toInt();
        nombre_dispositivo = dispositivoa.c_str();

        setup_wifi();

        nombre_estado = String(nombre_dispositivo) + "/" + "estado";

        for (int i = 0; i <= 3; i++)
        {
            nombre_completo_salida[i] = String(nombre_dispositivo) + "/" + "salida_" + String(i);
            nombre_completo_duracion[i] = String(nombre_dispositivo) + "/" + "duracion_" + String(i);
            sc[i] = nombre_completo_salida[i] + "c";

            pinMode(Salida[i], OUTPUT);
            digitalWrite(Salida[i], LOW);
        }

        nombre_completo_tiempo_inicio = String(nombre_dispositivo) + "/" + "tiempo_inicio";
        nombre_completo_ciclo = String(nombre_dispositivo) + "/" + "ciclo";

        client.setServer(nmqtt, numero_puerto_MQTT);
        client.setCallback(callback);

        pinMode(Entrada_configuracion, INPUT_PULLUP);

        tempinicio = millis();

        servidorhttp();
    }
}

void loop()
{
    Valor_entrada_configuracion = digitalRead(Entrada_configuracion);

    if (Valor_entrada_configuracion == true)
    {
        Serial.println("En Configuración");
        delay(1000);
    }
    else
    {

        if (!client.connected())
        {
            reconnect();
        }
        client.loop();

        unsigned long now = millis();

        if (now - temppruebas > 120000)
        {
            temppruebas = now;
            Serial.println(nombre_completo_duracion);
            Serial.println(Duracion);
            Serial.println(nombre_completo_ciclo);
            Serial.println(Ciclo);
            Serial.println(nombre_completo_tiempo_inicio);
            Serial.println(Tiempo_inicio);
            Serial.println(nombre_estado);
            Serial.println(Estado);

            unsigned long t1 = Tiempo_inicio / minutos;
            unsigned long t2 = Duracion / minutos;
            unsigned long t3 = Ciclo / horas;

            String a = (String)Estado;
            String b = (String)t1;
            String c = (String)t2;
            String d = (String)t3;

            String sa = nombre_estado;
            String sb = nombre_completo_tiempo_inicio + "c";
            String sc = nombre_completo_duracion + "c";
            String sd = nombre_completo_ciclo + "c";

            client.publish(sa.c_str(), a.c_str());
            client.publish(sb.c_str(), b.c_str());
            client.publish(sc.c_str(), c.c_str());
            client.publish(sd.c_str(), d.c_str());
            client.publish(s1c.c_str(), s1ctext.c_str());
        }

        switch (Estado)
        {
        case 0:
            if (now - tempinicio > Tiempo_inicio)
            {
                start = true;
                tempfin = now;
                tempciclo = now;
            }

            break;
        case 1:
            if (now - tempfin > Duracion)
            {
                stop = true;
            }
            break;
        case 2:
            if (now - tempciclo > Ciclo)
            {
                start = true;
            }
            break;
        case 3:
            break;
        }

        if (start)
        {
            tempfin = now;
            tempciclo = now;
            start = false;
            if (Estado != 3)
            {
                Estado = 1;
            }
            Serial.println("Cambiando " + nombre_completo_salida_1 + " a: on");
            digitalWrite(Salida_1, HIGH);
            s1ctext = "on";
            client.publish(s1c.c_str(), s1ctext.c_str());
        }

        if (stop)
        {
            stop = false;
            if (Estado != 3)
            {
                Estado = 2;
            }
            Serial.println("Cambiando " + nombre_completo_salida_1 + " a: off");
            digitalWrite(Salida_1, LOW);
            s1ctext = "off";

            client.publish(s1c.c_str(), s1ctext.c_str());
        }
    }
}