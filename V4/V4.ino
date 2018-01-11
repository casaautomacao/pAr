

extern "C" {
#include "user_interface.h"
#include "espconn.h"
}

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <Crescer.h>


Tempora tempCom;
Tempora tempImpr;
Tempora tempConfSTA;
Tempora tempScanSSID;
Tempora tempPing;

byte ModoOper = 0;
String SSIDcasa = "", SENHAcasa = "", IPfixo = "", Porta = "";
byte BIPfixo = 0;
int IPorta = 80;

IPAddress ip;
IPAddress ip1;
IPAddress ip2;
byte ipap[4] = {192, 168, 4, 1};
byte FinalIP = 56;
WiFiServer server(80);

int  val0 = 0, val2 = 0;
int pino0 = 0, pino2 = 2;
int ssidOK = 0;


int statusWIFI = 255;
boolean setaVar = 0, setaSSID = 0, setaSTA = 0, setaConecta = 0;

/////////////////////////////////////////////////////
////////////////////////    SETUP
////////////////////////////////////////////////////

void setup() {


  // prepare Saídas
  pinMode(pino2, OUTPUT);
  digitalWrite(pino2, val2);
  pinMode(pino0, OUTPUT);
  digitalWrite(pino0, val0);

  // EEPROM
  EEPROM.begin(512); // max 4096
  ModoOper  = EEPROM.read(0);

  // Timers
  tempCom.defiSP(5000);
  tempImpr.defiSP(500);
  tempConfSTA.defiSP(50000);
  tempScanSSID.defiSP(20000);
  tempPing.defiSP(60000);

  // Serial
  Serial.begin(9600);
  delay(1000);
  Serial.println();
  Serial.print("Memoria do modo de operação: ");
  Serial.println(ModoOper);
  Serial.println();

  // Configura AP
  WiFi.mode(WIFI_AP_STA);
  AcessPoint();

}

/////////////////////////////////////////////////////
////////////////////////    LOOP
////////////////////////////////////////////////////

void loop() {

  // decide entre AP ou STA, normalmente usado para primeira gravação
  if (ModoOper == 1)
  {
    // entra se for STA
    //Le os parametros da rede WIFI somente no primeiro loop
    if (!setaVar)
    {
      Le_variaveis();
      setaVar = 1;
    }
    //Verifica se o SSID existe somente no primeiro loop
    if (!setaSSID)
    {
      ssidOK = detecta_SSID();
      setaSSID = 1;
    }

    if (ssidOK)
    {
      //SSID existindo, faz a conexão somente no primeiro loop virando STA
      if (!setaSTA)
      {
        WiFi.mode(WIFI_STA);
        configura_Server();
        setaSTA = 1;
      }

      // NÃO CONECTOU, volta para AP
      statusWIFI = WiFi.status();
      if (statusWIFI != 3)
      {
       // WiFi.mode(WIFI_AP);
        WiFi.mode(WIFI_AP_STA);
        // Verifica de tempo em tempo se a conexão voltou
        //if (tempConfSTA.Saida(1))
        //  {
        //   WiFi.mode(WIFI_STA);
        //   configura_Server();
        //   tempConfSTA.Saida(0);
        //  }

      }
      //Se estiver conectado mantem o tempo em 0
      else tempConfSTA.Saida(0);

      tempScanSSID.Saida(0);
    }
    // de tempo em tempo verifica o SSID
    else
    {

      if (tempScanSSID.Saida(1))
      {
        ssidOK = detecta_SSID();
        tempScanSSID.Saida(0);
      }


    }

  }






  // se estiver conectado, pinga o servidor a cada 60s, para ver se está tudo correto

  if (statusWIFI == 3) //  tem que ter gateway
  {
    if (tempPing.Saida(1))

    {

      if (Ping.ping(WiFi.gatewayIP()) ) {
        Serial.println("Pingou!!");

      } else {
        Serial.println("Erro no Ping");
        ESP.restart();
      }
      tempPing.Saida(0);
    }

  }


  //////////////////////////////////////////////////////////////////
  ////////////////  Parte PADRÃO
  /////////////////////////////////////////////////////////////////

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    tempCom.Saida(0);
    return;
  }


  tempPing.Saida(0);

  while (!client.available()) {
    delay(1);

    if (tempCom.Saida(1))
    {
      client.flush();
      String s = "";
      s =  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
      s += "client invalidou";


      // Send the response to the client
      client.print(s);
      delay(1);
      //   Serial.println("Client disconectado depois da nao comunicacao");

      tempCom.Saida(0);
      return;

    }
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();





  // Match the request

  if (req.indexOf("?saida2L=") != -1)
    val2 = 1;
  else if (req.indexOf("?saida2D=") != -1)
    val2 = 0;
  else if (req.indexOf("?saida0L=") != -1)
    val0 = 1;
  else if (req.indexOf("?saida0D=") != -1)
    val0 = 0;
  else if (req.indexOf("Configura") != -1)
  {
    String s = "";
    s = configuraSTA(req);
    client.print(s);
    client.stop();
    delay(1000);
    ESP.restart();

  }

  // Set GPIO2 according to the request


  digitalWrite(pino2, val2);
  digitalWrite(pino0, val0);


  client.flush();
  String s = "";
  s =  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  s += "rodou o loop normal";


  // Send the response to the client
  client.print(s);

  //  Serial.println("Client disonnected");
  delay(10);
  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed

  client.stop();


}

