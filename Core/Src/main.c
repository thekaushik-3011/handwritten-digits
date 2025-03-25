/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <ft5336.h>
#include <ft5336.h>
#include "stm32f4xx_hal.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include "ai_platform.h"
#include "network.h"
#include "network_data.h"
#include "stm32f429i_discovery_ts.h"
#include "stm32f429i_discovery_lcd.h"

/* Touchscreen definitions */
#define TS_I2C_ADDRESS                   ((uint16_t)0x70)
#define CANVAS_MARGIN                    20
#define CANVAS_SIZE                      200
#define DIGIT_RECOGNITION_SIZE           28  /* MNIST standard 28x28 */
#define TOUCH_THRESHOLD                  2   /* Adjacent points threshold */
#define LINE_THICKNESS                   15

/* AI model variables */
AI_ALIGNED(4) ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];
AI_ALIGNED(4) ai_u8 in_data[AI_NETWORK_IN_1_SIZE_BYTES];
AI_ALIGNED(4) ai_u8 out_data[AI_NETWORK_OUT_1_SIZE_BYTES];

ai_handle network = AI_HANDLE_NULL;
ai_network_report network_info;

/* Drawing variables */
uint8_t canvas[CANVAS_SIZE][CANVAS_SIZE];
uint8_t resized_digit[DIGIT_RECOGNITION_SIZE][DIGIT_RECOGNITION_SIZE];
uint8_t is_drawing = 0;
uint16_t last_x = 0;
uint16_t last_y = 0;

/* UI elements positions */
#define CLEAR_BUTTON_X                   40
#define CLEAR_BUTTON_Y                   240
#define CLEAR_BUTTON_WIDTH               60
#define CLEAR_BUTTON_HEIGHT              40

#define RECOGNIZE_BUTTON_X               160
#define RECOGNIZE_BUTTON_Y               240
#define RECOGNIZE_BUTTON_WIDTH           60
#define RECOGNIZE_BUTTON_HEIGHT          40

#define RESULT_TEXT_X                    120
#define RESULT_TEXT_Y                    270

/* External LCD/Touch Driver */
extern FT5336_Object_t ft5336_obj;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

SPI_HandleTypeDef hspi5;

UART_HandleTypeDef huart1;

SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C3_Init(void);
static void MX_LTDC_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI5_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Initialize AI Model */
static void AI_Init(void)
{
  ai_error err;

  /* Create network instance */
  err = ai_network_create(&network, AI_NETWORK_DATA_CONFIG);
  if (err.type != AI_ERROR_NONE) {
    printf("Error: could not create NN instance\r\n");
    while(1);
  }

  /* Initialize network */
  const ai_network_params params = {
    AI_NETWORK_DATA_WEIGHTS(ai_network_data_weights_get()),
    AI_NETWORK_DATA_ACTIVATIONS(activations)
  };

  if (!ai_network_init(network, &params)) {
    printf("Error: could not initialize NN\r\n");
    while(1);
  }

  /* Get network info */
  if (!ai_network_get_report(network, &network_info)) {
    printf("Error: could not get NN report\r\n");
    while(1);
  }
}

/* Initialize Touchscreen */
static void Touchscreen_Init(void)
{
  uint32_t ts_status = BSP_TS_Init(240, 320);
  if (ts_status != TS_OK) {
    printf("Error: Touchscreen initialization failed\r\n");
    Error_Handler();
  }
}

/* Initialize LCD Display */
static void LCD_Init(void)
{
  /* Initialize LCD */
  BSP_LCD_Init();
  BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
  BSP_LCD_SelectLayer(0);
  BSP_LCD_DisplayOn();

  /* Clear entire screen to white */
  BSP_LCD_Clear(LCD_COLOR_WHITE);

  /* Set default drawing colors */
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);

  /* Draw Canvas border (200x200 at 20,20) */
  BSP_LCD_DrawRect(CANVAS_MARGIN - 2, CANVAS_MARGIN - 2,
                   CANVAS_SIZE + 4, CANVAS_SIZE + 4);

  /* Draw Buttons */
  BSP_LCD_SetTextColor(LCD_COLOR_RED);
  BSP_LCD_FillRect(CLEAR_BUTTON_X, CLEAR_BUTTON_Y,
                   CLEAR_BUTTON_WIDTH, CLEAR_BUTTON_HEIGHT);

  BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
  BSP_LCD_FillRect(RECOGNIZE_BUTTON_X, RECOGNIZE_BUTTON_Y,
                   RECOGNIZE_BUTTON_WIDTH, RECOGNIZE_BUTTON_HEIGHT);

  /* Button text */
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_SetBackColor(LCD_COLOR_RED);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAt(CLEAR_BUTTON_X + 10, CLEAR_BUTTON_Y + 12,
                          (uint8_t *)"CLEAR", LEFT_MODE);

  BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
  BSP_LCD_DisplayStringAt(RECOGNIZE_BUTTON_X + 10, RECOGNIZE_BUTTON_Y + 12,
                          (uint8_t *)"RECOG", LEFT_MODE);

  /* Title and instructions */
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetFont(&Font20);
  BSP_LCD_DisplayStringAt(0, 5, (uint8_t *)"Digit Recognition", CENTER_MODE);
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_DisplayStringAt(0, 30, (uint8_t *)"Draw a digit (0-9)", CENTER_MODE);
}

/* Clear drawing canvas */
static void Clear_Canvas(void)
{
  /* Clear canvas array */
  memset(canvas, 0, sizeof(canvas));

  /* Clear LCD canvas area */
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillRect(CANVAS_MARGIN, CANVAS_MARGIN, CANVAS_SIZE, CANVAS_SIZE);

  /* Redraw canvas border */
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_DrawRect(CANVAS_MARGIN - 2, CANVAS_MARGIN - 2,
                   CANVAS_SIZE + 4, CANVAS_SIZE + 4);

  /* Clear previous result */
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillRect(RESULT_TEXT_X - 60, RESULT_TEXT_Y, 135, 48);
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_DisplayStringAt(0, 30, (uint8_t *)"Draw a digit (0-9)", CENTER_MODE);
}

/* Draw line between two points */
static void Draw_Line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);

  if (steep) {
    SWAP(x0, y0);
    SWAP(x1, y1);
  }

  if (x0 > x1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }

  int16_t dx = x1 - x0;
  int16_t dy = abs(y1 - y0);
  int16_t err = dx / 2;
  int16_t ystep = (y0 < y1) ? 1 : -1;

  BSP_LCD_SetTextColor(LCD_COLOR_BLACK); /* Ensure color is set */
  for (; x0 <= x1; x0++) {
    int16_t base_x = steep ? y0 : x0;
    int16_t base_y = steep ? x0 : y0;

    for (int i = -LINE_THICKNESS/2; i <= LINE_THICKNESS/2; i++) {
      for (int j = -LINE_THICKNESS/2; j <= LINE_THICKNESS/2; j++) {
        int16_t px = base_x + i;
        int16_t py = base_y + j;
        if (px >= CANVAS_MARGIN && px < CANVAS_MARGIN + CANVAS_SIZE &&
            py >= CANVAS_MARGIN && py < CANVAS_MARGIN + CANVAS_SIZE) {
          canvas[py - CANVAS_MARGIN][px - CANVAS_MARGIN] = 255;
          BSP_LCD_DrawPixel(px, py, LCD_COLOR_BLACK);
        }
      }
    }

    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

/* Process touch events */
static void Process_Touch(void)
{
  TS_StateTypeDef ts_state;
  BSP_TS_GetState(&ts_state);

  if (ts_state.TouchDetected) {
    /* Raw touch coordinates */
    uint16_t x = ts_state.X;
    uint16_t touch_y = ts_state.Y;

//    /* Transform coordinates: completely opposite */
//    uint16_t x = 240 - touch_x;  /* Invert X */
    uint16_t y = 320 - touch_y;  /* Invert Y */

//    /* Debug output to UART */
//    char msg[50];
//    sprintf(msg, "Touch: Raw(%d,%d) -> LCD(%d,%d)\r\n", touch_x, touch_y, x, y);
//    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);

    /* Clear button (y=240 now) */
    if (x >= CLEAR_BUTTON_X && x <= CLEAR_BUTTON_X + CLEAR_BUTTON_WIDTH &&
        y >= CLEAR_BUTTON_Y && y <= CLEAR_BUTTON_Y + CLEAR_BUTTON_HEIGHT) {
      Clear_Canvas();
      HAL_Delay(200);
      return;
    }

    /* Recognize button */
    if (x >= RECOGNIZE_BUTTON_X && x <= RECOGNIZE_BUTTON_X + RECOGNIZE_BUTTON_WIDTH &&
        y >= RECOGNIZE_BUTTON_Y && y <= RECOGNIZE_BUTTON_Y + RECOGNIZE_BUTTON_HEIGHT) {
      Recognize_Digit();
      HAL_Delay(200);
      return;
    }

    /* Drawing area */
    if (x >= CANVAS_MARGIN && x < CANVAS_MARGIN + CANVAS_SIZE &&
        y >= CANVAS_MARGIN && y < CANVAS_MARGIN + CANVAS_SIZE) {
      if (is_drawing) {
        if (abs((int16_t)x - (int16_t)last_x) > TOUCH_THRESHOLD ||
            abs((int16_t)y - (int16_t)last_y) > TOUCH_THRESHOLD) {
          Draw_Line(last_x, last_y, x, y);
        }
      } else {
        is_drawing = 1;
        BSP_LCD_DrawPixel(x, y, LCD_COLOR_BLACK);
        canvas[y - CANVAS_MARGIN][x - CANVAS_MARGIN] = 255;
      }
      last_x = x;
      last_y = y;
    } else {
      is_drawing = 0;
    }
  } else {
    is_drawing = 0;
  }
}

/* Resize canvas to 28x28 for model input */
static void Resize_Canvas(void)
{
  float scale_factor = (float)CANVAS_SIZE / DIGIT_RECOGNITION_SIZE;

  for (int i = 0; i < DIGIT_RECOGNITION_SIZE; i++) {
    for (int j = 0; j < DIGIT_RECOGNITION_SIZE; j++) {
      /* Average pooling for downsampling */
      int sum = 0;
      int count = 0;

      int start_y = (int)(i * scale_factor);
      int end_y = (int)((i + 1) * scale_factor);
      int start_x = (int)(j * scale_factor);
      int end_x = (int)((j + 1) * scale_factor);

      /* Boundary check */
      end_y = (end_y >= CANVAS_SIZE) ? CANVAS_SIZE - 1 : end_y;
      end_x = (end_x >= CANVAS_SIZE) ? CANVAS_SIZE - 1 : end_x;

      for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
          sum += canvas[y][x];
          count++;
        }
      }

      resized_digit[i][j] = (count > 0) ? (sum / count) : 0;
    }
  }

  //Display_Resized_Canvas();
}

//void UART_Transmit(char *data, uint16_t size)
//{
//    HAL_UART_Transmit(&huart1, (uint8_t *)data, size, HAL_MAX_DELAY);
//}
//
//void Display_Resized_Canvas(void) {
//    char buffer[100]; // Buffer for formatting strings
//
//    // Print a header
//    sprintf(buffer, "\r\n=== Resized Canvas (DIGIT_RECOGNITION_SIZE: %d x %d) ===\r\n",
//            DIGIT_RECOGNITION_SIZE, DIGIT_RECOGNITION_SIZE);
//    UART_Transmit(buffer, strlen(buffer));
//
//    // Iterate through the resized_digit array
//    for (int i = 0; i < DIGIT_RECOGNITION_SIZE; i++) {
//        for (int j = 0; j < DIGIT_RECOGNITION_SIZE; j++) {
//            // Print each element with formatting for readability
//            sprintf(buffer, "%3d ", resized_digit[i][j]); // %3d ensures 3-character width for alignment
//            UART_Transmit(buffer, strlen(buffer));
//        }
//        // New line after each row
//        sprintf(buffer, "\r\n");
//        UART_Transmit(buffer, strlen(buffer));
//    }
//
//    // Print a footer
//    sprintf(buffer, "=== End of Resized Canvas ===\r\n\r\n");
//    UART_Transmit(buffer, strlen(buffer));
//}

/* Prepare input for AI model */
static void Prepare_Model_Input(void)
{
  float *input_data = (float *)in_data;

  /* Normalize and flatten the input */
  for (int i = 0; i < DIGIT_RECOGNITION_SIZE; i++) {
    for (int j = 0; j < DIGIT_RECOGNITION_SIZE; j++) {
      /* Convert to float and normalize to [0,1] range */
      input_data[i * DIGIT_RECOGNITION_SIZE + j] = resized_digit[i][j] / 255.0f;
    }
  }
}

/* Run inference and display result */
void Recognize_Digit(void)
{
  ai_i32 batch;

  Resize_Canvas();
  Prepare_Model_Input();

  // Define input buffer with shape and strides
	ai_buffer ai_input[AI_NETWORK_IN_NUM] = {
		{
			.data = AI_HANDLE_PTR(in_data),
			.size = AI_NETWORK_IN_1_SIZE_BYTES, // 3136
			.format = AI_BUFFER_FORMAT_FLOAT,
			.shape = AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 1, 28, 28), // Matches model
		}
	};

	// Define output buffer with shape and strides
	ai_buffer ai_output[AI_NETWORK_OUT_NUM] = {
		{
			.data = AI_HANDLE_PTR(out_data),
			.size = AI_NETWORK_OUT_1_SIZE_BYTES, // 40
			.format = AI_BUFFER_FORMAT_FLOAT,
			.shape = AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 10, 1, 1), // Matches model
		}
	};

  batch = ai_network_run(network, ai_input, ai_output);
  if (batch != 1) {
    printf("Error: Inference failed\r\n");
    return;
  }

  float *output = (float *)out_data;
  int max_idx = 0;
  float max_val = output[0];
  for (int i = 1; i < 10; i++) {
    if (output[i] > max_val) {
      max_val = output[i];
      max_idx = i;
    }
  }

  /* Adjust result display position */
  BSP_LCD_SetFont(&Font24);
  char result_str[20];
  sprintf(result_str, "Digit: %d", max_idx);
  BSP_LCD_DisplayStringAt(RESULT_TEXT_X - 60, RESULT_TEXT_Y, (uint8_t *)result_str, LEFT_MODE);

  BSP_LCD_SetFont(&Font16);
  char conf_str[30];
  sprintf(conf_str, "Conf: %.2f%%", max_val * 100);
  BSP_LCD_DisplayStringAt(RESULT_TEXT_X - 60, RESULT_TEXT_Y + 30, (uint8_t *)conf_str, LEFT_MODE);
}

//void Recognize_Digit(void)
//{
//    ai_i32 batch;
//    char uart_str[100];
//
//    Resize_Canvas();
//    Prepare_Model_Input();
//
//    // Define input buffer with shape and strides
//    ai_buffer ai_input[AI_NETWORK_IN_NUM] = {
//        {
//            .data = AI_HANDLE_PTR(in_data),
//            .size = AI_NETWORK_IN_1_SIZE_BYTES, // 3136
//            .format = AI_BUFFER_FORMAT_FLOAT,
//            .shape = AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 1, 28, 28), // Matches model
//        }
//    };
//
//    // Define output buffer with shape and strides
//    ai_buffer ai_output[AI_NETWORK_OUT_NUM] = {
//        {
//            .data = AI_HANDLE_PTR(out_data),
//            .size = AI_NETWORK_OUT_1_SIZE_BYTES, // 40
//            .format = AI_BUFFER_FORMAT_FLOAT,
//            .shape = AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 10, 1, 1), // Matches model
//        }
//    };
//
//    sprintf(uart_str, "Before ai_network_run: Input size=%d bytes, Output size=%d bytes\r\n",
//            ai_input[0].size, ai_output[0].size);
//    HAL_UART_Transmit(&huart1, (uint8_t*)uart_str, strlen(uart_str), HAL_MAX_DELAY);
//    sprintf(uart_str, "Input ptr=%p, Output ptr=%p\r\n",
//            (void*)ai_input[0].data, (void*)ai_output[0].data);
//    HAL_UART_Transmit(&huart1, (uint8_t*)uart_str, strlen(uart_str), HAL_MAX_DELAY);
//
//    batch = ai_network_run(network, ai_input, ai_output);
//    if (batch != 1) {
//        sprintf(uart_str, "Error: ai_network_run failed (batch=%d)\r\n", batch);
//        HAL_UART_Transmit(&huart1, (uint8_t*)uart_str, strlen(uart_str), HAL_MAX_DELAY);
//        ai_error err = ai_network_get_error(network);
//        sprintf(uart_str, "Last error: type=%d, code=%d\r\n", err.type, err.code);
//        HAL_UART_Transmit(&huart1, (uint8_t*)uart_str, strlen(uart_str), HAL_MAX_DELAY);
//        return;
//    }
//
//    float *output = (float *)out_data;
//    sprintf(uart_str, "Output: [%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f]\r\n",
//            output[0], output[1], output[2], output[3], output[4],
//            output[5], output[6], output[7], output[8], output[9]);
//    HAL_UART_Transmit(&huart1, (uint8_t*)uart_str, strlen(uart_str), HAL_MAX_DELAY);
//
//    int max_idx = 0;
//    float max_val = output[0];
//    for (int i = 1; i < 10; i++) {
//        if (output[i] > max_val) {
//            max_val = output[i];
//            max_idx = i;
//        }
//    }
//    sprintf(uart_str, "Recognized Digit: %d (Confidence: %.2f%%)\r\n", max_idx, max_val * 100);
//    HAL_UART_Transmit(&huart1, (uint8_t*)uart_str, strlen(uart_str), HAL_MAX_DELAY);
//}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

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
  MX_CRC_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_USART1_UART_Init();
  MX_SPI5_Init();
  /* USER CODE BEGIN 2 */

  /* Initialize LCD */
  LCD_Init();

  /* Initialize Touchscreen */
  Touchscreen_Init();

  /* Initialize AI Model */
  AI_Init();

  /* Clear Canvas to start */
  Clear_Canvas();

  BSP_LCD_SetFont(&Font20);
  BSP_LCD_DisplayStringAt(0, 5, (uint8_t *)"Digit Recognition", CENTER_MODE);
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_DisplayStringAt(0, 30, (uint8_t *)"Draw a digit (0-9)", CENTER_MODE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Process_Touch();
    HAL_Delay(10); /* Poll touchscreen at 100Hz */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};
  LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 7;
  hltdc.Init.VerticalSync = 3;
  hltdc.Init.AccumulatedHBP = 14;
  hltdc.Init.AccumulatedVBP = 5;
  hltdc.Init.AccumulatedActiveW = 654;
  hltdc.Init.AccumulatedActiveH = 485;
  hltdc.Init.TotalWidth = 660;
  hltdc.Init.TotalHeight = 487;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 0;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 0;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg.Alpha = 0;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0;
  pLayerCfg.ImageWidth = 0;
  pLayerCfg.ImageHeight = 0;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg1.WindowX0 = 0;
  pLayerCfg1.WindowX1 = 0;
  pLayerCfg1.WindowY0 = 0;
  pLayerCfg1.WindowY1 = 0;
  pLayerCfg1.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg1.Alpha = 0;
  pLayerCfg1.Alpha0 = 0;
  pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg1.FBStartAdress = 0;
  pLayerCfg1.ImageWidth = 0;
  pLayerCfg1.ImageHeight = 0;
  pLayerCfg1.Backcolor.Blue = 0;
  pLayerCfg1.Backcolor.Green = 0;
  pLayerCfg1.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

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
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_1;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_DISABLE;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 16;
  SdramTiming.ExitSelfRefreshDelay = 16;
  SdramTiming.SelfRefreshTime = 16;
  SdramTiming.RowCycleDelay = 16;
  SdramTiming.WriteRecoveryTime = 16;
  SdramTiming.RPDelay = 16;
  SdramTiming.RCDDelay = 16;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
