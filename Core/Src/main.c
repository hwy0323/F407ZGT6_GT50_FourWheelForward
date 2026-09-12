#include "main.h"

/* Four TB6612 motor channels. All STBY pins share PC0. */
#define MOTOR_PWM_PERIOD 8399U
#define MOTOR_TEST_SPEED 180U
#define MOTOR_RUN_TIME_MS 2000U
#define MOTOR_STOP_TIME_MS 500U
#define KEY_ACTIVE_LOW 1U

static TIM_HandleTypeDef htim1;

static void Error_Stop(void)
{
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
  while (1) { }
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState = RCC_HSE_ON;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLM = 8;
  osc.PLL.PLLN = 336;
  osc.PLL.PLLP = RCC_PLLP_DIV2;
  osc.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Stop();
  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV4;
  clk.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) Error_Stop();
}

static void GPIO_Init_All(void)
{
  GPIO_InitTypeDef gpio = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
  gpio.Pin = LED_Pin;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &gpio);

  /* PC0 is connected to the STBY pin of every TB6612 module. */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
  gpio.Pin = GPIO_PIN_0;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &gpio);

  /* M1: PD0/PD1, M2: PD2/PD3, M3: PD4/PD5, M4: PD6/PD7. */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                              GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
                    GPIO_PIN_RESET);
  gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
             GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &gpio);

  /* M1: PE9, M2: PE11, M3: PE13, M4: PE14. */
  gpio.Pin = GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &gpio);

  /* Board KEY is PA15. It is normally high and becomes low when pressed. */
  gpio.Pin = GPIO_PIN_15;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &gpio);
}

static void PWM_Init_All(void)
{
  TIM_OC_InitTypeDef pwm = {0};
  __HAL_RCC_TIM1_CLK_ENABLE();
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = MOTOR_PWM_PERIOD;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) Error_Stop();

  pwm.OCMode = TIM_OCMODE_PWM1;
  pwm.Pulse = 0;
  pwm.OCPolarity = TIM_OCPOLARITY_HIGH;
  pwm.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &pwm, TIM_CHANNEL_1) != HAL_OK) Error_Stop();
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &pwm, TIM_CHANNEL_2) != HAL_OK) Error_Stop();
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &pwm, TIM_CHANNEL_3) != HAL_OK) Error_Stop();
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &pwm, TIM_CHANNEL_4) != HAL_OK) Error_Stop();

  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) Error_Stop();
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) Error_Stop();
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) != HAL_OK) Error_Stop();
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK) Error_Stop();
}

static void Motor_Stop_All(void)
{
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                              GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
                    GPIO_PIN_RESET);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
}

static void Motor_Forward_One(uint8_t motor, uint16_t speed)
{
  if (speed > 1000U) speed = 1000U;

  /* Previous test proved that this car moves forward with IN1 = low, IN2 = high. */
  Motor_Stop_All();
  if (motor == 1U) {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (speed * MOTOR_PWM_PERIOD) / 1000U);
  } else if (motor == 2U) {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (speed * MOTOR_PWM_PERIOD) / 1000U);
  } else if (motor == 3U) {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (speed * MOTOR_PWM_PERIOD) / 1000U);
  } else {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (speed * MOTOR_PWM_PERIOD) / 1000U);
  }
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
}

static void Run_One_Motor(uint8_t motor)
{
  Motor_Forward_One(motor, MOTOR_TEST_SPEED);
  HAL_Delay(MOTOR_RUN_TIME_MS);
  Motor_Stop_All();
  HAL_Delay(MOTOR_STOP_TIME_MS);
}

static uint8_t Key_Clicked(void)
{
  GPIO_PinState pressed_level = KEY_ACTIVE_LOW ? GPIO_PIN_RESET : GPIO_PIN_SET;
  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == pressed_level) {
    HAL_Delay(20);
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == pressed_level) {
      while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == pressed_level) { }
      HAL_Delay(20);
      return 1U;
    }
  }
  return 0U;
}

int main(void)
{
  uint32_t led_tick = 0;
  HAL_Init();
  SystemClock_Config();
  GPIO_Init_All();
  PWM_Init_All();
  Motor_Stop_All();

  while (1) {
    if (Key_Clicked()) {
      Run_One_Motor(1U);  /* Left front: Motor A. */
      Run_One_Motor(2U);  /* Right front: Motor B. */
      Run_One_Motor(3U);  /* Left rear: Motor C. */
      Run_One_Motor(4U);  /* Right rear: Motor D. */
    }
    if (HAL_GetTick() - led_tick >= 500U) {
      led_tick = HAL_GetTick();
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
  }
}

void Error_Handler(void) { Error_Stop(); }
