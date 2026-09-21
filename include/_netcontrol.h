#ifndef __NETCONTROL_H__
    #define __NETCONTROL_H__

#include "main.h"
#include "adc.h"

#define ADC_SAMPLES             10UL

#define CPU_FREQ_HZ             72000000UL

#define ADC_SAMPLE_US           1000UL
#define ADC_SAMPLE_CYCLES       ((CPU_FREQ_HZ / 1000000UL) * ADC_SAMPLE_US)

#define ADC_VDDA_MV             3300UL

// Sensibilidad efectiva medida del conjunto
// ACS712
#define ACS712_MV_PER_AMP       185UL

// Filtro IIR de salida
// 2 = rápido
// 4 = medio
// 8 = suave
// 16 = muy suave
#define CURRENT_FILTER_DIV      16

typedef struct
{
    uint16_t netvoltage;
    uint32_t netcurrent;
}stNetvalues;

uint32_t _GetNetVoltage(void);
uint32_t _GetNetCurrent(void);


void ProcNetValues(void);


#endif