/*
 *  Author: LoBo (loboris@gmail.com, loboris.github)
 *
*/


#ifndef _LIBGSM_H_
#define _LIBGSM_H_

#define GSM_STATE_DISCONNECTED	0
#define GSM_STATE_CONNECTED		1
#define GSM_STATE_IDLE			89
#define GSM_STATE_FIRSTINIT		98


typedef struct
{
	int		idx;
	char	*msg;
	char	stat[32];
	char	from[32];
	char	time[32];
	time_t	time_value;
	int		tz;
}SMS_Msg;

typedef struct
{
	int		nmsg;
	SMS_Msg	*messages;
}SMS_Messages;

extern struct netif ppp_netif;

/*
 * Create GSM/PPPoS task if not already created
 * Initialize GSM and connect to Internet
 * Handle all PPPoS requests
 * Disconnect/Reconnect from/to Internet on user request
 */
//==============
int ppposInit();

/*
 * Disconnect from Internet
 * If 'end_task' = 1 also terminate GSM/PPPoS task
 * If 'rfoff' = 1, turns off GSM RF section to preserve power
 * If already disconnected, this function does nothing
 */
//====================================================
void ppposDisconnect(uint8_t end_task, uint8_t rfoff);

/*
 * Get transmitted and received bytes count
 * If 'rst' = 1, resets the counters
 */
//=========================================================
void getRxTxCount(uint32_t *rx, uint32_t *tx, uint8_t rst);

/*
 * Resets transmitted and received bytes counters
 */
//====================
void resetRxTxCount();

/*
 * Get GSM/Task status
 *
 * Result:
 * GSM_STATE_DISCONNECTED	(0)		Disconnected from Internet
 * GSM_STATE_CONNECTED		(1)		Connected to Internet
 * GSM_STATE_IDLE			(89)	Disconnected from Internet, Task idle, waiting for reconnect request
 * GSM_STATE_FIRSTINIT		(98)	Task started, initializing PPPoS
 */
//================
int ppposStatus();

/*
 * Turn GSM RF Off
 */
//==============
int gsm_RFOff();

/*
 * Turn GSM RF On
 */
//=============
int gsm_RFOn();

/*
 * Send SMS
 *
 * Params:
 *   smsnum:	Pointer to phone number in international format (+<counry_code><gsm number>)
 *      msg:	Pointer to message text
 */
//==================================
int smsSend(char *smsnum, char *msg);

/*
 * Read all SMS messages to 'SMS_Messages' structure
 */
//============================================
void smsRead(SMS_Messages *SMSmesg, int sort);

/*
 * Delete the message at GSM message index 'idx'
 */
int smsDelete(int idx);


#endif
