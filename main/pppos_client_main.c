/* PPPoS Client Example with GSM
 *  (tested with SIM800)
 *  Author: LoBo (loboris@gmail.com, loboris.github)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */


#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/semphr.h"

#include "driver/uart.h"

#include "netif/ppp/pppos.h"
#include "netif/ppp/ppp.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/pppapi.h"

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include <esp_event.h>
#include <esp_wifi.h>

#include "apps/sntp/sntp.h"
#include "cJSON.h"

#include "libGSM.h"

#ifdef CONFIG_GSM_USE_WIFI_AP
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#endif

#define EXAMPLE_TASK_PAUSE	300		// pause between task runs in seconds
#define TASK_SEMAPHORE_WAIT 140000	// time to wait for mutex in miliseconds

QueueHandle_t http_mutex;

static const char *TIME_TAG = "[SNTP]";
static const char *HTTP_TAG = "[HTTP]";
static const char *HTTPS_TAG = "[HTTPS]";

#ifdef CONFIG_GSM_SEND_SMS
static const char *SMS_TAG = "[SMS]";
#endif

#ifdef CONFIG_GSM_USE_WIFI_AP
static const char *WEBSRV_TAG = "[WebServer]";
const static char http_html_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const static char http_index_html[] = "<!DOCTYPE html>"
                                     "<html>\n"
                                     "<head>\n"
                                     "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                                     "  <style type=\"text/css\">\n"
                                     "    html, body, iframe { margin: 0; padding: 0; height: 100%; }\n"
                                     "    iframe { display: block; width: 100%; border: none; }\n"
                                     "  </style>\n"
                                     "<title>HELLO ESP32</title>\n"
                                     "</head>\n"
                                     "<body>\n"
                                     "<h1>Hello World, from ESP32!</h1>\n";
#endif

// ===============================================================================================
// ==== Http/Https get requests ==================================================================
// ===============================================================================================

// Constants that aren't configurable in menuconfig
#define WEB_SERVER "loboris.eu"
#define WEB_PORT 80
#define WEB_URL "http://loboris.eu/ESP32/info.txt"
#define WEB_URL1 "http://loboris.eu"

#define SSL_WEB_SERVER "www.howsmyssl.com"
#define SSL_WEB_URL "https://www.howsmyssl.com/a/check"
#define SSL_WEB_PORT "443"

static const char *REQUEST = "GET " WEB_URL " HTTP/1.1\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static const char *REQUEST1 = "GET " WEB_URL1 " HTTP/1.1\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static const char *SSL_REQUEST = "GET " SSL_WEB_URL " HTTP/1.1\r\n"
    "Host: "SSL_WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

/* Root cert for howsmyssl.com, taken from server_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

static int level = 0;
//-----------------------------------
static void parse_object(cJSON *item)
{
    ESP_LOGI(HTTPS_TAG, "JSON: level %d", level);
	cJSON *subitem=item->child;
	while (subitem)
	{
		printf("%s = ", subitem->string);
		if (subitem->type == cJSON_String) printf("%s\r\n", subitem->valuestring);
		else if (subitem->type == cJSON_Number) printf("%d\r\n", subitem->valueint);
		else if (subitem->type == cJSON_False) printf("False\r\n");
		else if (subitem->type == cJSON_True) printf("True\r\n");
		else if (subitem->type == cJSON_NULL) printf("NULL\r\n");
		else if (subitem->type == cJSON_Object) {
            printf("{Object}\r\n");
            // handle subitem
            if (subitem->child) {
                cJSON *subitemData = cJSON_GetObjectItem(subitem,subitem->string);
                if (subitemData) {
                    level++;
                    parse_object(subitemData);
                    level--;
                }
            }
        }
		else if (subitem->type == cJSON_Array) {
			int arr_count = cJSON_GetArraySize(subitem);
			printf("[Array] of %d items\r\n", arr_count);
			int n;
			for (n = 0; n < 3; n++) {
				// Get the JSON element and then get the values as before
				cJSON *arritem = cJSON_GetArrayItem(subitem, n);
				if ((arritem) && (arritem->valuestring)) printf("   %s\n", arritem->valuestring);
				else break;
			}
			if (arr_count > 3 ) printf("   + %d more...\r\n", arr_count-3);
		}
		else printf("[?]\r\n");

		subitem=subitem->next;
	}
}

//============================================
static void https_get_task(void *pvParameters)
{
	if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
		ESP_LOGE(HTTPS_TAG, "*** ERROR: CANNOT GET MUTEX ***n");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	char buf[512];
    char *buffer;
    int ret, flags, len, rlen=0, totlen=0;

	buffer = malloc(8192);
	if (!buffer) {
		xSemaphoreGive(http_mutex);
		ESP_LOGE(HTTPS_TAG, "*** ERROR allocating receive buffer ***");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}
	
	mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    ESP_LOGI(HTTPS_TAG, "Seeding the random number generator");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    NULL, 0)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
		xSemaphoreGive(http_mutex);
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
    }

    ESP_LOGI(HTTPS_TAG, "Loading the CA root certificate...");

    ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
                                 server_root_cert_pem_end-server_root_cert_pem_start);

    if(ret < 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
		xSemaphoreGive(http_mutex);
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
    }

    ESP_LOGI(HTTPS_TAG, "Setting hostname for TLS session...");

    // Host name set here should match CN in server certificate
    if((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
		xSemaphoreGive(http_mutex);
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
    }

    ESP_LOGI(HTTPS_TAG, "Setting up the SSL/TLS structure...");

    if((ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }

    // MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
    //   a warning if CA verification fails but it will continue to connect.
    //   You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }

    goto start;

    while(1) {
		if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
			ESP_LOGE(HTTPS_TAG, "===== ERROR: CANNOT GET MUTEX ===================================\n");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
			continue;
		}
start:
        // ** We must be connected to Internet
        if (ppposInit() == 0) goto finished;

        ESP_LOGI(HTTPS_TAG, "===== HTTPS GET REQUEST =========================================\n");
		//netif_set_default(&ppp_netif);

        mbedtls_net_init(&server_fd);

        ESP_LOGI(HTTPS_TAG, "Connecting to %s:%s...", SSL_WEB_SERVER, SSL_WEB_PORT);

        if ((ret = mbedtls_net_connect(&server_fd, SSL_WEB_SERVER,
                                      SSL_WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
        {
            ESP_LOGE(HTTPS_TAG, "mbedtls_net_connect returned -%x", -ret);
            goto exit;
        }

        ESP_LOGI(HTTPS_TAG, "Connected.");

        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        ESP_LOGI(HTTPS_TAG, "Performing the SSL/TLS handshake...");

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
        {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
                goto exit;
            }
        }

        ESP_LOGI(HTTPS_TAG, "Verifying peer X.509 certificate...");

        if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
        {
            // In real life, we probably want to close connection if ret != 0
            ESP_LOGW(HTTPS_TAG, "Failed to verify peer certificate!");
            bzero(buf, sizeof(buf));
            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
            ESP_LOGW(HTTPS_TAG, "verification info: %s", buf);
        }
        else {
            ESP_LOGI(HTTPS_TAG, "Certificate verified.");
        }

        ESP_LOGI(HTTPS_TAG, "Writing HTTP request...");

        while((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)SSL_REQUEST, strlen(SSL_REQUEST))) <= 0)
        {
            if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_write returned -0x%x", -ret);
                goto exit;
            }
        }

        len = ret;
        ESP_LOGI(HTTPS_TAG, "%d bytes written", len);
        ESP_LOGI(HTTPS_TAG, "Reading HTTP response...");

		rlen = 0;
		totlen = 0;
        do
        {
            len = sizeof(buf) - 1;
            bzero(buf, sizeof(buf));
            ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

            if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
                continue;

            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                ret = 0;
                break;
            }

            if(ret < 0)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_read returned -0x%x", -ret);
                break;
            }

            if(ret == 0)
            {
                ESP_LOGI(HTTPS_TAG, "connection closed");
                break;
            }

            len = ret;
            //ESP_LOGI(HTTPS_TAG, "%d bytes read", len);
			totlen += len;
			if ((rlen + len) < 8192) {
				memcpy(buffer+rlen, buf, len);
				rlen += len;
			}
        } while(1);

        mbedtls_ssl_close_notify(&ssl);

    exit:
        mbedtls_ssl_session_reset(&ssl);
        mbedtls_net_free(&server_fd);

        ESP_LOGI(HTTPS_TAG, "%d bytes read, %d in buffer", totlen, rlen);
        if(ret != 0)
        {
            mbedtls_strerror(ret, buf, 100);
            ESP_LOGE(HTTPS_TAG, "Last error was: -0x%x - %s", -ret, buf);
        }

        buffer[rlen] = '\0';
        char *json_ptr = strstr(buffer, "{\"given_cipher_suites\":");
        char *hdr_end_ptr = strstr(buffer, "\r\n\r\n");
		if (hdr_end_ptr) {
			*hdr_end_ptr = '\0';
			printf("Header:\r\n-------\r\n%s\r\n-------\r\n", buffer);
		}
		if (json_ptr) {
			ESP_LOGI(HTTPS_TAG, "JSON data received.");
            //printf("JSON: [\n%s]\n", json_ptr);
            //ToDo: Check json in latest esp-idf
			cJSON *root = cJSON_Parse(json_ptr);
			ESP_LOGI(HTTPS_TAG, "JSON string parsed.");
			if (root) {
				ESP_LOGI(HTTPS_TAG, "parsing JSON data:");
                level = 1;
				parse_object(root);
				cJSON_Delete(root);
			}
		}

		// We can disconnect from Internet now and turn off RF to save power
		//ppposDisconnect(0, 1);

finished:
        ESP_LOGI(HTTPS_TAG, "Waiting %d sec...", EXAMPLE_TASK_PAUSE);
        ESP_LOGI(HTTPS_TAG, "=================================================================\n\n");
		xSemaphoreGive(http_mutex);
        for(int countdown = EXAMPLE_TASK_PAUSE; countdown >= 0; countdown--) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}


//===========================================
static void http_get_task(void *pvParameters)
{
	if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
		ESP_LOGE(HTTP_TAG, "*** ERROR: CANNOT GET MUTEX ***n");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[128];
    char *buffer;
    int rlen=0, totlen=0;

	buffer = malloc(2048);
	if (!buffer) {
		ESP_LOGE(HTTPS_TAG, "*** ERROR allocating receive buffer ***");
		xSemaphoreGive(http_mutex);
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	goto start;

    while(1) {
        if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
			ESP_LOGE(HTTP_TAG, "===== ERROR: CANNOT GET MUTEX ==================================\n");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
			continue;
		}
start:
        // ** We must be connected to Internet
        if (ppposInit() == 0) goto finished;

		ESP_LOGI(HTTP_TAG, "===== HTTP GET REQUEST =========================================\n");

		//netif_set_default(&ppp_netif);
        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(HTTP_TAG, "DNS lookup failed err=%d res=%p", err, res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(HTTP_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(HTTP_TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... allocated socket\r\n");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(HTTP_TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(HTTP_TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(HTTP_TAG, "... socket send failed");
            close(s);
            xSemaphoreGive(http_mutex);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... socket send success");
        ESP_LOGI(HTTP_TAG, "... reading HTTP response...");

        memset(buffer, 0, 2048);
        /* Read HTTP response */
        int opt = 500;
        int first_block = 1;
        char *cont_len = NULL;
        int clen = 0;
        rlen = 0;
        totlen = 0;
        char *hdr_end_ptr = NULL;
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            totlen += r;
            if ((rlen + r) < 2048) {
                memcpy(buffer+rlen, recv_buf, r);
                rlen += r;
                buffer[rlen] = '\0';
                if (clen == 0) {
                    cont_len = strstr(buffer, "Content-Length: ");
                    if (cont_len) {
                        cont_len += 16;
                        if (strstr(cont_len, "\r\n")) {
                            char slen[16] = {0};
                            memcpy(slen, cont_len, strstr(cont_len, "\r\n")-cont_len);
                            clen = atoi(slen);
                        }
                    }
                }
                if (hdr_end_ptr == NULL) {
                    hdr_end_ptr = strstr(buffer, "\r\n\r\n");
                    if (hdr_end_ptr) {
                        *hdr_end_ptr = '\0';
                        hdr_end_ptr += 4;
                    }
                }
                else if (clen > 0) {
                    if (strlen(hdr_end_ptr) >= clen) break; 
                }
            }
            if (first_block) {
                lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(int));
                first_block = 0;
            }
        } while(r > 0);

		if (hdr_end_ptr) {
			printf("Header:\r\n-------\r\n%s\r\n-------\r\n", buffer);
			printf("Data:\r\n-----\r\n%s\r\n-----\r\n", hdr_end_ptr);
		}
        ESP_LOGI(HTTP_TAG, "... done reading from socket. %d bytes read, %d in buffer, errno=%d\r\n", totlen, rlen, errno);
        close(s);

        // We can disconnect from Internet now and turn off RF to save power
		//ppposDisconnect(0, 1);

finished:
        ESP_LOGI(HTTP_TAG, "Waiting %d sec...", EXAMPLE_TASK_PAUSE);
        ESP_LOGI(HTTP_TAG, "================================================================\n\n");
		xSemaphoreGive(http_mutex);
        for(int countdown = EXAMPLE_TASK_PAUSE; countdown >= 0; countdown--) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

#ifdef CONFIG_GSM_SEND_SMS

//======================================
static void sms_task(void *pvParameters)
{
	if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
		ESP_LOGE(SMS_TAG, "*** ERROR: CANNOT GET MUTEX ***n");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	SMS_Messages messages;
	uint32_t sms_time = 0;
	char buf[160];

	goto start;

	while(1) {
        if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
			ESP_LOGE(SMS_TAG, "===== ERROR: CANNOT GET MUTEX ==================================\n");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
			continue;
		}
start:
		ESP_LOGI(SMS_TAG, "===== SMS TEST =================================================\n");

		// ** For SMS operations we have to off line **
		ppposDisconnect(0, 0);
		gsm_RFOn();  // Turn on RF if it was turned off
		vTaskDelay(2000 / portTICK_RATE_MS);

		if (clock() > sms_time) {
			if (smsSend(CONFIG_GSM_SMS_NUMBER, "Hi from ESP32 via GSM\rThis is the test message.") == 1) {
				printf("SMS sent successfully\r\n");
			}
			else {
				printf("SMS send failed\r\n");
			}
			sms_time = clock() + CONFIG_GSM_SMS_INTERVAL; // next sms send time
		}

		smsRead(&messages, -1);
		if (messages.nmsg) {
			printf("\r\nReceived messages: %d\r\n", messages.nmsg);
			SMS_Msg *msg;
			for (int i=0; i<messages.nmsg; i++) {
				msg = messages.messages + (i * sizeof(SMS_Msg));
				struct tm * timeinfo;
				timeinfo = localtime (&msg->time_value );
				printf("-------------------------------------------\r\n");
				printf("Message #%d: idx=%d, from: %s, status: %s, time: %s, tz=GMT+%d, timestamp: %s\r\n",
						i+1, msg->idx, msg->from, msg->stat, msg->time, msg->tz, asctime(timeinfo));
				printf("Text: [\r\n%s\r\n]\r\n\r\n", msg->msg);

				// Check if SMS text contains known command
				if (strstr(msg->msg, "Esp32 info") == msg->msg) {
					char buffer[80];
					time_t rawtime;
					time(&rawtime);
					timeinfo = localtime( &rawtime );
					strftime(buffer,80,"%x %H:%M:%S", timeinfo);
					sprintf(buf, "Hi, %s\rMy time is now\r%s", msg->from, buffer);
					if (smsSend(CONFIG_GSM_SMS_NUMBER, buf) == 1) {
						printf("Response sent successfully\r\n");
					}
					else {
						printf("Response send failed\r\n");
					}
				}
				// Free allocated message text buffer
				if (msg->msg) free(msg->msg);
				if ((i+1) == messages.nmsg) {
					printf("Delete message at index %d\r\n", msg->idx);
					if (smsDelete(msg->idx) == 0) printf("Delete ERROR\r\n");
					else printf("Delete OK\r\n");
				}
			}
			free(messages.messages);
		}
		else printf("\r\nNo messages\r\n");

		// ** We can turn off GSM RF to save power
		gsm_RFOff();
		// ** We can now go back on line, or stay off line **
        //ppposInit();

        ESP_LOGI(SMS_TAG, "Waiting %d sec...", EXAMPLE_TASK_PAUSE);
        ESP_LOGI(SMS_TAG, "================================================================\n\n");

        xSemaphoreGive(http_mutex);
        for(int countdown = EXAMPLE_TASK_PAUSE; countdown >= 0; countdown--) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
#endif


#ifdef CONFIG_GSM_USE_WIFI_AP

// ==== WiFi handling & simple WebServer ====================================================

// FreeRTOS event group to signal when we are connected & ready to make a request
static EventGroupHandle_t wifi_event_group;

// The event group allows multiple bits for each event,
const int CONNECTED_BIT = BIT0;
const int APSTARTED_BIT = BIT1;

//-------------------------------------------------------------------
esp_err_t esp32_wifi_eventHandler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_START:
        xEventGroupSetBits(wifi_event_group, APSTARTED_BIT);
        ESP_LOGD(WEBSRV_TAG, "AP Started");
        break;
    case SYSTEM_EVENT_AP_STOP:
        xEventGroupClearBits(wifi_event_group, APSTARTED_BIT);
        ESP_LOGD(WEBSRV_TAG, "AP Stopped");
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGD(WEBSRV_TAG, "station connected to access point. Connected stations:");
        wifi_sta_list_t sta_list;
        ESP_ERROR_CHECK( esp_wifi_ap_get_sta_list(&sta_list));
        for(int i = 0; i < sta_list.num; i++) {
            //Print the mac address of the connected station
            wifi_sta_info_t sta = sta_list.sta[i];
            ESP_LOGD(WEBSRV_TAG, "Station %d MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", i, sta.mac[0], sta.mac[1], sta.mac[2], sta.mac[3], sta.mac[4], sta.mac[5]);
        }
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGD(WEBSRV_TAG, "station disconnected from access point");
        break;
    default:
        break;
    }
	return ESP_OK;
}

//----------------------------------------------------------------------------------------
static void http_get_fromInternet(struct netconn *conn, const char *request, uint8_t type)
{
    ESP_LOGD(WEBSRV_TAG, "Get data from Internet");
	const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    int s, r;
    char recv_buf[128];
    int rlen=0, totlen=0;
    char *buffer = NULL;
    char *hdr_end_ptr = NULL;

    // =======================================================
    // Other tasks may use PPPoS, wait until they are finished
    // =======================================================
    if (!(xSemaphoreTake(http_mutex, TASK_SEMAPHORE_WAIT))) {
        ESP_LOGE(WEBSRV_TAG, "ERROR: CANNOT GET MUTEX");
        return;
    }

    // ** We must be connected to Internet
    if (ppposInit() == 0) {
        ESP_LOGE(WEBSRV_TAG, "PPPoS not initialized");
        xSemaphoreGive(http_mutex);
        return;
    }

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);
    if(err != 0 || res == NULL) {
        ESP_LOGE(WEBSRV_TAG, "DNS lookup failed err=%d res=%p", err, res);
        xSemaphoreGive(http_mutex);
        return;
    }

    // Create socket and connect to http server
    s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0) {
        ESP_LOGE(WEBSRV_TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        xSemaphoreGive(http_mutex);
        return;
    }

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(WEBSRV_TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        xSemaphoreGive(http_mutex);
        return;
    }

    freeaddrinfo(res);

    // Send GET request
    if (write(s, request, strlen(request)) < 0) {
        ESP_LOGE(WEBSRV_TAG, "... socket send failed");
        close(s);
        xSemaphoreGive(http_mutex);
        return;
    }

    // Get the response to buffer
    buffer = malloc(2048);
    if (!buffer) {
        ESP_LOGE(WEBSRV_TAG, "ERROR allocating receive buffer");
        close(s);
        buffer = NULL;
        xSemaphoreGive(http_mutex);
        return;
    }
    memset(buffer, 0, 2048);
    /* Read HTTP response */
    int opt = 500;
    int first_block = 1;
    char *cont_len = NULL;
    int clen = 0;
    rlen = 0;
    totlen = 0;
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);
        totlen += r;
        if ((rlen + r) < 2048) {
            memcpy(buffer+rlen, recv_buf, r);
            rlen += r;
            buffer[rlen] = '\0';
            if (clen == 0) {
                cont_len = strstr(buffer, "Content-Length: ");
                if (cont_len) {
                    cont_len += 16;
                    if (strstr(cont_len, "\r\n")) {
                        char slen[16] = {0};
                        memcpy(slen, cont_len, strstr(cont_len, "\r\n")-cont_len);
                        clen = atoi(slen);
                    }
                }
            }
            if (hdr_end_ptr == NULL) {
                hdr_end_ptr = strstr(buffer, "\r\n\r\n");
                if (hdr_end_ptr) hdr_end_ptr += 4;
            }
            else if (clen > 0) {
                if (strlen(hdr_end_ptr) >= clen) break; 
            }
        }
        if (first_block) {
            lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(int));
            first_block = 0;
        }
    } while(r > 0);

    close(s);
    xSemaphoreGive(http_mutex);

    // Send the response to the client connected via WiFi AP
    if (hdr_end_ptr) {
        ESP_LOGI(WEBSRV_TAG, "Received data: rlen=%d, total=%d, body=%d [%d]", rlen, totlen, strlen(hdr_end_ptr), clen);
    }
    else {
        ESP_LOGE(WEBSRV_TAG, "No header end found, rlen=%d", rlen);
        free(buffer);
        buffer = NULL;
        return;
    }

    if (type) netconn_write(conn, "<h4>Received via PPPoS:</h4>\r\n<pre><h5>", 39, NETCONN_NOCOPY);
    netconn_write(conn, hdr_end_ptr, strlen(hdr_end_ptr), NETCONN_NOCOPY);
    if (type) netconn_write(conn, "</h5></pre>\r\n", 13, NETCONN_NOCOPY);
    free(buffer);
    buffer = NULL;

    return;
}


//-----------------------------------------------------------
static void http_server_netconn_serve(struct netconn *conn) {

    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    err_t err;

    err = netconn_recv(conn, &inbuf);

    if (err == ERR_OK) {
        
        netbuf_data(inbuf, (void**)&buf, &buflen);
        
        // extract the first line, with the request
        char *first_line = strtok(buf, "\n");
        
        if (first_line) {
            // default page
            if (strstr(first_line, "GET / ")) {
                netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                netconn_write(conn, http_index_html, sizeof(http_index_html) - 1, NETCONN_NOCOPY);
                http_get_fromInternet(conn, REQUEST, 1);
                netconn_write(conn, "</body>\n</html>\n", 16, NETCONN_NOCOPY);
                ESP_LOGI(WEBSRV_TAG, "Got request from client");
            }
            else if (strstr(first_line, "GET /test ")) {
                http_get_fromInternet(conn, REQUEST1, 0);
                ESP_LOGI(WEBSRV_TAG, "Got [test] request from client");
            }
            else {
                ESP_LOGW(WEBSRV_TAG, "Unknown request type [%s]", first_line);
            }
        }
        else {
            ESP_LOGE(WEBSRV_TAG, "Unknown request");
        }
    }

    // close the connection and free the buffer
    netconn_close(conn);
    netbuf_delete(inbuf);
}

//-------------------------------------------
static void http_server(void *pvParameters) {
	
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
    ESP_LOGD(WEBSRV_TAG, "HTTP Server listening...");
	do {
		err = netconn_accept(conn, &newconn);
	    ESP_LOGD(WEBSRV_TAG, "New client connected");
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
	} while(err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
    ESP_LOGE(WEBSRV_TAG, "Netconn accept error, task halted!");
}

// ==========================================================================================

#endif


//=============
void app_main()
{
    http_mutex = xSemaphoreCreateMutex();

	#ifdef CONFIG_GSM_USE_WIFI_AP
	// ----- Set AP(STA)---------------------------------------------------
	nvs_flash_init();
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK( esp_event_loop_init(esp32_wifi_eventHandler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
	wifi_config_t apConfig = {
	   .ap = {
		  .ssid="ESP32_TESTAP",
		  .ssid_len=0,
		  .password="",
		  .channel=0,
		  .authmode=WIFI_AUTH_OPEN,
		  .ssid_hidden=0,
		  .max_connection=8,
		  .beacon_interval=100
	   },
	};
	ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_AP, &apConfig) );

    //wifi_config_t staConfig = {
	//   .sta = {
	//	   .ssid = "MySSID",
	//	   .password = "MyPassword",
	//   }
	//};
	//ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &staConfig) );

    ESP_ERROR_CHECK( esp_wifi_start() );
	// ---------------------------------------------------------------------

	xEventGroupWaitBits(wifi_event_group, APSTARTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
		// start the HTTP Server task
	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);

	vTaskDelay(1000 / portTICK_RATE_MS);
	#endif

	if (ppposInit() == 0) {
		ESP_LOGE("PPPoS EXAMPLE", "ERROR: GSM not initialized, HALTED");
		while (1) {
			vTaskDelay(1000 / portTICK_RATE_MS);
		}
	}

	// ==== Get time from NTP server =====
	time_t now = 0;
	struct tm timeinfo = { 0 };
	int retry = 0;
	const int retry_count = 10;

	time(&now);
	localtime_r(&now, &timeinfo);

	while (1) {
		printf("\r\n");
		ESP_LOGI(TIME_TAG,"OBTAINING TIME");
	    ESP_LOGI(TIME_TAG, "Initializing SNTP");
	    sntp_setoperatingmode(SNTP_OPMODE_POLL);
	    sntp_setservername(0, "pool.ntp.org");
	    sntp_init();
		ESP_LOGI(TIME_TAG,"SNTP INITIALIZED");

		// wait for time to be set
		now = 0;
		while ((timeinfo.tm_year < (2016 - 1900)) && (++retry < retry_count)) {
			ESP_LOGI(TIME_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			time(&now);
			localtime_r(&now, &timeinfo);
			if (ppposStatus() != GSM_STATE_CONNECTED) break;
		}
		if (ppposStatus() != GSM_STATE_CONNECTED) {
			sntp_stop();
			ESP_LOGE(TIME_TAG, "Disconnected, waiting for reconnect");
			retry = 0;
			while (ppposStatus() != GSM_STATE_CONNECTED) {
				vTaskDelay(100 / portTICK_RATE_MS);
			}
			continue;
		}

		if (retry < retry_count) {
			ESP_LOGI(TIME_TAG, "TIME SET TO %s", asctime(&timeinfo));
			break;
		}
		else {
			ESP_LOGI(TIME_TAG, "ERROR OBTAINING TIME\n");
		}
		sntp_stop();
		break;
	}

	// ==== Create PPPoS tasks ====
    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
    xTaskCreate(&https_get_task, "https_get_task", 16384, NULL, 4, NULL);
	#ifdef CONFIG_GSM_SEND_SMS
	// ==== Create SMS task ====
    xTaskCreate(&sms_task, "sms_task", 4096, NULL, 3, NULL);
    #endif
    
    while(1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}
