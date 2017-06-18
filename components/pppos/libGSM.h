/*
 *  Author: LoBo (loboris@gmail.com, loboris.github)
 *
*/


#ifndef _LIBGSM_H_
#define _LIBGSM_H_

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


int ppposInit();
void ppposDisconnect(uint8_t end_task, uint8_t rfoff);
void getRxTxCount(uint32_t *rx, uint32_t *tx, uint8_t rst);
void resetRxTxCount();
int is_pppConnected();
int gsmRFOff();

int smsSend(char *smsnum, char *msg);
void smsRead(SMS_Messages *SMSmesg, int sort);
int smsDelete(int idx);

#endif
