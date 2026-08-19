#ifndef __ADC_H__
    #define  __ADC_H__

#include "main.h"

#define ADC_AVG_SIZE 16 // Para filtros

typedef struct
{
    uint16_t buffer[ADC_AVG_SIZE];
    uint32_t sum;
    uint8_t  idx;
} adc_avg_t;

typedef struct
{
    uint32_t y;    // valor filtrado escalado
    uint8_t  alpha; // 1..255 (más chico = más filtrado)
} adc_iir_t;


typedef enum
{
    ADC_CH_PA0  = 0,   // ADC_IN0
    ADC_CH_PA1  = 1,   // ADC_IN1
    ADC_CH_PA2  = 2,   // ADC_IN2
    ADC_CH_PA3  = 3,   // ADC_IN3
    ADC_CH_PA4  = 4,   // ADC_IN4
    ADC_CH_PA5  = 5,   // ADC_IN5
    ADC_CH_PA6  = 6,   // ADC_IN6
    ADC_CH_PA7  = 7,   // ADC_IN7

    ADC_CH_PB0  = 8,   // ADC_IN8
    ADC_CH_PB1  = 9,   // ADC_IN9

    ADC_CH_PC0  = 10,  // ADC_IN10
    ADC_CH_PC1  = 11,  // ADC_IN11
    ADC_CH_PC2  = 12,  // ADC_IN12
    ADC_CH_PC3  = 13,  // ADC_IN13
    ADC_CH_PC4  = 14,  // ADC_IN14
    ADC_CH_PC5  = 15,  // ADC_IN15

    ADC_CH_TEMP = 16,  // Internal temperature sensor
    ADC_CH_VREF = 17   // Internal Vrefint

} adc_channel_t;

void ADC1_Init(uint8_t channel);
void ADC1_Init_Temperature(void);
void ADC2_Init(uint8_t channel);
int16_t ADC1_ReadTemperature(void);
uint16_t ADC1_Read(void);
uint16_t ADC2_Read(void);
void ADC1_Init_Multi(uint8_t *channels, uint8_t n);
uint16_t ADC1_Read_Channel(uint8_t channel_index);



#endif