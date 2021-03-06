#include <WildFire_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

boolean restart_required_buildtest = false;
extern boolean restart_required_fwpatch;

WildFire_CC3000 cc3000;
#define WLAN_SSID       "MyNetworkSSID"        // cannot be longer than 32 characters!
#define WLAN_PASS       "MyNetworkPassword"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

void testCC3000(void){
  if(testCC3000_enabled){
    wf.begin();
    
    if(restart_required_fwpatch){
      Serial.println(F("A restart is required before executing this test...")); 
      Serial.println(F("No action taken."));
      testCC3000_enabled = false;
      return;
    }
    else{
      Serial.println(F("Testing CC3000 Functionality... please be patient while it connects to the network."));
    }
    
    restart_required_buildtest = true;
     
    /* Initialise the module */
    Serial.print(F("CC3000 Initialization..."));
    if (!cc3000.begin())
    {
      Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
      while(1);
    }
    Serial.println(F("Complete."));    
    
    displayDriverMode();
    uint16_t firmware = checkFirmwareVersion();
    displayMACAddress();
#ifndef CC3000_TINY_DRIVER
    listSSIDResults();
#endif    
#ifndef CC3000_TINY_DRIVER
    listSSIDResults();
#endif    

    /* Attempt to connect to an access point */
    char *ssid = WLAN_SSID;             /* Max 32 chars */
    Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);
    
    /* NOTE: Secure connections are not available in 'Tiny' mode!
       By default connectToAP will retry indefinitely, however you can pass an
       optional maximum number of retries (greater than zero) as the fourth parameter.
    */
    if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
      Serial.println(F("Failed!"));
      while(1);
    }
     
    Serial.println(F("Connected!"));
    
    /* Wait for DHCP to complete */
    Serial.println(F("Request DHCP"));
    while (!cc3000.checkDHCP())
    {
      delay(100); // ToDo: Insert a DHCP timeout!
    }  
  
    /* Display the IP address DNS, Gateway, etc. */  
    while (! displayConnectionDetails()) {
      delay(1000);
    }
    
  #ifndef CC3000_TINY_DRIVER
    /* Try looking up www.wickeddevice.com */
    uint32_t ip = 0;
    Serial.print(F("www.wickeddevice.com -> "));
    while  (ip  ==  0)  {
      if  (!  cc3000.getHostByName("www.wickeddevice.com", &ip))  {
        Serial.println(F("Couldn't resolve!"));
      }
      delay(500);
    }  
    cc3000.printIPdotsRev(ip);
    
    /* Do a quick ping test on wickeddevice.com */  
    Serial.print(F("\n\rPinging ")); cc3000.printIPdotsRev(ip); Serial.print("...");  
    uint8_t replies = cc3000.ping(ip, 5);
    Serial.print(replies); Serial.println(F(" replies"));
    if (replies)
      Serial.println(F("Ping successful!"));
  #endif
  
    /* You need to make sure to clean up after yourself or the CC3000 can freak out */
    /* the next time you try to connect ... */
    Serial.println(F("\n\nClosing the connection"));
    cc3000.disconnect();   
    
    Serial.println(F("Test Complete."));    
    testCC3000_enabled = false;
    
  }  
}

/**************************************************************************/
/*!
    @brief  Displays the driver mode (tiny of normal), and the buffer
            size if tiny mode is not being used

    @note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
  #ifdef CC3000_TINY_DRIVER
    Serial.println(F("CC3000 is configure in 'Tiny' mode"));
  #else
    Serial.print(F("RX Buffer : "));
    Serial.print(CC3000_RX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
    Serial.print(F("TX Buffer : "));
    Serial.print(CC3000_TX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
  #endif
}

/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;
  
#ifndef CC3000_TINY_DRIVER  
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
void displayFirmwareVersion(void)
{
  #ifndef CC3000_TINY_DRIVER
  uint8_t major, minor;

  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
  }
  #endif
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
boolean MACvalid = false;
// array to store MAC address from EEPROM
uint8_t cMacFromEeprom[MAC_ADDR_LEN];

void displayMACAddress(void){
  if(!cc3000.getMacAddress(cMacFromEeprom))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
    MACvalid = false;
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&cMacFromEeprom, 6);
    MACvalid = true;    
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void)
{
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33]; 

  if (!cc3000.startSSIDscan(&index)) {
    Serial.println(F("SSID scan failed!"));
    return;
  }

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    
    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}
