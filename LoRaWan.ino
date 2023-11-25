#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "DHT.h"
#include "keys.h"

#define DHTPIN GPIO1 // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
/* OTAA para*/
uint8_t devEui[] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x2E, 0x6E};
uint8_t appEui[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// TTN Application Key
uint8_t appKey[] = APP_KEY;

/* ABP para*/
uint8_t nwkSKey[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
uint8_t appSKey[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
uint32_t devAddr = (uint32_t)0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = {0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t loraWanClass = LORAWAN_CLASS;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = LORAWAN_NETMODE;

/*ADR enable*/
bool loraWanAdr = false;

/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
bool keepNet = LORAWAN_NET_RESERVE;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = LORAWAN_UPLINKMODE;

/* Application port */
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 2;

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port, int t, int h)
{
	appDataSize = 8;
	appData[0] = (t >> 24) & 0xFF;
	appData[1] = (t >> 16) & 0xFF;
	appData[2] = (t >> 8) & 0xFF;
	appData[3] = t & 0xFF;

	appData[4] = (h >> 24) & 0xFF;
	appData[5] = (h >> 16) & 0xFF;
	appData[6] = (h >> 8) & 0xFF;
	appData[7] = h & 0xFF;
}

void setup()
{
	Serial.begin(115200);
	dht.begin();
#if (AT_SUPPORT)
	enableAt();
#endif
	deviceState = DEVICE_STATE_INIT;
	LoRaWAN.ifskipjoin();
	LoRaWAN.setDataRateForNoADR(3);
}

void loop()
{
	int h = (int)dht.readHumidity();
	int t = (int)dht.readTemperature();
	switch (deviceState)
	{
	case DEVICE_STATE_INIT:
	{
#if (LORAWAN_DEVEUI_AUTO)
		LoRaWAN.generateDeveuiByChipID();
#endif
#if (AT_SUPPORT)
		getDevParam();
#endif
		printDevParam();
		LoRaWAN.init(loraWanClass, loraWanRegion);
		deviceState = DEVICE_STATE_JOIN;
		break;
	}
	case DEVICE_STATE_JOIN:
	{
		LoRaWAN.join();
		break;
	}
	case DEVICE_STATE_SEND:
	{
		prepareTxFrame(appPort, t, h);
		LoRaWAN.send();
		deviceState = DEVICE_STATE_CYCLE;
		break;
	}
	case DEVICE_STATE_CYCLE:
	{
		// Schedule next packet transmission
		txDutyCycleTime = appTxDutyCycle + randr(0, APP_TX_DUTYCYCLE_RND);
		LoRaWAN.cycle(txDutyCycleTime);
		deviceState = DEVICE_STATE_SLEEP;
		break;
	}
	case DEVICE_STATE_SLEEP:
	{
		LoRaWAN.sleep();
		break;
	}
	default:
	{
		deviceState = DEVICE_STATE_INIT;
		break;
	}
	}
}
