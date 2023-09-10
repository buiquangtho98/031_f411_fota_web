/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "UartRingbuffer_multi.h"
#include "ESP8266_HAL.h"
#include "parse.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SSID "Tho"
#define PASSWORD "12345678"
#define FLASH_ADDRESS_BASE 0x40023C00
#define SECTOR_A 0x08008000 // Sector 2
#define SECTOR_B 0x0800C000 // Sector 3
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define pc_uart &huart1

char recv_buf[2048] = { 0 };
char lat_ver[128] = { 0 };
char fw_buf[1024] = { 0 };
char new = 0;
char current_sector = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void leds_init() {
	__HAL_RCC_GPIOD_CLK_ENABLE();
	uint32_t *GPIOD_MODER = (uint32_t*) (0x40020C00 + 0x00);
	uint32_t *GPIOD_OTYPER = (uint32_t*) (0x40020C00 + 0x04);
	/* set up PD12, 13, 14, 15 in OUTPUT */
	*GPIOD_MODER &= ~(0xff << 24);
	*GPIOD_MODER |= (0b01 << 24) | (0b01 << 26) | (0b01 << 28) | (0b01 << 30);
	/* set up PD12, 13, 14, 15 in push-pull */
	*GPIOD_OTYPER &= ~(0b1111 << 12);
}

void print_pc(char *str) {
	Uart_sendstring(str, pc_uart);
}

int Erase(int sector_num) {
	if ((sector_num < 0) | (sector_num > 7))
		return -1;
	//1. Check that no flash memory operation is ongoing by checking the BSY bit in the FLASH_SR register
	uint32_t *FLASH_SR = (uint32_t*) (FLASH_ADDRESS_BASE + 0x0c);
	// Wait while the BSY bit is still 1
	while (((*FLASH_SR >> 16) & 1) == 1)
		;
	//2. Set the SER bit and select the sector out of the 12 sectors in the main memory block you wish to erase (SNB) in the FLASH_CR register
	uint32_t *FLASH_CR = (uint32_t*) (FLASH_ADDRESS_BASE + 0x10);
	// After reset, FLASH_CR is locked, have to unlock to config it
	// Check if the CR is locked
	if (((*FLASH_CR >> 31) & 1) == 1) {
		// Make unlock sequence
		uint32_t *FLASH_KEYR = (uint32_t*) (FLASH_ADDRESS_BASE + 0x04);
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}
	*FLASH_CR |= 1 << 1; // Set SER bit
	*FLASH_CR &= ~(0b1111 << 3); // Reset SNB value
	*FLASH_CR |= sector_num << 3; // Write SNB value
	//3. Set the STRT bit in the FLASH_CR register
	*FLASH_CR |= 1 << 16;
	//4. Wait for the BSY bit to be cleared
	while (((*FLASH_SR >> 16) & 1) == 1)
		;
	// Careful coder
	*FLASH_CR &= ~(1 << 1); // Reset SER bit
	return 0;
}

void Program(char *address, char *data, int data_size) {
	//1. Check that no main Flash memory operation is ongoing by checking the BSY bit in the FLASH_SR register
	uint32_t *FLASH_SR = (uint32_t*) (FLASH_ADDRESS_BASE + 0x0c);
	// Wait while the BSY bit is still 1
	while (((*FLASH_SR >> 16) & 1) == 1)
		;
	//2. Set the PG bit in the FLASH_CR register
	uint32_t *FLASH_CR = (uint32_t*) (FLASH_ADDRESS_BASE + 0x10);
	// After reset, FLASH_CR is locked, have to unlock to config it
	// Check if the CR is locked
	if (((*FLASH_CR >> 31) & 1) == 1) {
		// Make unlock sequence
		uint32_t *FLASH_KEYR = (uint32_t*) (FLASH_ADDRESS_BASE + 0x04);
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}
	*FLASH_CR |= 1 << 0; // Set PG bit
	//todo	3. Perform the data write operation(s) to the desired memory address (inside main memory block or OTP area):
	for (int i = 0; i < data_size; i++) {
		address[i] = data[i];
	}
	//4. Wait for the BSY bit to be cleared
	while (((*FLASH_SR >> 16) & 1) == 1)
		;
	// Careful coder
	*FLASH_CR &= ~(1 << 0); // Reset PG bit
}

void system_reset() {
	uint32_t *AIRCR = (uint32_t*) 0xE000ED0C;
	*AIRCR = (0x5fa << 16) | (1 << 2);
}

void firmware_update() {
	print_pc("Starting update\r\n");
	if (current_sector == 2) {
		Erase(3);
		print_pc("Erased sector 3 (B)\r\n");
		Program((char*) SECTOR_B, (char*) fw_buf, sizeof(fw_buf));
		print_pc("Programmed into sector 3 (B)\r\n");
	} else if ((current_sector == 3) | (current_sector == 0)) {
		Erase(2);
		print_pc("Erased sector 2 (A)\r\n");
		Program((char*) SECTOR_A, (char*) fw_buf, sizeof(fw_buf));
		print_pc("Programmed into sector 2 (A)\r\n");
	}
	print_pc("Resetting system\r\n");
	HAL_Delay(100);
	system_reset();
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Some tests
//char src_buf[]="Hel\r\n\r\n+IPD,30:lo\0 world\r\n\r\n+IPD,22:,\r\n this is \r\n\r\n+IPD,12:my string\r\nCLOSED\r\n";
//char dst_buf[100]={0};
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */
	leds_init();
	Ringbuf_init();
	print_pc("Bootloader initiated\r\n");

	ESP_Init(SSID, PASSWORD);
	print_pc("Connected to wifi\r\n");

	// Get latest firmware's name from server
	memset(recv_buf, 0, sizeof(recv_buf));
	ESP_Get_Latest_Version(recv_buf);
	memset(lat_ver, 0, sizeof(lat_ver));
	version_parse(lat_ver, recv_buf);
	print_pc("Got latest file name\r\n");

	// Check the 2 current version
	uint32_t *reset_handler_address;
	void (*reset_handler_function)();
	uint32_t *vers_secta = (uint32_t*) (SECTOR_A + 0x198);
	uint32_t *vers_sectb = (uint32_t*) (SECTOR_B + 0x198);
	if (((*vers_secta > *vers_sectb) & (*vers_secta != 0xffffffff))
			| ((*vers_secta != 0xffffffff) & (*vers_sectb == 0xffffffff))) {
		reset_handler_address = (uint32_t*) (SECTOR_A + 0x4);
		current_sector = 2;
	} else if (((*vers_secta < *vers_sectb) & (*vers_sectb != 0xffffffff))
			| ((*vers_sectb != 0xffffffff) & (*vers_secta == 0xffffffff))) {
		reset_handler_address = (uint32_t*) (SECTOR_B + 0x4);
		current_sector = 3;
	} else if (current_sector == 0) {
		print_pc("There is no app\r\n");
	} else {
		reset_handler_address = (uint32_t*) (SECTOR_A + 0x4);
		current_sector = 2;
	}
	print_pc("Got current version\r\n");

	// Get latest firmware's content from server
	memset(recv_buf, 0, sizeof(recv_buf));
	ESP_Get_Firmware(recv_buf, lat_ver);
	memset(fw_buf, 0, sizeof(fw_buf));
	firmware_parse(fw_buf, recv_buf, sizeof(recv_buf));
	print_pc("Downloaded firmware file\r\n");

	// Check if the firmware on the web is newer than the current one or not
	new=0;
	uint32_t *vers_web = 0;
	vers_web = &fw_buf[0x198];
	if ((current_sector == 2)
			& ((*vers_web > *vers_secta) | (*vers_secta == 0xffffffff))) {
		new = 1;
	} else if ((current_sector == 3)
			& ((*vers_web > *vers_sectb) | (*vers_sectb == 0xffffffff))) {
		new = 1;
	} else if ((current_sector == 0) & (*vers_web != 0))
		new = 1;

	// If there is newer firmware, download and update
	if (new) {
		print_pc("The web firmware is newer, going to update\r\n");
		// Reset the new firmware flag
		new = 0;
		// Call update firmware function
		firmware_update();
	}
	reset_handler_function = (void*) *reset_handler_address;
	if (current_sector != 0) {
		reset_handler_function();
	}
	// Some tests
//	memset(dst_buf,0,sizeof(dst_buf));
//	firmware_parse(dst_buf, src_buf, sizeof(rcv_buf));
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
		HAL_Delay(500);
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 60;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

	/*Configure GPIO pin : PD12 */
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
