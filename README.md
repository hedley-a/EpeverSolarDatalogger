# ESP8266 based datalogger for EPEver Solar CC
- Thanks to https://github.com/chickey/RS485-WiFi-EPEver


# Features:
Hardware Changes:
 *    To Add: Microchip 12F683 to time wake (reset) from sleep and reset on external input
 *    Logging: will be saving homeopathic amounts of data, 4 bytes hourly  - 8 bytes daily,  
 *    32K bytes of FRAM will - log 512 days saved as bitmap. 
 *    Software Changes:
 *    WiFi-GUI as AP - no LAN so direct GUI connection to mobile device - implemented 20/12/23
 *    Powersave: ESP runs once to log load and solar power: reset every 40 seconds by 12F683
 *    Epever RTC used hourly to save watt/hour figures to FRAM
 *    If load state toggled - ESP+wifi+GUI powered for 10 mins (use Epever RTC to time this?)
 *    Managed by GUI banner with timeout - reset by GUI pushbutton?
 *    Midday Load OFF - every day - 
-

