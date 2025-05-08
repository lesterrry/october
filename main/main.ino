/******************************
Suburbs: october
Universal thermostatic relay

me@aydar.media
******************************/

#define DEBUG 1

#undef CLEAR_EEPROM

#define RELAY_PIN                          7
#define ENC_CLK_PIN                        11
#define ENC_DT_PIN                         10
#define ENC_SW_PIN                         9
#define TEMPA_DT_PIN                       5
#define TEMPA_SCK_PIN                      6
#define TEMPB_DT_PIN                       2
#define TEMPB_SCK_PIN                      3
#define LED_ADDRESS                        0x27
#define LED_WIDTH                          16
#define LED_HEIGHT                         2
#define EEPROM_INITIAL_TEMP_SET_ADDRESS    0
#define EEPROM_INITIAL_MODE_ADDRESS        2
#define TEMP_SET_MAX                       30
#define TEMP_SET_MIN                       10
#define TEMP_GRACE_INC                     1
#define TEMP_GRACE_DEC                     0
#define EXT_TEMP_ENABLED                   1

#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <GyverEncoder.h>
#include <SHT1x.h>
