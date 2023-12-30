/*
 *    RS485 TO  WIFI ADAPTOR CODE
 *    https://github.com/chickey/RS485-WiFi-EPEver
 *    by Colin Hickey 2021
 *    Prrof of concept - Sleep managed by ESP timer D0 Change used to  reset via 12F683
 *    This code is designed to work with the specific board designed by meself which is on sale at tindie and my own website
 *    https://store.eplop.co.uk/product/epever-rs485-to-wifi-adaptor-new-revision/
 *    https://www.tindie.com/products/plop211/epever-rs485-to-wifi-adaptor-v15/
 * 
 *    3D printed case is available at https://www.thingiverse.com/thing:4766788/files
 *    
 *    If your just using just the code and would like to help out a coffee is always appreciated paypal.me/colinmhickey
 *    
 *    A big thankyou to the following project for getting me on the right path https://github.com/glitterkitty/EpEverSolarMonitor 
 *    I also couldn't have made this without the ESPUI project.
 *    
 *    Version 0.51
 *    
*/
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPUI.h>
#include <ModbusMaster.h>
#include <ESP8266WiFi.h>
#include <string.h> 
//#include <SoftwareSerial.h>
//#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include <ESPAsyncWebServer.h>    //Local WebServer used to serve the configuration portal
//#include <ESPAsyncWiFiManager.h>  // switched from tapzu to https://github.com/khoih-prog/ESPAsync_WiFiManager

#include <Updater.h>

const char* ssid     = "ESP8266-Datalogger";
const char* password = "password";
const char* hostname = "datalogger";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
int lastmin = 120;

const char* OTA_INDEX PROGMEM
    = R"=====(<!DOCTYPE html><html><head><meta charset=utf-8><title>OTA</title></head><body><div class="upload"><form method="POST" action="/ota" enctype="multipart/form-data"><input type="file" name="data" /><input type="submit" name="upload" value="Upload" title="Upload Files"></form></div></body></html>)=====";


////////////////
#define DEBUG
//#define GUI_DEBUG
//#define MQTT_DEBUG
//#define INFLUX_DEBUG
////////////////
bool switch_load = true;
bool loadState = false;

#include "settings.h"
#include "config.h"
#include "gui.h"

AsyncWebServer server(80);
DNSServer dns;
ModbusMaster node;   // instantiate ModbusMaster object


#ifndef TRANSMIT_PERIOD
  #define TRANSMIT_PERIOD 30000
#endif
unsigned long time_now = 0;

void preTransmission()
{
  digitalWrite(MAX485_RE, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE, 0);
  digitalWrite(MAX485_DE, 0);
}

void waitForSerial(unsigned long timeout_millis) {
  unsigned long start = millis();
  while (!Serial) {
    if (millis() - start > timeout_millis)
      break;
  }
}

void setup(void) {
  //Attempt to read settings and if it fails resort to factory defaults
  if (!LoadConfigFromEEPROM())
    FactoryResetSettings();
  
  //  Create ESPUI interface tabs
  uint16_t tab1 = ESPUI.addControl( ControlType::Tab, "Settings 1", "Live Data" );
  uint16_t tab2 = ESPUI.addControl( ControlType::Tab, "Settings 2", "Historical Data" );
  uint16_t tab3 = ESPUI.addControl( ControlType::Tab, "Settings 3", "Settings" );
  uint16_t tab4 = ESPUI.addControl( ControlType::Tab, "Settings 4", "Date-Time" );
  uint16_t tab5 = ESPUI.addControl( ControlType::Tab, "Settings 5", "About" );



  //  Add Live Data controls
  SolarVoltage = ESPUI.addControl( ControlType::Label, "Solar Voltage", "0", ControlColor::Emerald, tab1);
  SolarAmps = ESPUI.addControl( ControlType::Label, "Solar Amps", "0", ControlColor::Emerald, tab1);
  SolarWattage = ESPUI.addControl( ControlType::Label, "Solar Wattage", "0", ControlColor::Emerald, tab1);
  BatteryVoltage = ESPUI.addControl( ControlType::Label, "Battery Voltage", "0", ControlColor::Emerald, tab1);
  BatteryAmps = ESPUI.addControl( ControlType::Label, "Battery Amps", "0", ControlColor::Emerald, tab1);
  BatteryWattage = ESPUI.addControl( ControlType::Label, "Battery Wattage", "0", ControlColor::Emerald, tab1);
  LoadVoltage = ESPUI.addControl( ControlType::Label, "Load Voltage", "0", ControlColor::Emerald, tab1);
  LoadAmps = ESPUI.addControl( ControlType::Label, "Load Amps", "0", ControlColor::Emerald, tab1);
  LoadWattage = ESPUI.addControl( ControlType::Label, "Load Wattage", "0", ControlColor::Emerald, tab1);
  BatteryStateOC = ESPUI.addControl( ControlType::Label, "Battery SOC", "0", ControlColor::Emerald, tab1);
  ChargingStatus = ESPUI.addControl( ControlType::Label, "Charging Status", "0", ControlColor::Emerald, tab1);
  BatteryStatus = ESPUI.addControl( ControlType::Label, "Battery Status", "4", ControlColor::Emerald, tab1);
  BatteryTemp = ESPUI.addControl( ControlType::Label, "Battery temperature", "0", ControlColor::Emerald, tab1);
  LoadStatus = ESPUI.addControl( ControlType::Label, "Load Status", "Off", ControlColor::Emerald, tab1);
  //DeviceTemp = ESPUI.addControl( ControlType::Label, "Device Temp", "0", ControlColor::Emerald, tab1);

  // Add Historical Data Controls
  Maxinputvolttoday = ESPUI.addControl( ControlType::Label, "Max input voltage today", "0", ControlColor::Emerald, tab2);
  Mininputvolttoday = ESPUI.addControl( ControlType::Label, "Min input voltage today", "0", ControlColor::Emerald, tab2);
  MaxBatteryvolttoday = ESPUI.addControl( ControlType::Label, "Max battery voltage today", "0", ControlColor::Emerald, tab2);
  MinBatteryvolttoday = ESPUI.addControl( ControlType::Label, "Min battery voltage today", "0", ControlColor::Emerald, tab2);
  ConsumedEnergyToday = ESPUI.addControl( ControlType::Label, "Consumed energy today", "0", ControlColor::Emerald, tab2);
  ConsumedEnergyMonth = ESPUI.addControl( ControlType::Label, "Consumed energy this month", "0", ControlColor::Emerald, tab2);
  ConsumedEngeryYear = ESPUI.addControl( ControlType::Label, "Consumed energy this year", "0", ControlColor::Emerald, tab2);
  TotalConsumedEnergy = ESPUI.addControl( ControlType::Label, "Total consumed energy", "0", ControlColor::Emerald, tab2);
  GeneratedEnergyToday = ESPUI.addControl( ControlType::Label, "Generated energy today", "0", ControlColor::Emerald, tab2);
  GeneratedEnergyMonth = ESPUI.addControl( ControlType::Label, "Generated energy this month", "0", ControlColor::Emerald, tab2);
  GeneratedEnergyYear = ESPUI.addControl( ControlType::Label, "Generated energy this year", "0", ControlColor::Emerald, tab2);
  TotalGeneratedEnergy = ESPUI.addControl( ControlType::Label, "Total generated energy", "0", ControlColor::Emerald, tab2);
    
  //CONFIGPWD = ESPUI.text("Password", callback, ControlColor::Dark, "tiddles123", tab3 ,&CONFIGPWDtxt);
  DEVICEID = ESPUI.addControl( ControlType::Text, "Device ID", String(DEFAULT_DEVICE_ID), ControlColor::Emerald, tab3 ,&DEVICEIDtxt);
  DEVICEBAUD = ESPUI.addControl( ControlType::Text, "BAUD Rate", String(DEFAULT_SERIAL_BAUD), ControlColor::Emerald, tab3 ,&DEVICEBAUDtxt);
  
  LoadSwitchstate = ESPUI.addControl(ControlType::Switcher, "Load", "", ControlColor::Alizarin,tab3, &LoadSwitch);

  RebootButton = ESPUI.addControl( ControlType::Button, "Reboot", "Reboot", ControlColor::Peterriver, tab3, &RebootButtontxt );
  SaveButton = ESPUI.addControl( ControlType::Button, "Save Settings", "Save", ControlColor::Peterriver, tab3, &SaveButtontxt );
  savestatustxt = ESPUI.addControl( ControlType::Label, "Status:", "Changes Saved", ControlColor::Turquoise, tab3 );

  // Add Date-Time Controls
  DateTime = ESPUI.addControl( ControlType::Label, "Epever RTC: dd,mm,yy UTC hh:mm:ss ","01-01-1970  00:00:00", ControlColor::Emerald,tab4);
  //LiveTime = ESPUI.addControl( ControlType::Label, "Time Now", "00:00:00", ControlColor::Emerald,tab4);

  //https://github.com/s00500/ESPUI?tab=readme-ov-file#switch
  //Add the invisible "Time" control
  //auto timeId = ESPUI.addControl(Time, "", "", None, 0, timeCallback);

  // Add Date-Time Controls
  Abouttxt = ESPUI.addControl( ControlType::Label,"", "RS485 MODBUS Data Logger using code written by<br>Colin Hickey 2021<br><br>https://github.com/chickey/RS485-WiFi-EPEver<br><br>modified for off line data logging<br> by Aidan Hedley 2024<br><br>https://github.com/hedley-a/EpeverDataLogger<br><br>", ControlColor::Turquoise,tab5);
#ifdef ENABLE_HA_FACTORY_RESET_FUNCTIONS
  FactoryResetButton = ESPUI.addControl( ControlType::Button, "Reset to Factory Settings", "Reset", ControlColor::Alizarin, tab4, &FactoryResetButtontxt );
#endif

  Serial.begin(myConfig.Device_BAUD);
  waitForSerial(2000);
  Serial1.begin(115200);
  waitForSerial(2000);

  // Connect D0 to RST to wake up
  pinMode(D0, WAKEUP_PULLUP);

  // init modbus in receive mode
  pinMode(MAX485_RE, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  postTransmission();

  // EPEver Device ID and Baud Rate
  node.begin(myConfig.Device_ID, Serial);
    
  // modbus callbacks
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial1.print("AP IP address: ");
  Serial1.println(IP);
  dnsServer.start(DNS_PORT, "*", apIP);
  
  //AsyncWiFiManager wifiManager(&server,&dns);
  //wifiManager.autoConnect("RS485-WiFi");
  //wifiManager.setConfigPortalTimeout(180);
  ESPUI.jsonInitialDocumentSize = 16000; // This is the default, adjust when you have too many widgets or options
  setupGUI();  //Start Web Interface with OTA enabled

  uint8_t baseMac[6];
  WiFi.macAddress(baseMac);
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
}

void niceDelay(unsigned long delayTime)
{  
  unsigned long startTime = millis();
  while (millis() - startTime < delayTime)
  {
    yield();
  }
}

uint16_t ReadRegister(uint16_t Register) {
  // Read register at the address passed in

  niceDelay(50);
  node.clearResponseBuffer();
  uint8_t result = node.readInputRegisters(Register, 1);
  if (result == node.ku8MBSuccess)  {
    
    EQChargeVoltValue = node.getResponseBuffer(0);

#ifdef DEBUG
    Serial.println(String(node.getResponseBuffer(0)));
#endif
  } else  {
#ifdef DEBUG
    Serial1.print(F("Miss read - ")); 
    Serial1.print(Register);
    Serial1.print(F(", ret val:"));
    Serial1.println(result, HEX);
#endif
  }
  return result;
}

void ReadValues() {  
  // clear old data
  //
  memset(rtc.buf,0,sizeof(rtc.buf));
  memset(live.buf,0,sizeof(live.buf));
  memset(stats.buf,0,sizeof(stats.buf));

  // Read registers for clock
  //
  niceDelay(50);
  node.clearResponseBuffer();
  uint8_t result = node.readHoldingRegisters(RTC_CLOCK, RTC_CLOCK_CNT);
  if (result == node.ku8MBSuccess)  {

    rtc.buf[0]  = node.getResponseBuffer(0);
    rtc.buf[1]  = node.getResponseBuffer(1);
    rtc.buf[2]  = node.getResponseBuffer(2);

  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read rtc-data, ret val:"));
    Serial1.println(result, HEX);
#endif
  } 
  if (result==226)     ErrorCounter++;
  
  // read LIVE-Data
  // 
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(LIVE_DATA, LIVE_DATA_CNT);

  if (result == node.ku8MBSuccess)  {

    for(uint8_t i=0; i< LIVE_DATA_CNT ;i++) live.buf[i] = node.getResponseBuffer(i);
       
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read liva-data, ret val:"));
    Serial1.println(result, HEX);
#endif
  }

  // Statistical Data
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(STATISTICS, STATISTICS_CNT);

  if (result == node.ku8MBSuccess)  {
    
    for(uint8_t i=0; i< STATISTICS_CNT ;i++)  stats.buf[i] = node.getResponseBuffer(i);
    
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read statistics, ret val:"));
    Serial1.println(result, HEX);
#endif
  } 

  // BATTERY_TYPE
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(BATTERY_TYPE, 1);
  if (result == node.ku8MBSuccess)  {
    
    BatteryType = node.getResponseBuffer(0);
#ifdef DEBUG
    Serial1.println(String(node.getResponseBuffer(0)));
#endif
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read BATTERY_TYPE, ret val:"));
    Serial1.println(result, HEX);
#endif
  }
 
  // EQ_CHARGE_VOLT
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(EQ_CHARGE_VOLT, 1);
  if (result == node.ku8MBSuccess)  {
    
    EQChargeVoltValue = node.getResponseBuffer(0);
#ifdef DEBUG
    Serial1.println(String(node.getResponseBuffer(0)));
#endif
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read EQ_CHARGE_VOLT, ret val:"));
    Serial1.println(result, HEX);
#endif
  }

  // CHARGING_LIMIT_VOLT
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(CHARGING_LIMIT_VOLT, 1);
  if (result == node.ku8MBSuccess)  {
    
    ChargeLimitVolt = node.getResponseBuffer(0);
#ifdef DEBUG
    Serial1.println(String(node.getResponseBuffer(0)));
#endif
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read CHARGING_LIMIT_VOLT, ret val:"));
    Serial1.println(result, HEX);
#endif
  }
  
  // Capacity
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(BATTERY_CAPACITY, 1);
  if (result == node.ku8MBSuccess)  {
    
    BatteryCapactity = node.getResponseBuffer(0);
#ifdef DEBUG
    Serial1.println(String(node.getResponseBuffer(0)));
#endif
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read BATTERY_CAPACITY, ret val:"));
    Serial1.println(result, HEX);
#endif
  }   
  // Battery SOC
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(BATTERY_SOC, 1);
  if (result == node.ku8MBSuccess)  {
    
    batterySOC = node.getResponseBuffer(0);
    
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read batterySOC, ret val:"));
    Serial1.println(result, HEX);
#endif
  }

  // Battery Net Current = Icharge - Iload
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(  BATTERY_CURRENT_L, 2);
  if (result == node.ku8MBSuccess)  {
    
    batteryCurrent = node.getResponseBuffer(0);
    batteryCurrent |= node.getResponseBuffer(1) << 16;
    
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read batteryCurrent, ret val:"));
    Serial1.println(result, HEX);
#endif
  }
 
  if (!switch_load) {
    // State of the Load Switch
    niceDelay(50);
    node.clearResponseBuffer();
    result = node.readCoils(  LOAD_STATE, 1 );
    if (result == node.ku8MBSuccess)  {
      
      loadState = node.getResponseBuffer(0);
          
    } else  {
#ifdef DEBUG
      Serial1.print(F("Miss read loadState, ret val:"));
      Serial1.println(result, HEX);
 #endif
    }
  }

  // Read Model
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(CCMODEL, 1);
  if (result == node.ku8MBSuccess)  {
    
    CCModel = node.getResponseBuffer(0);
    
  } else {
#ifdef DEBUG
    Serial1.print(F("Miss read Model, ret val:"));
    Serial1.println(result, HEX);
#endif
  }
    
  // Read Status Flags
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(  0x3200, 2 );
  if (result == node.ku8MBSuccess)  {

    uint16_t temp = node.getResponseBuffer(0);
#ifdef DEBUG 
    Serial1.print(F("Batt Flags : "));
    Serial1.println(temp);
#endif
    
    status_batt.volt = temp & 0b1111;
    status_batt.temp = (temp  >> 4 ) & 0b1111;
    status_batt.resistance = (temp  >>  8 ) & 0b1;
    status_batt.rated_volt = (temp  >> 15 ) & 0b1;
    
    temp = node.getResponseBuffer(1);
#ifdef DEBUG 
    Serial1.print(F("Chrg Flags : "));
    Serial1.println(temp, HEX); 
#endif

    charger_mode = ( temp & 0b0000000000001100 ) >> 2 ;
#ifdef DEBUG 
    Serial1.print(F("charger_mode  : "));
    Serial1.println( charger_mode );
#endif
  } else {
#ifdef DEBUG
    Serial.print(F("Miss read ChargeState, ret val:"));
    Serial.println(result, HEX);
#endif
  }
}


void debug_output(){
#ifdef DEBUG
  //Output values to serial
  Serial1.printf("\n\nTime:  20%02d-%02d-%02d   %02d:%02d:%02d   \n",  rtc.r.y , rtc.r.M , rtc.r.d , rtc.r.h , rtc.r.m , rtc.r.s  );
  /*
  Serial1.print(  F("\nLive-Data:           Volt        Amp       Watt  "));
  Serial1.printf( "\n  Panel:            %7.3f    %7.3f    %7.3f ",  live.l.pV/100.f ,  live.l.pI/100.f ,  live.l.pP/100.0f );
  Serial1.printf( "\n  Batt:             %7.3f    %7.3f    %7.3f ",  live.l.bV/100.f ,  live.l.bI/100.f ,  live.l.bP/100.0f );
  Serial1.printf( "\n  Load:             %7.3f    %7.3f    %7.3f \n",  live.l.lV/100.f ,  live.l.lI/100.f ,  live.l.lP/100.0f );
  Serial1.printf( "\n  Battery Current:  %7.3f  A ",      batteryCurrent/100.f  );
  Serial1.printf( "\n  Battery SOC:      %7.0f  %% ",     batterySOC/1.0f  );
  Serial1.printf( "\n  Load Switch:          %s   ",     (loadState==1?" On":"Off") );

  Serial1.print(  F("\n\nStatistics:  "));
  Serial1.printf( "\n  Panel:       min: %7.3f   max: %7.3f   V", stats.s.pVmin/100.f  , stats.s.pVmax/100.f  );
  Serial1.printf( "\n  Battery:     min: %7.3f   max: %7.3f   V\n", stats.s.bVmin /100.f , stats.s.bVmax/100.f);

  Serial1.printf( "\n  Consumed:    day: %7.3f   mon: %7.3f   year: %7.3f  total: %7.3f   kWh",
      stats.s.consEnerDay/100.f  ,stats.s.consEnerMon/100.f  ,stats.s.consEnerYear/100.f  ,stats.s.consEnerTotal/100.f   );
  Serial1.printf( "\n  Generated:   day: %7.3f   mon: %7.3f   year: %7.3f  total: %7.3f   kWh",
      stats.s.genEnerDay/100.f   ,stats.s.genEnerMon/100.f   ,stats.s.genEnerYear/100.f   ,stats.s.genEnerTotal/100.f  );
  Serial1.printf( "\n  CO2-Reduction:    %7.3f  t\n",      stats.s.c02Reduction/100.f  );

  Serial1.print(  F("\nStatus:"));
  Serial1.printf( "\n    batt.volt:         %s   ",     batt_volt_status[status_batt.volt] );
  Serial1.printf( "\n    batt.temp:         %s   ",     batt_temp_status[status_batt.temp] );
  Serial1.printf( "\n    charger.charging:  %s   \n\n",     charger_charging_status[ charger_mode] );
  */
#endif
}

void loop(void) {

  String buffer;
  buffer.reserve(64);
  // Print out to serial if debug is enabled.
  //
#ifdef DEBUG
    debug_output();
#endif

// Do these need to be continuously updated as they only change when the
//   user changes them via webpage
// Disabling them should reduce the JSON packet size from ESPUI

  ESPUI.updateControlValue(DEVICEID , String(myConfig.Device_ID));
  ESPUI.updateControlValue(DEVICEBAUD , String(myConfig.Device_BAUD));

  // Read Values from Charge Controller
  ReadValues();

  //Update ESPUI Live Data components
  ESPUI.updateControlValue(LoadStatus , String(loadState==1?F(" On"):F("Off")));
  ESPUI.updateControlValue(DeviceTemp , String(batt_temp_status[status_batt.temp]));
  
  buffer.clear(); buffer.concat(rtc.r.d); buffer.concat(F("-"));buffer.concat(rtc.r.M);buffer.concat(F("-"));buffer.concat(rtc.r.y);
  buffer.concat(F("  ")); buffer.concat(rtc.r.h); buffer.concat(F(":"));

  if(rtc.r.m <= 9){
    buffer.concat(F("0")); // leading zero
  } 
  buffer.concat(rtc.r.m);buffer.concat(F(":"));

  if(rtc.r.s <= 9){
    buffer.concat(F("0")); // leading zero
  }
  ;buffer.concat(rtc.r.s);
  ESPUI.updateControlValue(DateTime, buffer);
  
  buffer.clear(); buffer.concat(live.l.pV/100.f); buffer.concat(F("V"));
  ESPUI.updateControlValue(SolarVoltage , buffer);

  buffer.clear(); buffer.concat(live.l.pI/100.f); buffer.concat(F("A"));
  ESPUI.updateControlValue(SolarAmps , buffer);
  
  buffer.clear(); buffer.concat(live.l.pP/100.0f); buffer.concat(F("w"));
  ESPUI.updateControlValue(SolarWattage , buffer);
  
  buffer.clear(); buffer.concat(live.l.bV/100.f); buffer.concat(F("V"));
  ESPUI.updateControlValue(BatteryVoltage  , buffer);
  
  buffer.clear(); buffer.concat(live.l.bI/100.f); buffer.concat(F("A"));
  ESPUI.updateControlValue(BatteryAmps , buffer);
  
  buffer.clear(); buffer.concat(live.l.bP/100.0f); buffer.concat(F("w"));
  ESPUI.updateControlValue(BatteryWattage , buffer);
  
  buffer.clear(); buffer.concat(live.l.lV/100.f); buffer.concat(F("V"));
  ESPUI.updateControlValue(LoadVoltage , buffer);
  
  buffer.clear(); buffer.concat(live.l.lI/100.f); buffer.concat(F("A"));
  ESPUI.updateControlValue(LoadAmps , buffer);
  
  buffer.clear(); buffer.concat(live.l.lP/100.0f); buffer.concat(F("w"));
  ESPUI.updateControlValue(LoadWattage , buffer);
  
  buffer.clear(); buffer.concat(batterySOC/1.0f); buffer.concat(F("%"));
  ESPUI.updateControlValue(BatteryStateOC , buffer);
  ESPUI.updateControlValue(ChargingStatus , String(charger_charging_status[ charger_mode]));
  ESPUI.updateControlValue(BatteryStatus , String(batt_volt_status[status_batt.volt]));
  ESPUI.updateControlValue(BatteryTemp , String(batt_temp_status[status_batt.temp]));

  //Update historical values
  buffer.clear(); buffer.concat(stats.s.pVmax/100.f); buffer.concat(F("V"));
  ESPUI.updateControlValue(Maxinputvolttoday, buffer);
  
  buffer.clear(); buffer.concat(stats.s.pVmin/100.f); buffer.concat(F("V"));
  ESPUI.updateControlValue(Mininputvolttoday , buffer);
  
  buffer.clear(); buffer.concat(stats.s.bVmax/100.f); buffer.concat(F("V"));
  ESPUI.updateControlValue(MaxBatteryvolttoday , buffer);
  
  buffer.clear(); buffer.concat(stats.s.bVmin/100.f); buffer.concat(F("V"));
  ESPUI.updateControlValue(MinBatteryvolttoday, buffer);
  
  buffer.clear(); buffer.concat(stats.s.consEnerDay/100.f); buffer.concat(F("kWh"));
  ESPUI.updateControlValue(ConsumedEnergyToday , buffer);
  
  buffer.clear(); buffer.concat(stats.s.consEnerMon/100.f); buffer.concat(F("kWh"));
  ESPUI.updateControlValue(ConsumedEnergyMonth , buffer);
  
  buffer.clear(); buffer.concat(stats.s.consEnerYear/100.f); buffer.concat(F("kWh"));
  ESPUI.updateControlValue(ConsumedEngeryYear , buffer);
  
  buffer.clear(); buffer.concat(stats.s.consEnerTotal/100.f); buffer.concat(F("kWh"));
  ESPUI.updateControlValue(TotalConsumedEnergy , buffer);
  
  buffer.clear(); buffer.concat(stats.s.genEnerDay/100.f); buffer.concat(F("kWh"));
  ESPUI.updateControlValue(GeneratedEnergyToday , buffer);
  
  buffer.clear(); buffer.concat(stats.s.genEnerMon/100.f); buffer.concat(F("kWh"));
  ESPUI.updateControlValue(GeneratedEnergyMonth , buffer);
  
  buffer.clear(); buffer.concat(stats.s.genEnerYear/100.f); buffer.concat(F("kWh"));
  ESPUI.updateControlValue(GeneratedEnergyYear , buffer);
  
  buffer.clear(); buffer.concat(stats.s.genEnerTotal/100.f); buffer.concat(F("kWh"));
  ESPUI.updateControlValue(TotalGeneratedEnergy , buffer);
  
 // buffer.clear(); buffer.concat(stats.s.c02Reduction/100.f); buffer.concat(F("t"));
 // ESPUI.updateControlValue(Co2Reduction , buffer);
  
  // Do the Switching of the Load here and post new state to MQTT if enabled
  if( switch_load == 1 ){
    switch_load = 0;  

#ifdef DEBUG
    Serial1.print(F("Switching Load "));
    Serial1.println( (loadState?F("On"):F("Off")) );
    niceDelay(200);
#endif

  uint8_t result = node.writeSingleCoil(0x0002, loadState);

#ifdef DEBUG
  if (result != node.ku8MBSuccess)  {
    Serial1.print(F("Miss write loadState, ret val:"));
    Serial1.println(result, HEX);
  }
#endif
  }

  //Check error count and if it exceeds 5 reset modbus
#ifdef DEBUG
  Serial1.print(F("Error count = "));
  Serial1.println(ErrorCounter);
#endif
  if (ErrorCounter>5) {
    // init modbus in receive mode
    pinMode(MAX485_RE, OUTPUT);
    pinMode(MAX485_DE, OUTPUT);
    postTransmission();

    // EPEver Device ID and Baud Rate
    node.begin(myConfig.Device_ID, Serial);
    
    // modbus callbacks
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    ErrorCounter = 0;  
  }
  
  // power down MAX485_DE
  postTransmission();
}
