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
const int Entrada_configuracion = 14;
int Valor_entrada_configuracion = 0;

// Salidas Digitales
const int Salida[4] = {4, 5, 13, 12};

enum Estados {
  Reposo,
  Riego_1,
  Riego_2,
  Riego_3,
  Riego_4,
  Esperando_inicio,
  Esperando_ciclo,
  Ciclo_riego_1,
  Ciclo_riego_2,
  Ciclo_riego_3,
  Ciclo_riego_4,
  Configuracion
};

Estados estado = Estados::Reposo;

// Inicialización Variables
unsigned long horas = 3600000;
unsigned long minutos = 60000;

//unsigned long horas = 60000;
//unsigned long minutos = 1000;

//Temporizadores
unsigned long now = 0;
unsigned long temp_comms = 0;
unsigned long temp_inicio = 0;
unsigned long temp_ciclo = 0;
unsigned long temp_estado = 0;

//Tiempos
unsigned long Tiempo_inicio = 0;          // minutos hasta primer inicio
unsigned long Duracion[4] = {0, 0, 0, 0}; // minutos activo
unsigned long Ciclo = 0;                  // horas entre inicios

//Ordenes
boolean start[4] = {false, false, false, false};
boolean stop = false;
boolean start_ciclo = false;
boolean primera_vez=false;
//Varios
String s1c[4] = {"s1", "s2", "s3", "s4"};
String s1ctext[4] = {"off", "off", "off", "off"};

// Nombres equipos y señales
int numero_puerto_MQTT = 1883;
const char *nmqtt = "ptg43.mooo.com";
const char *nombre_dispositivo = "wemo_riego_1";
String nombre_estado = "Salida_1";
String nombre_completo_salida[4] = {"Salida_1", "Salida_2", "Salida_3", "Salida_4"};
String nombre_completo_start = "Entrada_Configuracion";
String nombre_completo_entrada_configuracion = "Entrada_Configuracion";
String nombre_completo_tiempo_inicio = "Salida_1";
String nombre_completo_duracion[4] = {"Salida_1", "Salida_2", "Salida_3", "Salida_4"};
String nombre_completo_ciclo = "Salida_1";
String nombre_completo_stop ="Stop";
boolean conf = false;

// Valores funcionamiento normal wifi y mqtt
WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];
int value = 0;

#include <servidor.txt>

void proximoEstado() 
{
  
  switch (estado) {
    case Estados::Reposo:
        if      (start[0]) {estado=Estados::Riego_1; primera_vez=true;start[0]=false;}
        else if (start[1]) {estado=Estados::Riego_2; primera_vez=true;start[1]=false;}
        else if (start[2]) {estado=Estados::Riego_3; primera_vez=true;start[2]=false;}
        else if (start[3]) {estado=Estados::Riego_4; primera_vez=true;start[3]=false;}
        else if (stop) {estado=Estados::Reposo; stop=false;}
        else if (start_ciclo) {estado=Estados::Ciclo_riego_1; primera_vez=true;}
        else if (Ciclo>0) {estado=Estados::Esperando_inicio; primera_vez=true;}
    break;
    case Estados::Riego_1:
        if (Ciclo>0) {estado=Estados::Esperando_ciclo; primera_vez=true;stop=false;}
        else {estado=Estados::Reposo; primera_vez=true;stop=false;}
    break;
    case Estados::Riego_2:
        if (Ciclo>0) {estado=Estados::Esperando_ciclo; primera_vez=true;stop=false;}
        else {estado=Estados::Reposo; primera_vez=true;stop=false;}
    break;
    case Estados::Riego_3:
        if (Ciclo>0) {estado=Estados::Esperando_ciclo; primera_vez=true;stop=false;}
        else {estado=Estados::Reposo; primera_vez=true;stop=false;}
    break;
    case Estados::Riego_4:
        if (Ciclo>0) {estado=Estados::Esperando_ciclo; primera_vez=true;stop=false;}
        else {estado=Estados::Reposo; primera_vez=true;stop=false;}
    break;
    case Estados::Esperando_inicio:
        if      (start[0]) {estado=Estados::Riego_1; primera_vez=true;start[0]=false;}
        else if (start[1]) {estado=Estados::Riego_2; primera_vez=true;start[1]=false;}
        else if (start[2]) {estado=Estados::Riego_3; primera_vez=true;start[2]=false;}
        else if (start[3]) {estado=Estados::Riego_4; primera_vez=true;start[3]=false;}
        else if (stop) {estado=Estados::Esperando_inicio; stop=false;}
        else if (start_ciclo) {estado=Estados::Ciclo_riego_1; primera_vez=true;}
        else if (Ciclo>0) {estado=Estados::Ciclo_riego_1; primera_vez=true;}
        else {estado=Estados::Reposo; primera_vez=true;}
    break;
    case Estados::Ciclo_riego_1:
        if (start_ciclo) {estado=Estados::Ciclo_riego_2; primera_vez=true;}
        else if (Ciclo>0) {
            if (stop) {estado=Estados::Esperando_ciclo; primera_vez=true;stop=false;}
            else      {estado=Estados::Ciclo_riego_2; primera_vez=true;}
        }
        else {estado=Estados::Reposo; primera_vez=true;}
    break;  
    case Estados::Ciclo_riego_2:
        if (start_ciclo) {estado=Estados::Ciclo_riego_3; primera_vez=true;}
        else if (Ciclo>0) {
            if (stop) {estado=Estados::Esperando_ciclo; primera_vez=true;stop=false;}
            else      {estado=Estados::Ciclo_riego_3; primera_vez=true;}
        }
        else {estado=Estados::Reposo; primera_vez=true;}
    break;  
    case Estados::Ciclo_riego_3:
        if (start_ciclo) {estado=Estados::Ciclo_riego_4; primera_vez=true;}
        else if (Ciclo>0) {
            if (stop) {estado=Estados::Esperando_ciclo; primera_vez=true;stop=false;}
            else      {estado=Estados::Ciclo_riego_4; primera_vez=true;}
        }
        else {estado=Estados::Reposo; primera_vez=true;}
    break;  
    case Estados::Ciclo_riego_4:
        if (start_ciclo & (Ciclo>0)) {estado=Estados::Esperando_ciclo; primera_vez=true;start_ciclo=false;}
        else if (start_ciclo & (Ciclo==0)) {estado=Estados::Reposo; primera_vez=true;start_ciclo=false;}        
        else if (Ciclo>0) {
            if (stop) {estado=Estados::Esperando_ciclo; primera_vez=true;stop=false;}
            else      {estado=Estados::Esperando_ciclo; primera_vez=true;}
        }
        else {estado=Estados::Reposo; primera_vez=true;}
    break;  
    case Estados::Esperando_ciclo:
        if      (start[0]) {estado=Estados::Riego_1; primera_vez=true;start[0]=false;}
        else if (start[1]) {estado=Estados::Riego_2; primera_vez=true;start[1]=false;}
        else if (start[2]) {estado=Estados::Riego_3; primera_vez=true;start[2]=false;}
        else if (start[3]) {estado=Estados::Riego_4; primera_vez=true;start[3]=false;}
        else if (stop) {estado=Estados::Esperando_ciclo; stop=false;}
        else if (start_ciclo) {estado=Estados::Ciclo_riego_1; primera_vez=true;}
        else if (Ciclo>0) {estado=Estados::Ciclo_riego_1; primera_vez=true;}
        else {estado=Estados::Reposo; primera_vez=true;}
    break;
    case Estados::Configuracion:

    break;            
    }
}

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
        return readFile(LittleFS, "/duracion_0.txt");
    }
    else if (var == "duracion_2")
    {
        return readFile(LittleFS, "/duracion_1.txt");
    }
    else if (var == "duracion_3")
    {
        return readFile(LittleFS, "/duracion_2.txt");
    }
    else if (var == "duracion_4")
    {
        return readFile(LittleFS, "/duracion_3.txt");
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
        if (digitalRead(Salida[2]))
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
        if (digitalRead(Salida[3]))
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
    start[i]=true;
    proximoEstado();
}

void s1_off()
{
    start_ciclo=false;
    stop=true;
    proximoEstado();    
}

void s2(String j, String tiempo)
{
    int i = j.toInt();
    int duracion = tiempo.toInt();
    String nombre = "/duracion_" + j + ".txt";
    Duracion[i] = duracion * minutos;
    Serial.println("Duracion: " + Duracion[i]);
    writeFile(LittleFS, nombre.c_str(), tiempo.c_str());
}

void s3(String tiempo)
{
    int ciclo = tiempo.toInt();
    Ciclo = ciclo * horas;
    Serial.println("Ciclo: " + Ciclo);
    writeFile(LittleFS, "/ciclo.txt", tiempo.c_str());
    proximoEstado();
}

void s4(String tiempo)
{
    int tiempo_inicio = tiempo.toInt();
    Tiempo_inicio = tiempo_inicio * minutos;
    Serial.println("Tiempo inicio: " + Tiempo_inicio);
    writeFile(LittleFS, "/tiempo_inicio.txt", tiempo.c_str());
    temp_inicio = millis();
    proximoEstado();
}

void connect(const String& host, int port) 
{ 
    static char pHost[64] = {0}; 
    strcpy(pHost, host.c_str()); 
    client.setServer(pHost, port); 
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

    if (String(topic) == nombre_completo_stop)
    {
        if (mensaje == "on")
        {
            s1_off();
        }

        String stopc = nombre_completo_stop + "c";
        client.publish(stopc.c_str(), "off");
    }
    
    if (String(topic) == nombre_completo_start)
    {
        if (mensaje == "on")
        {
            stop=false;
            start_ciclo=true;
            proximoEstado();
        }
        String startc = nombre_completo_start + "c";
        client.publish(startc.c_str(), "off");
    }

    if (String(topic) == nombre_completo_salida[0])
    {
        if (mensaje == "on")
        {
            s1_on(0);
        }
        else if (mensaje == "off")
        {
            s1_off();
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
            s1_off();
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
            s1_off();
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
            s1_off();
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
            client.publish("Riego_1/Estado", "MQTT conectado");

            Serial.println("");
            Serial.println("Salidas");
            for (int i = 0; i <= 3; i++)
            {
                Serial.println(nombre_completo_salida[i]);
                Serial.println(nombre_completo_duracion[i]);
            }
            Serial.println(nombre_estado);

            Serial.println(nombre_completo_start);
            Serial.println(nombre_completo_ciclo);
            Serial.println(nombre_completo_tiempo_inicio);
            Serial.println(nombre_completo_stop);

            Serial.println("");

            for (int i = 0; i <= 3; i++)
            {
                client.subscribe(nombre_completo_salida[i].c_str(), 1);
                client.subscribe(nombre_completo_duracion[i].c_str(), 1);
            }
            client.subscribe(nombre_completo_start.c_str(), 1);
            client.subscribe(nombre_completo_ciclo.c_str(), 1);
            client.subscribe(nombre_completo_tiempo_inicio.c_str(), 1);
            client.subscribe(nombre_completo_stop.c_str(), 1);
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
        Tiempo_inicio=inputMessage.toInt()*minutos;
        writeFile(LittleFS, "/tiempo_inicio.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("duracion_1")) {
        inputMessage = request->getParam("duracion_1")->value();
        Duracion[0]=inputMessage.toInt()*minutos;
        writeFile(LittleFS, "/duracion_0.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("duracion_2")) {
        inputMessage = request->getParam("duracion_2")->value();
        Duracion[1]=inputMessage.toInt()*minutos;
        writeFile(LittleFS, "/duracion_1.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("duracion_3")) {
        inputMessage = request->getParam("duracion_3")->value();
        Duracion[2]=inputMessage.toInt()*minutos;
        writeFile(LittleFS, "/duracion_2.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("duracion_4")) {
        inputMessage = request->getParam("duracion_4")->value();
        Duracion[3]=inputMessage.toInt()*minutos;
        writeFile(LittleFS, "/duracion_3.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("ciclo")) {
        inputMessage = request->getParam("ciclo")->value();
        Ciclo=inputMessage.toInt()*horas;
        writeFile(LittleFS, "/ciclo.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("reiniciar")) {
        inputMessage = request->getParam("reiniciar")->value();
        if (inputMessage == "1") {
          ESP.restart();
        }
      } 
      else if (request->hasParam("stop")) {
        inputMessage = request->getParam("stop")->value();
        if (inputMessage == "1") {
          stop=true;
          proximoEstado();
        }
      }
      else if (request->hasParam("start")) {
        inputMessage = request->getParam("start")->value();
        if (inputMessage == "1") {
            start_ciclo=true;
            proximoEstado();
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

    pinMode(Entrada_configuracion, INPUT_PULLUP);

    Valor_entrada_configuracion = digitalRead(Entrada_configuracion);
    Serial.print("Estado configuracion: ");
    Serial.print(Valor_entrada_configuracion);

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
        estado=Estados::Configuracion;
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

        for (int i = 0; i <= 3; i++)
        {
            nombre_completo_salida[i] = String(nombre_dispositivo) + "/" + "salida_" + String(i);
            nombre_completo_duracion[i] = String(nombre_dispositivo) + "/" + "duracion_" + String(i);
            s1c[i] = nombre_completo_salida[i] + "c";

            pinMode(Salida[i], OUTPUT);
            digitalWrite(Salida[i], LOW);
        }

        nombre_estado = String(nombre_dispositivo) + "/" + "estado";
        nombre_completo_start = String(nombre_dispositivo) + "/" + "start";
        nombre_completo_tiempo_inicio = String(nombre_dispositivo) + "/" + "tiempo_inicio";
        nombre_completo_ciclo = String(nombre_dispositivo) + "/" + "ciclo";
        nombre_completo_stop = String(nombre_dispositivo) + "/" + "stop";

        connect(servidor_MQTTa, numero_puerto_MQTT);
        client.setCallback(callback);

        pinMode(Entrada_configuracion, INPUT_PULLUP);

        temp_inicio = millis();

        servidorhttp();
        proximoEstado();
    }
}

void comms()
{
    temp_comms = now;

    for (int i = 0; i <= 3; i++)
    {
        Serial.println(nombre_completo_duracion[i]);
        Serial.println(Duracion[i]);
    }
    Serial.println("Inicio Reporte");
    Serial.println(estado);
    Serial.println(nombre_estado);
    Serial.println(nombre_completo_start);
    Serial.println(nombre_completo_ciclo);
    Serial.println(Ciclo);
    Serial.println(nombre_completo_tiempo_inicio);
    Serial.println(Tiempo_inicio);

    unsigned long t1 = Tiempo_inicio / minutos;
    unsigned long t2[4] = {};
    unsigned long t3 = Ciclo / horas;
    String c[4] = {};
    String sa = nombre_estado + "c";
    String sc[4] = {};
    String a="";

    for (int i = 0; i <= 3; i++)
    {
        t2[i] = Duracion[i] / minutos;
        c[i] = (String)t2[i];
        sc[i] = nombre_completo_duracion[i] + "c";
        if (digitalRead(Salida[i])){
            s1ctext[i]="on";
        }
        else {
            s1ctext[i]="off";
        }
    }
    String b = (String)t1;
    String d = (String)t3;

    switch (estado) {
        case 0:
            a="Reposo";
        break;
        case 1:
            a="Riego 1 manual";
        break;
        case 2:
            a="Riego 2 manual";
        break;
        case 3:
            a="Riego 3 manual";
        break;
        case 4:
            a="Riego 4 manual";
        break;
        case 5:
            a="Esperando inicio";
        break;
        case 6:
            a="Esperando ciclo";
        break;
        case 7:
            a="Riego 1 auto";
        break;
        case 8:
            a="Riego 2 auto";
        break;
        case 9:
            a="Riego 3 auto";
        break;
        case 10:
            a="Riego 4 auto";
        break;
        case 11:
            a="Configuracion";
        break;
    }

    String sb = nombre_completo_tiempo_inicio + "c";
    String sd = nombre_completo_ciclo + "c";
    // String stopc = nombre_completo_stop + "c";
    // String startc = nombre_completo_start + "c";

    for (int i = 0; i <= 3; i++)
    {
        client.publish(sc[i].c_str(), c[i].c_str());        // Actualiza duración salidas
        client.publish(s1c[i].c_str(), s1ctext[i].c_str()); // Actualiza estado salidas
    }

    // if (stop){
    //     client.publish(stopc.c_str(), "on");
    // }
    // else{
    //     client.publish(stopc.c_str(), "off");
    // }

    // if (start_ciclo){
    //     client.publish(startc.c_str(), "on");
    // }
    // else{
    //     client.publish(startc.c_str(), "off");
    // }
    
    client.publish(sa.c_str(), a.c_str());
    client.publish(sb.c_str(), b.c_str());
    client.publish(sd.c_str(), d.c_str());


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

        now = millis();

        if (now - temp_comms > 10000) // Lazo envío estado
        {
            comms();
        }

        switch (estado)
        {
            case Estados::Reposo:
                if (primera_vez)
                {
                    //Estado
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    digitalWrite(Salida[2], LOW);
                    digitalWrite(Salida[3], LOW);
                    primera_vez=false;
                    comms();
                }
                //Cambio por tiempo
                
                break;

            case Estados::Riego_1:
                if (Duracion[0]>0){    
                    if (primera_vez)
                    {
                        temp_estado=now;
                        //Estado
                        digitalWrite(Salida[0], HIGH);
                        digitalWrite(Salida[1], LOW);
                        digitalWrite(Salida[2], LOW);
                        digitalWrite(Salida[3], LOW);
                        primera_vez=false;
                        comms();
                    }
                }
                //Cambio por tiempo
                if (now - temp_estado > Duracion[0]) { 
                    proximoEstado(); 
                }

                break;

            case Estados::Riego_2:
                if (Duracion[1]>0){    
                    if (primera_vez)
                    {                
                        temp_estado=now;
                        //Estado
                        digitalWrite(Salida[0], LOW);
                        digitalWrite(Salida[1], HIGH);
                        digitalWrite(Salida[2], LOW);
                        digitalWrite(Salida[3], LOW);
                        primera_vez=false;
                        comms();
                    }
                }
                //Cambio por tiempo
                if (now - temp_estado > Duracion[1]) { 
                    proximoEstado(); 
                }                
                break;

            case Estados::Riego_3:
                if (Duracion[2]>0){    
                    if (primera_vez)
                    {
                        temp_estado=now;
                        //Estado
                        digitalWrite(Salida[0], LOW);
                        digitalWrite(Salida[1], LOW);
                        digitalWrite(Salida[2], HIGH);
                        digitalWrite(Salida[3], LOW);
                        primera_vez=false;
                        comms();
                    }
                }
                //Cambio por tiempo
                if (now - temp_estado > Duracion[2]) { 
                    proximoEstado(); 
                }                
                break;

            case Estados::Riego_4:
                if (Duracion[3]>0){    
                    if (primera_vez)
                    {                
                        temp_estado=now;
                        //Estado
                        digitalWrite(Salida[0], LOW);
                        digitalWrite(Salida[1], LOW);
                        digitalWrite(Salida[2], LOW);
                        digitalWrite(Salida[3], HIGH);
                        primera_vez=false;
                        comms();
                    }
                }
                //Cambio por tiempo
                if (now - temp_estado > Duracion[3]) { 
                    proximoEstado(); 
                }                
                break;

            case Estados::Esperando_inicio:
                if (primera_vez)
                {                
                    temp_estado=now;
                    //Estado
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    digitalWrite(Salida[2], LOW);
                    digitalWrite(Salida[3], LOW);
                    primera_vez=false;
                    comms();
                    }
                //Cambio por tiempo
                if (now - temp_estado > Tiempo_inicio) { 
                    proximoEstado(); 
                }                
                break;

            case Estados::Esperando_ciclo:
                if (primera_vez)
                {                
                    //Estado
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    digitalWrite(Salida[2], LOW);
                    digitalWrite(Salida[3], LOW);
                    primera_vez=false;
                    comms();
                    }
                //Cambio por tiempo
                if (now - temp_ciclo > Ciclo) { 
                    proximoEstado(); 
                }                   
                break;

            case Estados::Ciclo_riego_1:
                if (Duracion[0]>0){    
                    if (primera_vez)
                    {                
                        temp_estado=now;
                        temp_ciclo=now;
                        //Estado
                        digitalWrite(Salida[0], HIGH);
                        digitalWrite(Salida[1], LOW);
                        digitalWrite(Salida[2], LOW);
                        digitalWrite(Salida[3], LOW);
                        primera_vez=false;
                        comms();
                    }
                }
                //Cambio por tiempo
                if (now - temp_estado > Duracion[0]) { 
                    proximoEstado(); 
                }                
                break;

            case Estados::Ciclo_riego_2:
                if (Duracion[1]>0){    
                    if (primera_vez)
                    {                
                        temp_estado=now;
                        //Estado
                        digitalWrite(Salida[0], LOW);
                        digitalWrite(Salida[1], HIGH);
                        digitalWrite(Salida[2], LOW);
                        digitalWrite(Salida[3], LOW);
                        primera_vez=false;
                        comms();
                    }
                }
                //Cambio por tiempo
                if (now - temp_estado > Duracion[1]) { 
                    proximoEstado(); 
                }                 
                break;

            case Estados::Ciclo_riego_3:
                if (Duracion[2]>0){    
                    if (primera_vez)
                    {                
                        temp_estado=now;
                        //Estado
                        digitalWrite(Salida[0], LOW);
                        digitalWrite(Salida[1], LOW);
                        digitalWrite(Salida[2], HIGH);
                        digitalWrite(Salida[3], LOW);
                        primera_vez=false;
                        comms();
                    }
                }
                //Cambio por tiempo
                if (now - temp_estado > Duracion[2]) { 
                    proximoEstado(); 
                }                 
                break;
    
            case Estados::Ciclo_riego_4:
                if (Duracion[3]>0){    
                    if (primera_vez)
                    {                
                        temp_estado=now;
                        //Estado
                        digitalWrite(Salida[0], LOW);
                        digitalWrite(Salida[1], LOW);
                        digitalWrite(Salida[2], LOW);
                        digitalWrite(Salida[3], HIGH);
                        primera_vez=false;
                        comms();
                    }
                }
                //Cambio por tiempo
                if (now - temp_estado > Duracion[3]) { 
                    proximoEstado(); 
                }             
                break;
                
                case Estados::Configuracion:
                
                break;

        }
    }
}
