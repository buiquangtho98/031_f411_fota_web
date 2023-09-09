/*
 * ESP8266_HAL.c
 *
 *  Created on: Apr 14, 2020
 *      Author: Controllerstech
 */

#include "UartRingbuffer_multi.h"
#include "ESP8266_HAL.h"
#include "stdio.h"
#include "string.h"

extern UART_HandleTypeDef huart2;

#define wifi_uart &huart2

char buffer[20];

/*****************************************************************************************************************************************/

void ESP_Init(char *SSID, char *PASSWD) {
	char data[80];

//	Ringbuf_init();

	Uart_sendstring("AT+RST\r\n", wifi_uart);
//	Uart_sendstring("RESETTING.", pc_uart);
	for (int i = 0; i < 5; i++) {
//		Uart_sendstring(".", pc_uart);
		for (int j = 0; j < 1160 * 4 * 1000; j++)
			;
	}

	/********* AT **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT\r\n", wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
//	Uart_sendstring("AT---->OK\n\n", pc_uart);

	/********* AT+CWMODE=1 **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT+CWMODE=1\r\n", wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
//	Uart_sendstring("CW MODE---->1\n\n", pc_uart);

	/********* AT+CWJAP="SSID","PASSWD" **********/
	Uart_flush(wifi_uart);
//	Uart_sendstring("connecting... to the provided AP\n", pc_uart);
	sprintf(data, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, PASSWD);
	Uart_sendstring(data, wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
//	sprintf (data, "Connected to,\"%s\"\n\n", SSID);
//	Uart_sendstring(data,pc_uart);

	/********* AT+CIFSR **********/
//	Uart_flush(wifi_uart);
//	Uart_sendstring("AT+CIFSR\r\n", wifi_uart);
//	while (!(Wait_for("CIFSR:STAIP,\"", wifi_uart)));
//	while (!(Copy_upto("\"",buffer, wifi_uart)));
//	while (!(Wait_for("OK\r\n", wifi_uart)));
//	int len = strlen (buffer);
//	buffer[len-1] = '\0';
//	sprintf (data, "IP ADDR: %s\n\n", buffer);
//	Uart_sendstring(data, pc_uart);
	/********* AT+CIPMUX **********/
//	Uart_flush(wifi_uart);
//	Uart_sendstring("AT+CIPMUX=1\r\n", wifi_uart);
//	while (!(Wait_for("OK\r\n", wifi_uart)));
//	Uart_sendstring("CIPMUX---->OK\n\n", pc_uart);
	/********* AT+CIPSERVER **********/
//	Uart_flush(wifi_uart);
//	Uart_sendstring("AT+CIPSERVER=1,80\r\n", wifi_uart);
//	while (!(Wait_for("OK\r\n", wifi_uart)));
//	Uart_sendstring("CIPSERVER---->OK\n\n", pc_uart);
//	Uart_sendstring("Now Connect to the IP ADRESS\n\n", pc_uart);
}

void bufclr(char *buf) {
	int len = strlen(buf);
	for (int i = 0; i < len; i++)
		buf[i] = '\0';
}

// Get the latest uploaded FW file name written on "latest_version.txt" file, on the web
void ESP_Get_Latest_Version(uint8_t* bufToPasteInto) {
	// Some temporary local buffer
	char local_buf[500] = { 0 };
	char local_buf2[30] = { 0 };
	char field_buf[200] = { 0 };
	// Create TCPIP connection to the web server
	Uart_flush(wifi_uart);
	Uart_sendstring(
			"AT+CIPSTART=\"TCP\",\"thobq-domain.000webhostapp.com\",80\r\n",
			wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
	// Send HTTP GET request to get the content of latest_version.txt
	// Prepair the HTTP GET request data
	bufclr(local_buf); // Make sure it cleared
	sprintf(local_buf, "GET /uploads/latest_version.txt HTTP/1.1\r\n"
			"Host: thobq-domain.000webhostapp.com\r\n"
			"Connection: close\r\n\r\n");
	int len = strlen(local_buf); // Get the data length
	// Prepair the CIPSEND command
	bufclr(local_buf2); // Make sure it cleared
	sprintf(local_buf2, "AT+CIPSEND=%d\r\n", len);
	// Send CIPSTART
	Uart_sendstring(local_buf2, wifi_uart);
	while (!(Wait_for(">", wifi_uart)))
		;
	// Send HTTP GET
	Uart_sendstring(local_buf, wifi_uart);
	while (!(Wait_for("SEND OK\r\n", wifi_uart)))
		;
	while (!(Wait_for("\r\n\r\n", wifi_uart)))
		;
	while (!(Copy_upto("\r\n", bufToPasteInto, wifi_uart)))
		;
}

// Send HTTP GET request to read the firmware file on web
void ESP_Get_Firmware(uint8_t* buff, uint8_t* vers){
	// Some temporary local buffer
	char local_buf[500] = { 0 };
	char local_buf2[30] = { 0 };
//	char field_buf[200] = { 0 };
	// Create TCPIP connection to the web server
	Uart_flush(wifi_uart);
	Uart_sendstring(
			"AT+CIPSTART=\"TCP\",\"thobq-domain.000webhostapp.com\",80\r\n",
			wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;
	// Send HTTP GET request to get the content of latest fw file
	// Prepair the HTTP GET request data
	bufclr(local_buf); // Make sure it cleared
	sprintf(local_buf, "GET /uploads/%s HTTP/1.1\r\n"
			"Host: thobq-domain.000webhostapp.com\r\n"
			"Connection: close\r\n\r\n", vers);
	int len = strlen(local_buf); // Get the data length
	// Prepair the CIPSEND command
	bufclr(local_buf2); // Make sure it cleared
	sprintf(local_buf2, "AT+CIPSEND=%d\r\n", len);
	// Send CIPSTART
	Uart_sendstring(local_buf2, wifi_uart);
	while (!(Wait_for(">", wifi_uart)))
		;
	// Send HTTP GET
	Uart_sendstring(local_buf, wifi_uart);
	while (!(Wait_for("SEND OK\r\n", wifi_uart)))
		;
	while (!(Wait_for("\r\n\r\n", wifi_uart)))
		;
	while (!(Copy_upto("CLOSED\r\n", buff, wifi_uart)))
		;
}

#if(0)
void ESP_Send_Multi(char *APIkey, int numberoffileds, float value[]) {
	char local_buf[500] = { 0 };
	char local_buf2[30] = { 0 };
	char field_buf[200] = { 0 };

	Uart_flush(wifi_uart);
	Uart_sendstring("AT+CIPSTART=\"TCP\",\"184.106.153.149\",80\r\n", wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)))
		;

	sprintf(local_buf, "GET /update?api_key=%s", APIkey);
	for (int i = 0; i < numberoffileds; i++) {
		sprintf(field_buf, "&field%d=%.6f", i + 1, value[i]);
		strcat(local_buf, field_buf);
	}
	strcat(local_buf, "\r\n");

	int len = strlen(local_buf);
	sprintf(local_buf2, "AT+CIPSEND=%d\r\n", len);
	Uart_sendstring(local_buf2,wifi_uart);
	while (!(Wait_for(">",wifi_uart)))
		;

	Uart_sendstring(local_buf,wifi_uart);
	while (!(Wait_for("SEND OK\r\n",wifi_uart)))
		;

	while (!(Wait_for("CLOSED",wifi_uart)))
		;

	bufclr(local_buf);
	bufclr(local_buf2);

//	Ringbuf_init();

}

int Server_Send (char *str, int Link_ID)
{
	int len = strlen (str);
	char data[80];
	sprintf (data, "AT+CIPSEND=%d,%d\r\n", Link_ID, len);
	Uart_sendstring(data, wifi_uart);
	while (!(Wait_for(">", wifi_uart)));
	Uart_sendstring (str, wifi_uart);
	while (!(Wait_for("SEND OK", wifi_uart)));
	sprintf (data, "AT+CIPCLOSE=5\r\n");
	Uart_sendstring(data, wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	return 1;
}

void Server_Handle (char *str, int Link_ID)
{
	char datatosend[1024] = {0};
	if (!(strcmp (str, "/ledon")))
	{
		sprintf (datatosend, Basic_inclusion);
		strcat(datatosend, LED_ON);
		strcat(datatosend, Terminate);
		Server_Send(datatosend, Link_ID);
	}

	else if (!(strcmp (str, "/ledoff")))
	{
		sprintf (datatosend, Basic_inclusion);
		strcat(datatosend, LED_OFF);
		strcat(datatosend, Terminate);
		Server_Send(datatosend, Link_ID);
	}

	else
	{
		sprintf (datatosend, Basic_inclusion);
		strcat(datatosend, LED_OFF);
		strcat(datatosend, Terminate);
		Server_Send(datatosend, Link_ID);
	}

}

void Server_Start (void)
{
	char buftocopyinto[64] = {0};
	char Link_ID;
	while (!(Get_after("+IPD,", 1, &Link_ID, wifi_uart)));
	Link_ID -= 48;
	while (!(Copy_upto(" HTTP/1.1", buftocopyinto, wifi_uart)));
	if (Look_for("/ledon", buftocopyinto) == 1)
	{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 1);
		Server_Handle("/ledon",Link_ID);
	}

	else if (Look_for("/ledoff", buftocopyinto) == 1)
	{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
		Server_Handle("/ledoff",Link_ID);
	}

	else if (Look_for("/favicon.ico", buftocopyinto) == 1);

	else
	{
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, 0);
		Server_Handle("/ ", Link_ID);
	}
}
#endif
