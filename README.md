#PPP over Serial (PPPoS) client example

This example uses the [pppos patches](https://github.com/amuzyka-grinn/esp-idf) proposed by amuzyka-grinn.

Until those patches are not pulled to esp-idf, you must patch your esp-idf.

##This is the required procedure:

1. copy all files from patches directory to the root of your esp-idf
2. go to esp-idf directory and execute 'apply_ppp_patch.sh'
3. go back to this example directory
4. as with other ESP32 examples, set IDF-PATH variable and add 'xtensa-esp32-elf/bin' to your path
5. execute make menuconfig, enable PPP support (Component config → LWIP → Enable PPP support)
6. execute make all to build the example
7. execute make flash to flash the example

##You can execute 'revert_ppp_patch.sh' in esp-idf to revert the changes before updating you esp-idf with 'git pull'


It shows example of ppp client using lwip PPPoS api and GSM.

Before you run this example, make sure your GSM is in command mode and is registered to network.

Tested with GSM SIM800L.

