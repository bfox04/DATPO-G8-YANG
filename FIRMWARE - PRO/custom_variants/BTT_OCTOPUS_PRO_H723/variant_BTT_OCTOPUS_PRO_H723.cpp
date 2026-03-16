/*
 * Variant file for BIGTREETECH Octopus Pro V1.1
 * MCU: STM32H723ZET6 (Cortex-M7, 550MHz)
 *
 * Replaces the F446-based variant_BTT_OCTOPUS.cpp from the non-Pro board.
 *
 * KEY DIFFERENCES vs F446 variant:
 *   - SystemClock_Config uses H7-series HAL (RCC_OscInitTypeDef H7 fields,
 *     HAL_PWREx_ConfigSupply, PLL1/2/3 instead of PLLSAI)
 *   - HSE crystal = 25MHz (Pro V1.1 hardware) vs 12MHz on non-Pro F446
 *   - Target SYSCLK = 480MHz (H723 max) via PLL1
 *   - USB clock sourced from PLL3Q (48MHz) on H7 series
 *   - Flash latency = 4 wait states at 480MHz VOS0
 *   - Digital and analog pin arrays are identical to F446 variant
 *     (same GPIO routing on the PCB)
 *
 * Licensed under BSD 3-Clause (STMicroelectronics base, BTT modifications)
 */
#if defined(ARDUINO_BTT_OCTOPUS_PRO_H723)
#include "pins_arduino.h"

// Digital PinName array — identical board routing to non-Pro
const PinName digitalPin[] = {
  PA_0,   // D0/A0
  PA_1,   // D1/A1
  PA_2,   // D2/A2
  PA_3,   // D3/A3
  PA_4,   // D4/A4
  PA_5,   // D5/A5
  PA_6,   // D6/A6
  PA_7,   // D7/A7
  PA_8,   // D8
  PA_9,   // D9
  PA_10,  // D10
  PA_11,  // D11
  PA_12,  // D12
  PA_13,  // D13
  PA_14,  // D14
  PA_15,  // D15
  PB_0,   // D16/A8
  PB_1,   // D17/A9
  PB_2,   // D18
  PB_3,   // D19
  PB_4,   // D20
  PB_5,   // D21
  PB_6,   // D22
  PB_7,   // D23
  PB_8,   // D24
  PB_9,   // D25
  PB_10,  // D26
  PB_11,  // D27
  PB_12,  // D28
  PB_13,  // D29
  PB_14,  // D30
  PB_15,  // D31
  PC_0,   // D32/A10
  PC_1,   // D33/A11
  PC_2,   // D34/A12
  PC_3,   // D35/A13
  PC_4,   // D36/A14
  PC_5,   // D37/A15
  PC_6,   // D38
  PC_7,   // D39
  PC_8,   // D40
  PC_9,   // D41
  PC_10,  // D42
  PC_11,  // D43
  PC_12,  // D44
  PC_13,  // D45
  PC_14,  // D46
  PC_15,  // D47
  PD_0,   // D48
  PD_1,   // D49
  PD_2,   // D50
  PD_3,   // D51
  PD_4,   // D52
  PD_5,   // D53
  PD_6,   // D54
  PD_7,   // D55
  PD_8,   // D56
  PD_9,   // D57
  PD_10,  // D58
  PD_11,  // D59
  PD_12,  // D60
  PD_13,  // D61
  PD_14,  // D62
  PD_15,  // D63
  PE_0,   // D64
  PE_1,   // D65
  PE_2,   // D66
  PE_3,   // D67
  PE_4,   // D68
  PE_5,   // D69
  PE_6,   // D70
  PE_7,   // D71
  PE_8,   // D72
  PE_9,   // D73
  PE_10,  // D74
  PE_11,  // D75
  PE_12,  // D76
  PE_13,  // D77
  PE_14,  // D78
  PE_15,  // D79
  PF_0,   // D80
  PF_1,   // D81
  PF_2,   // D82
  PF_3,   // D83/A16
  PF_4,   // D84/A17
  PF_5,   // D85/A18
  PF_6,   // D86/A19
  PF_7,   // D87/A20
  PF_8,   // D88/A21
  PF_9,   // D89/A22
  PF_10,  // D90/A23
  PF_11,  // D91
  PF_12,  // D92
  PF_13,  // D93
  PF_14,  // D94
  PF_15,  // D95
  PG_0,   // D96
  PG_1,   // D97
  PG_2,   // D98
  PG_3,   // D99
  PG_4,   // D100
  PG_5,   // D101
  PG_6,   // D102
  PG_7,   // D103
  PG_8,   // D104
  PG_9,   // D105
  PG_10,  // D106
  PG_11,  // D107
  PG_12,  // D108
  PG_13,  // D109
  PG_14,  // D110
  PG_15,  // D111
  PH_0,   // D112
  PH_1    // D113
};

// Analog (Ax) pin number array
const uint32_t analogInputPin[] = {
  0,  // A0,  PA0
  1,  // A1,  PA1
  2,  // A2,  PA2
  3,  // A3,  PA3
  4,  // A4,  PA4
  5,  // A5,  PA5
  6,  // A6,  PA6
  7,  // A7,  PA7
  16, // A8,  PB0
  17, // A9,  PB1
  32, // A10, PC0
  33, // A11, PC1
  34, // A12, PC2
  35, // A13, PC3
  36, // A14, PC4
  37, // A15, PC5
  83, // A16, PF3
  84, // A17, PF4
  85, // A18, PF5
  86, // A19, PF6
  87, // A20, PF7
  88, // A21, PF8
  89, // A22, PF9
  90  // A23, PF10
};

// ----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  System Clock Configuration for STM32H723ZET6
  *         HSE = 25MHz (Octopus Pro V1.1 crystal)
  *         SYSCLK = 480MHz via PLL1 (H723 maximum)
  *         USB clock = 48MHz via PLL3Q
  *
  * This replaces the F446 SystemClock_Config which used PLLSAI and
  * HAL_PWREx_EnableOverDrive — those APIs do not exist on H7 series.
  */
WEAK void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    // Supply configuration: LDO supply, VOS0 for 480MHz operation
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    // Enable HSE and configure PLL1 for 480MHz SYSCLK
    // PLL1: HSE(25MHz) / PLLM(5) = 5MHz VCO input
    //       x PLLN(192) = 960MHz VCO output
    //       / PLLP(2)   = 480MHz SYSCLK
    //       / PLLQ(4)   = 240MHz (available for peripherals)
    //       / PLLR(2)   = 480MHz
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM            = 5;
    RCC_OscInitStruct.PLL.PLLN            = 192;
    RCC_OscInitStruct.PLL.PLLP            = 2;
    RCC_OscInitStruct.PLL.PLLQ            = 4;
    RCC_OscInitStruct.PLL.PLLR            = 2;
    RCC_OscInitStruct.PLL.PLLRGE          = RCC_PLL1VCIRANGE_2;  // 4–8MHz VCO input range
    RCC_OscInitStruct.PLL.PLLVCOSEL       = RCC_PLL1VCOWIDE;     // 192–836MHz VCO output
    RCC_OscInitStruct.PLL.PLLFRACN        = 0;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    // Configure PLL3 for USB 48MHz clock
    // PLL3: HSE(25MHz) / PLLM(5) = 5MHz
    //       x PLLN(48) = 240MHz VCO
    //       / PLLQ(5)  = 48MHz USB clock
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInitStruct.PLL3.PLL3M           = 5;
    PeriphClkInitStruct.PLL3.PLL3N           = 48;
    PeriphClkInitStruct.PLL3.PLL3P           = 2;
    PeriphClkInitStruct.PLL3.PLL3Q           = 5;
    PeriphClkInitStruct.PLL3.PLL3R           = 2;
    PeriphClkInitStruct.PLL3.PLL3RGE         = RCC_PLL3VCIRANGE_2;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL      = RCC_PLL3VCOWIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN       = 0;
    PeriphClkInitStruct.UsbClockSelection    = RCC_USBCLKSOURCE_PLL3;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    // Configure bus clocks
    // HCLK  = SYSCLK / 2 = 240MHz  (AHB)
    // PCLK1 = HCLK   / 2 = 120MHz  (APB1)
    // PCLK2 = HCLK   / 2 = 120MHz  (APB2)
    // PCLK3 = HCLK   / 2 = 120MHz  (APB3)
    // PCLK4 = SYSCLK / 4 = 120MHz  (APB4)
    RCC_ClkInitStruct.ClockType      = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK  |
                                        RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2 |
                                        RCC_CLOCKTYPE_D1PCLK1| RCC_CLOCKTYPE_D3PCLK1);
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV4;
    // Flash latency: 4 wait states required at 225MHz < HCLK ≤ 240MHz, VOS0
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

#ifdef __cplusplus
}
#endif

#endif /* ARDUINO_BTT_OCTOPUS_PRO_H723 */
