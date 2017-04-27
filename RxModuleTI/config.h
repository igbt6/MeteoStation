/*
 * config.h
 *
 *  Created on: 19 lis 2016
 *      Author: igbt6
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "ads7843.h"


#define DEFAULT_VERSION 0xFF

////////////////////////////////////////////////////////////////////////
//FLASH stored parameters
////////////////////////////////////////////////////////////////////////

// Define the size of the flash program blocks for saving configuration data.
#define FLASH_PB_START          0x3FF00
#define FLASH_PB_END            FLASH_PB_START + 0xFF
//#define FLASH_PB_START          0x20000
//#define FLASH_PB_END            FLASH_PB_START + 0x4000
// The size of the parameter block to save.  This must be a power of 2,
// and should be large enough to contain the ConfigFlashParameters structure.
#define FLASH_PB_SIZE           256

typedef struct
{
	uint8_t wifiEnabled;
	uint8_t sensorsEnabled;
	uint8_t powerSavingEnabled;
	uint8_t wifiConnectionState;
	uint8_t sensorConnectionState;
}ConectionSetupState;

typedef struct
{
	ConectionSetupState connectionSetupState;
	uint8_t currentCity;
	uint8_t currentWifiConfig;
	uint8_t paramsVersion;
	uint8_t isModified;
}ConfigFlashParameters;

void configFlashInit(void);
void configFlashLoadFactory(void);
void configFlashLoad(void);
void configFlashSaveDefaults(void);
void configFlashSave(void);
bool configFlashCheckAndCleanModified(ConfigFlashParameters * const ptr);
void configFlashSetModified(ConfigFlashParameters* const ptr);
bool configFlashIsInvalid(const ConfigFlashParameters * const ptr);
ConfigFlashParameters* const configFlashGetDefaultSettings(void);
ConfigFlashParameters* configFlashGetCurrent(void);

////////////////////////////////////////////////////////////////////////
//EEPROM stored parameters
////////////////////////////////////////////////////////////////////////
#define MAX_CITIES 3
#define MAX_PARAMETER_NAME_LENGTH 20
#define MAX_WIFI_CONFIGS 2

#define  EEPROM_START_ADDRESS 0x0000

typedef struct
{
	CalibCoefficients calibCoeffs;
	uint8_t isValid; // 0 - default invalid, other - valid
}TouchScreenConfigParameters;

typedef struct
{
	char apSSID[MAX_PARAMETER_NAME_LENGTH];
	char apWPA2pass[MAX_PARAMETER_NAME_LENGTH];
}AccessPointConfigParameters;

typedef struct
{
	TouchScreenConfigParameters touchScreenParams;
	AccessPointConfigParameters wifiConfig[MAX_WIFI_CONFIGS];
	char cityNames[MAX_CITIES][MAX_PARAMETER_NAME_LENGTH];
	char openweatherDomain[MAX_PARAMETER_NAME_LENGTH + MAX_PARAMETER_NAME_LENGTH];
	uint8_t updateWifiPeriodTime;  /*Time after which request to OpenWeatherMap is sent in seconds*/
	uint8_t updateSensorPeriodTime;/*Time after which request to Sensor is sent in seconds*/
	uint8_t paramsVersion; 		   /* First Time the value will be 0x00 -invalid, 0x01-defaults, 0x02 - updated */
	uint8_t isModified;
}ConfigEepromParameters;

/*
 * @brief utils for EEprom memory
 * @return true on success
 */

void configEepromLoad(void);
bool configEepromInit(void);
bool configEepromSaveDefaults(void);
bool configEepromSave(void);
bool configEepromCheckAndCleanModified(ConfigEepromParameters * const ptr);
void configEepromSetModified(ConfigEepromParameters* const ptr);
bool configEepromIsInvalid(const ConfigEepromParameters * const ptr);
ConfigEepromParameters* const configEepromGetDefaultSettings(void);
ConfigEepromParameters* configEepromGetCurrent(void);

////////////////////////////////////////////////////////////////////////
//public API methods:
////////////////////////////////////////////////////////////////////////
void configInit(void);

#endif /* CONFIG_H_ */
