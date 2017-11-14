### PPP over Serial (PPPoS) client example

Tested with **esp-idf v2.1** and latest esp-idf master branch

---

If using with latest esp-idf (e6afe28bafe5db5ab79fae213f2e8e1ccd9f937c or later) a patch for **components/lwip/api/pppapi.c** is need.

Use included **pppapi.c.patch** to patch the file or copy **pppapi.c.patched** to **components/lwip/api/** as **pppapi.c**

** This patch was merged into esp-idf on 14. Nov 2017**

---

**Includes GSM library** *libGSM* **which handles all GSM and PPPoS operations***

See *libGSM.h* for functions usage detailes.

---

#### How to build

Configure your esp32 build environment as for other **esp-idf examples**

Clone the repository

`git clone https://github.com/loboris/ESP32-PPPOS-EXAMPLE.git`

Execute menuconfig and configure your Serial flash config and other settings. Included *sdkconfig.defaults* sets some defaults to be used.

Navigate to **GSM PPPoS configuration** and set GSM and example parameters:

* **Enable GSM debugging** if set GSM AT commands and responses are printed
* **UART Output to GSM Module** UART Tx pin, connected to GSM Module Rx pin.
* **UART Input from GSM Module** UART Rx pin, connected to GSM Module Tx pin.
* **UART Baud rate** UART baudrate to comunicate with GSM module
* **Internet User** Network provider internet user.
* **Internet password** Network provider internet password
* **Internet APN** Network provider's APN for internet access
* **Start WiFi and AP** Enable WiFi, configure AP, start WebServer to test local WiFi and Internet over GSM/PPPoS connection
* **Send SMS message** if set SMS messages will be sent during example run
* **SMS number** SMS number for sending messages, enter the number in international format (+123999876543)
* **SMS message interval** Set SMS message interval in miliseconds

`make menuconfig`

Make and flash the example.

`make all && make flash`

---

Power your GSM module with good power supply, using LiPo battery is recommended.

In this example **hw flow controll** is not used. With 2G (GPRS) module it is not needed.
If using higher speed 3G module, using hw flow controll is recomended. You can configure it in *libGSM.c*

---

#### The example runs as follows:

1. Creates the **pppos client task** which initializes modem on UART port and handles lwip interaction
2. If configured, starts WiFi, Access Point and WebServer
3. When connection to the Internet is established, gets the current time using SNTP protocol
4. Creates **http**, **https** and **sms** tasks synchronized with mutex
5. **HTTP task** gets text file from server and displays the header and data
6. **HTTPS task** gets ssl info from server and displays the header and received JSON data with info about used SSL
7. **SMS task** sends SMS messages after defined interval has passed, checks and displays received messages. If received messages starts with **Esp32 info** sends the response message to senders number.
8. The tasks repeats operation after interval defined in *pppos_client_main.c*
9. If WiFi is configured, simple web page can be accessed on *http://192.168.4.1*. Generated page includes includes the text received from internet via PPPoS.


#### Tested with GSM SIM800L, should also work with other SIMCOM & Telit GSM modules.


**Example output:**

```
I (0) cpu_start: App cpu up.
I (1121) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1143) heap_alloc_caps: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM
I (1164) heap_alloc_caps: At 3FFB6F28 len 000290D8 (164 KiB): DRAM
I (1185) heap_alloc_caps: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (1206) heap_alloc_caps: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (1228) heap_alloc_caps: At 40092D0C len 0000D2F4 (52 KiB): IRAM
I (1249) cpu_start: Pro cpu start user code
I (1304) cpu_start: Starting scheduler on PRO CPU.
I (197) cpu_start: Starting scheduler on APP CPU.
I (297) [PPPOS CLIENT]: AT COMMAND: [AT..]
I (317) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (417) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=4..]
I (437) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (437) [PPPOS CLIENT]: GSM initialization start
I (1037) [PPPOS CLIENT]: AT COMMAND: [AT..]
I (1057) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (1157) [PPPOS CLIENT]: AT COMMAND: [ATZ..]
I (1177) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (1277) [PPPOS CLIENT]: AT COMMAND: [ATE0..]
I (1297) [PPPOS CLIENT]: AT RESPONSE: [ATE0...OK..]
I (1397) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=1..]
I (1417) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (2517) [PPPOS CLIENT]: AT COMMAND: [AT+CNMI=0,0,0,0,0..]
I (2537) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (2637) [PPPOS CLIENT]: AT COMMAND: [AT+CPIN?..]
I (2657) [PPPOS CLIENT]: AT RESPONSE: [..+CPIN: READY....OK..]
I (2757) [PPPOS CLIENT]: AT COMMAND: [AT+CREG?..]
I (2777) [PPPOS CLIENT]: AT BAD RESPONSE: [..+CREG: 0,2....OK..]
W (2777) [PPPOS CLIENT]: Wrong response, restarting...
I (5777) [PPPOS CLIENT]: Skip command: [AT..]
I (5777) [PPPOS CLIENT]: Skip command: [ATZ..]
I (5777) [PPPOS CLIENT]: Skip command: [ATE0..]
I (5777) [PPPOS CLIENT]: Skip command: [AT+CFUN=1..]
I (5777) [PPPOS CLIENT]: Skip command: [AT+CNMI=0,0,0,0,0..]
I (5787) [PPPOS CLIENT]: Skip command: [AT+CPIN?..]
I (5897) [PPPOS CLIENT]: AT COMMAND: [AT+CREG?..]
I (5917) [PPPOS CLIENT]: AT BAD RESPONSE: [..+CREG: 0,2....OK..]
W (5917) [PPPOS CLIENT]: Wrong response, restarting...
I (8917) [PPPOS CLIENT]: Skip command: [AT..]
I (8917) [PPPOS CLIENT]: Skip command: [ATZ..]
I (8917) [PPPOS CLIENT]: Skip command: [ATE0..]
I (8917) [PPPOS CLIENT]: Skip command: [AT+CFUN=1..]
I (8917) [PPPOS CLIENT]: Skip command: [AT+CNMI=0,0,0,0,0..]
I (8927) [PPPOS CLIENT]: Skip command: [AT+CPIN?..]
I (9037) [PPPOS CLIENT]: AT COMMAND: [AT+CREG?..]
I (9057) [PPPOS CLIENT]: AT RESPONSE: [..+CREG: 0,1....OK..]
I (19157) [PPPOS CLIENT]: AT COMMAND: [AT+CGDCONT=1,"IP","internet.ht.hr"..]
I (19177) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (19277) [PPPOS CLIENT]: AT COMMAND: [AT+CGDATA="PPP",1..]
I (19297) [PPPOS CLIENT]: AT RESPONSE: [..CONNECT..]
I (20297) [PPPOS CLIENT]: GSM initialized.
I (21077) [PPPOS CLIENT]: status_cb: Connected
I (21077) [PPPOS CLIENT]:    ipaddr    = 10.229.68.97
I (21077) [PPPOS CLIENT]:    gateway   = 10.64.64.64
I (21077) [PPPOS CLIENT]:    netmask   = 255.255.255.255
I (21087) [PPPOS CLIENT]:    ip6addr   = ::

I (21087) [SNTP]: OBTAINING TIME
I (21097) [SNTP]: Initializing SNTP
I (21107) [SNTP]: SNTP INITIALIZED
I (21107) [SNTP]: Waiting for system time to be set... (1/10)
I (23107) [SNTP]: TIME SET TO Mon Jun 19 09:30:55 2017

I (23117) [HTTP]: ===== HTTP GET REQUEST =========================================

I (23497) [HTTP]: DNS lookup succeeded. IP=82.196.4.208
I (23497) [HTTP]: ... allocated socket

I (23787) [HTTP]: ... connected
I (23797) [HTTP]: ... socket send success
I (23797) [HTTP]: ... reading HTTP response...
Header:
-------
HTTP/1.1 200 OK
Date: Mon, 19 Jun 2017 09:30:57 GMT
Server: Apache/2.4.7 (Ubuntu)
Last-Modified: Sat, 18 Mar 2017 17:32:44 GMT
ETag: "149-54b04ae918eb8"
Accept-Ranges: bytes
Content-Length: 329
Vary: Accept-Encoding
Content-Type: text/plain
-------
Data:
-----
Welcome to ESP32
The ESP32 is a low cost, low power microcontroller with integrated Wi-Fi & dual-mode Bluetooth,
which employs a dual-core Tensilica Xtensa LX6 microprocessor.
ESP32 is created and developed by Espressif Systems, a Shanghai-based Chinese company,
and is manufactured by TSMC using their 40 nm process.

2017 LoBo

-----
I (29267) [HTTP]: ... done reading from socket. 581 bytes read, 581 in buffer, errno=22


I (31317) [PPPOS CLIENT]: Disconnect requested.
W (31347) [PPPOS CLIENT]: status_cb: User interrupt (disconnected)
I (32447) [PPPOS CLIENT]: AT COMMAND: [AT..]
I (32467) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (32567) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=4..]
I (35057) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (35057) [PPPOS CLIENT]: Disconnected.
I (35127) [HTTP]: Waiting 300 sec...
I (35127) [HTTP]: ================================================================


I (35127) [HTTPS]: Seeding the random number generator
I (35127) [HTTPS]: Loading the CA root certificate...
I (35137) [HTTPS]: Setting hostname for TLS session...
I (35137) [HTTPS]: Setting up the SSL/TLS structure...

I (35157) [PPPOS CLIENT]: Reconnect requested.
I (35157) [PPPOS CLIENT]: GSM initialization start
I (35757) [PPPOS CLIENT]: AT COMMAND: [AT..]
I (35777) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (35877) [PPPOS CLIENT]: AT COMMAND: [ATZ..]
I (35897) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (35997) [PPPOS CLIENT]: AT COMMAND: [ATE0..]
I (36017) [PPPOS CLIENT]: AT RESPONSE: [ATE0...OK..]
I (36117) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=1..]
I (36137) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (37237) [PPPOS CLIENT]: AT COMMAND: [AT+CNMI=0,0,0,0,0..]
I (37257) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (37357) [PPPOS CLIENT]: AT COMMAND: [AT+CPIN?..]
I (37377) [PPPOS CLIENT]: AT RESPONSE: [..+CPIN: READY....OK..]
I (37477) [PPPOS CLIENT]: AT COMMAND: [AT+CREG?..]
I (37497) [PPPOS CLIENT]: AT BAD RESPONSE: [..+CREG: 0,2....OK..]
W (37497) [PPPOS CLIENT]: Wrong response, restarting...
I (40497) [PPPOS CLIENT]: Skip command: [AT..]
I (40497) [PPPOS CLIENT]: Skip command: [ATZ..]
I (40497) [PPPOS CLIENT]: Skip command: [ATE0..]
I (40497) [PPPOS CLIENT]: Skip command: [AT+CFUN=1..]
I (40507) [PPPOS CLIENT]: Skip command: [AT+CNMI=0,0,0,0,0..]
I (40507) [PPPOS CLIENT]: Skip command: [AT+CPIN?..]
I (40617) [PPPOS CLIENT]: AT COMMAND: [AT+CREG?..]
I (40637) [PPPOS CLIENT]: AT RESPONSE: [..+CREG: 0,1....OK..]
I (40737) [PPPOS CLIENT]: AT COMMAND: [AT+CGDCONT=1,"IP","internet.ht.hr"..]
I (40757) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (40857) [PPPOS CLIENT]: AT COMMAND: [AT+CGDATA="PPP",1..]
I (40877) [PPPOS CLIENT]: AT RESPONSE: [..CONNECT..]
I (41877) [PPPOS CLIENT]: GSM initialized.
I (42917) [PPPOS CLIENT]: status_cb: Connected
I (42917) [PPPOS CLIENT]:    ipaddr    = 10.226.211.69
I (42917) [PPPOS CLIENT]:    gateway   = 10.64.64.64
I (42917) [PPPOS CLIENT]:    netmask   = 255.255.255.255
I (42927) [PPPOS CLIENT]:    ip6addr   = ::
I (42937) [HTTPS]: ===== HTTPS GET REQUEST =========================================

I (42937) [HTTPS]: Connecting to www.howsmyssl.com:443...
I (43787) [HTTPS]: Connected.
I (43787) [HTTPS]: Performing the SSL/TLS handshake...
I (46807) [HTTPS]: Verifying peer X.509 certificate...
W (46807) [HTTPS]: Failed to verify peer certificate!
W (46807) [HTTPS]: verification info:   ! The certificate Common Name (CN) does not match with the expected CN

I (46817) [HTTPS]: Writing HTTP request...
I (46827) [HTTPS]: 102 bytes written
I (46827) [HTTPS]: Reading HTTP response...
I (48007) [HTTPS]: 5524 bytes read, 5524 in buffer
Header:
-------
HTTP/1.1 200 OK
Content-Length: 5289
Access-Control-Allow-Origin: *
Connection: close
Content-Type: application/json
Date: Mon, 19 Jun 2017 09:31:19 GMT
Strict-Transport-Security: max-age=631138519; includeSubdomains; preload
-------
I (48027) [HTTPS]: JSON data received.
I (48037) [HTTPS]: parsing JSON data:
given_cipher_suites = [Array] of 131 items
   TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
   TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
   TLS_DHE_RSA_WITH_AES_256_GCM_SHA384
   + 128 more...
ephemeral_keys_supported = True
session_ticket_supported = True
tls_compression_supported = False
unknown_cipher_suite_supported = False
beast_vuln = False
able_to_detect_n_minus_one_splitting = False
insecure_cipher_suites = {Object}
tls_version = TLS 1.2
rating = Probably Okay

I (50087) [PPPOS CLIENT]: Disconnect requested.
W (50117) [PPPOS CLIENT]: status_cb: User interrupt (disconnected)
I (51217) [PPPOS CLIENT]: AT COMMAND: [AT..]
I (51237) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (51337) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=4..]
I (53797) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (53797) [PPPOS CLIENT]: Disconnected.
I (53797) [HTTPS]: Waiting 300 sec...
I (53797) [HTTPS]: =================================================================


I (53807) [SMS]: ===== SMS TEST =================================================

I (53917) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (56117) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=1..]
I (56137) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (58237) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (58257) [PPPOS CLIENT]: AT RESPONSE: [..+CFUN: 1....OK..]
I (58357) [PPPOS CLIENT]: AT COMMAND: [AT+CMGF=1..]
I (58377) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (58477) [PPPOS CLIENT]: AT COMMAND: [AT+CMGS="+38599xxxxxxx"..]
I (58497) [PPPOS CLIENT]: AT RESPONSE: [..> ]
I (58597) [PPPOS CLIENT]: AT COMMAND: [Hi from ESP32 via GSM.This is the test message..]
I (65617) [PPPOS CLIENT]: AT RESPONSE: [..+CMGS: 35....OK..]
SMS sent successfully
I (65717) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (65737) [PPPOS CLIENT]: AT RESPONSE: [..+CFUN: 1....OK..]
I (65837) [PPPOS CLIENT]: AT COMMAND: [AT+CMGF=1..]
I (65857) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (65957) [PPPOS CLIENT]: AT COMMAND: [AT+CMGL="ALL"..]

No messages
I (68157) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (70357) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=4..]
I (74097) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (74097) [SMS]: Waiting 300 sec...
I (74097) [SMS]: ================================================================

.
.
.

I (4238867) [SMS]: ===== SMS TEST =================================================

I (4238977) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (4241177) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=1..]
I (4241197) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (4243297) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (4243317) [PPPOS CLIENT]: AT RESPONSE: [..+CFUN: 1....OK..]
I (4243417) [PPPOS CLIENT]: AT COMMAND: [AT+CMGF=1..]
I (4243437) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (4243537) [PPPOS CLIENT]: AT COMMAND: [AT+CMGL="ALL"..]

Received messages: 1
-------------------------------------------
Message #1: idx=1, from: +38599xxxxxxx, status: REC UNREAD, time: 17/06/19,12:37:52+08, tz=GMT+2, timestamp: Wed Jul 19 12:37:52 2017

Text: [
Esp32 info, send me a reply
]

I (4245807) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (4245827) [PPPOS CLIENT]: AT RESPONSE: [..+CFUN: 1....OK..]
I (4245927) [PPPOS CLIENT]: AT COMMAND: [AT+CMGF=1..]
I (4245947) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (4246047) [PPPOS CLIENT]: AT COMMAND: [AT+CMGS="+38599xxxxxxx"..]
I (4246067) [PPPOS CLIENT]: AT RESPONSE: [..> ]
I (4246167) [PPPOS CLIENT]: AT COMMAND: [Hi, +38599xxxxxxx.My time is now.06/19/17 10:41:17.]
I (4250777) [PPPOS CLIENT]: AT RESPONSE: [..+CMGS: 38....OK..]
Response sent successfully
Delete message at index 1
I (4250877) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (4250897) [PPPOS CLIENT]: AT RESPONSE: [..+CFUN: 1....OK..]
I (4250997) [PPPOS CLIENT]: AT COMMAND: [AT+CMGF=1..]
I (4251017) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (4251117) [PPPOS CLIENT]: AT COMMAND: [AT+CMGD=1..]
I (4251227) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
Delete OK
I (4251327) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN?..]
I (4253527) [PPPOS CLIENT]: AT COMMAND: [AT+CFUN=4..]
I (4254357) [PPPOS CLIENT]: AT RESPONSE: [..OK..]
I (4254357) [SMS]: Waiting 300 sec...
I (4254357) [SMS]: ================================================================


```
