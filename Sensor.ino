#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <Agentuino.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 198, 0, 100);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetClient client;

#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

int porta = 10051;
String host = "sensorserv";

/*
# Agentuino 
# RFC1213-MIB OIDs 
# .iso (.1) 
# .iso.org (.1.3) 
# .iso.org.dod (.1.3.6) 
# .iso.org.dod.internet (.1.3.6.1) 
# .iso.org.dod.internet.mgmt (.1.3.6.1.2) 
# .iso.org.dod.internet.mgmt.mib-2 (.1.3.6.1.2.1) 
# .iso.org.dod.internet.mgmt.mib-2.system (.1.3.6.1.2.1.1) 
#...system.sysDescr(.1.3.6.1.2.1.1.1) read-only RO - DisplayString
#...system.sysObjectID (.1.3.6.1.2.1.1.2) RO - ObjectIdentifier 
#...system.sysUpTime (.1.3.6.1.2.1.1.3) RO - TimeTicks 
#...system.sysContact (.1.3.6.1.2.1.1.4) RW - DisplayString 
#...system.sysName (.1.3.6.1.2.1.1.5) RW - DisplayString 
#...system.sysLocation (.1.3.6.1.2.1.1.6) RW - DisplayString 
#...system.sysServices (.1.3.6.1.2.1.1.7) RO - Integer 
*/
const static char sysDescr[20] PROGMEM = "1.3.6.1.2.1.1.1.0"; 
const static char sysObjectID[20] PROGMEM = "1.3.6.1.2.1.1.2.0"; 
const static char sysUpTime[20] PROGMEM = "1.3.6.1.2.1.1.3.0"; 
const static char sysContact[20] PROGMEM = "1.3.6.1.2.1.1.4.0"; 
const static char sysName[20] PROGMEM = "1.3.6.1.2.1.1.5.0"; 
const static char sysLocation[20] PROGMEM = "1.3.6.1.2.1.1.6.0"; 
const static char sysServices[20] PROGMEM = "1.3.6.1.2.1.1.7.0";
/*
# Arduino defined OIDs 
# .iso.org.dod.internet.private (.1.3.6.1.4) 
# .iso.org.dod.internet.private.enterprises (.1.3.6.1.4.1) 
# ...enterprises.arduino (.1.3.6.1.4.1.36582) 
# ...arduino.value.valA0-A5 (.1.3.6.1.4.1.36582.3.1-6) RO - Integer 
*/
const static char temp[24] PROGMEM = "1.3.6.1.4.1.36582.3.1.0";
const static char umidade[24] PROGMEM = "1.3.6.1.4.1.36582.3.2.0";
const static char locDescr[20] = "Agentuino"; 
const static char locObjectID[20] = "1.3.6.1.4.1.36582"; 
const static uint32_t locUpTime = 0; 
const static char locContact[20] = "e-mail@administrador.com"; 
const static char locName[20] = "Responsável"; 
const static char locLocation[20] = "Local"; 
const static int32_t locServices = 7;

int loctemp = 0; 
int locumidade = 0; 

uint32_t dispMillis = 0;
char oid[SNMP_MAX_OID_LEN]; 
SNMP_API_STAT_CODES api_status; 
SNMP_ERR_CODES status;

void pduReceived() 
{
	SNMP_PDU pdu;
	api_status = Agentuino.requestPdu(&pdu);
	if ((pdu.type == SNMP_PDU_GET || pdu.type == SNMP_PDU_GET_NEXT || 
		 pdu.type == SNMP_PDU_SET) && pdu.error == SNMP_ERR_NO_ERROR && 
		 api_status == SNMP_API_STAT_SUCCESS ) {
		pdu.OID.toString(oid);
		if ( pdu.type == SNMP_PDU_SET ) {
			status = SNMP_ERR_READ_ONLY;
		} else if ( strcmp_P(oid, sysDescr ) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locDescr);
		} else if ( strcmp_P(oid, sysUpTime ) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_TIME_TICKS, locUpTime);
		} else if ( strcmp_P(oid, sysName ) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locName);
		} else if ( strcmp_P(oid, sysContact ) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locContact);
		} else if ( strcmp_P(oid, sysLocation ) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locLocation);
		} else if ( strcmp_P(oid, sysServices) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_INT, locServices);
		} else if ( strcmp_P(oid, sysObjectID) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locObjectID);
		} else if ( strcmp_P(oid, temp) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_INT, loctemp);
		} else if ( strcmp_P(oid, umidade) == 0 ) {
			status = pdu.VALUE.encode(SNMP_SYNTAX_INT, locumidade);
		} else {
			status = SNMP_ERR_NO_SUCH_NAME;
		}
		pdu.type = SNMP_PDU_RESPONSE; 
		pdu.error = status; 
		Agentuino.responsePdu(&pdu); 
	}
	Agentuino.freePdu(&pdu); 
}

void setup() {
	dht.begin();
	delay(1000);
	Ethernet.begin(mac, ip, gateway, subnet);
	delay(15000);
	api_status = Agentuino.begin(); //inicialização do pacote Agentuino
	if ( api_status == SNMP_API_STAT_SUCCESS ) { //Condições para inicialização do Pacote Agentuino 
		Agentuino.onPduReceive(pduReceived); 
		delay(10); 
		return; 
	} else { 
		delay(10); 
	}
}

void loop() {
	
	int u = (dht.readHumidity())*10; // variavel para umidade
	int t = (dht.readTemperature())*10; // variavel para temperatura
	
	Agentuino.listen(); //identificador se existe solicitações SNMP
	
	loctemp = t;
	locumidade = u;
	
}
