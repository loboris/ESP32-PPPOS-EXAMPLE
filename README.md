### PPP over Serial (PPPoS) client example

This example uses the [pppos patches](https://github.com/amuzyka-grinn/esp-idf) proposed by amuzyka-grinn.

Until those patches are not pulled to esp-idf, you must patch your **esp-idf**.

#### This is the required procedure:

1. copy all files from patches directory to the root of your esp-idf
2. go to esp-idf directory and execute *apply_ppp_patch.sh*
3. go back to this example directory
4. as with other ESP32 examples, set IDF-PATH variable and add 'xtensa-esp32-elf/bin' to your path
5. execute **make menuconfig**, enable **PPP support** (Component config → LWIP → Enable PPP support) and **configure your APN** (Component config → GSM configuration)
6. execute **make all** to build the example
7. execute **make flash** to flash the example

#### You can execute 'revert_ppp_patch.sh' in esp-idf to revert the changes before updating you esp-idf with 'git pull'


Before you run this example, make sure your GSM is powered on, in command mode, registered to network and connected to your ESP32 UART on pins #16 & #17 (hw flow controll is not used). 

The example runs as follows:

1. Creates the pppos client task which initializes modem on UART port and handles lwip interaction
2. When connection to the Internet is established, gets the current time using SNTP protocol
3. Creates http and https tasks synchronized with mutex
4. HTTP task gets text file from server and displays the header and data
5. HTTPS task gets ssl info from server and displays the header and received JSON data with info about used SSL
6. HTTP/HTTPS tasks repeats operation every 30 seconds


#### Tested with GSM SIM800L, should also work with other SIMCOM & Telit GSM modules.

