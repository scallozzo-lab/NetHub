#include "adc.h"
#include "main.h"

static uint16_t prev1 = 0;
static uint16_t prev2 = 0;
static adc_iir_t iir;

static inline void IWDG_Refresh(void)
{
    IWDG->KR = 0xAAAA;   // clave para recargar el contador
}

/*
// Inicializa ADC1 en un canal específico (PA0–PA7, PB0–PB1)
void ADC1_Init(uint8_t channel)
{
    // Habilitar reloj ADC1 y GPIO
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    // Configurar pin como analógico
    if(channel <= 7)       GPIOA->CRL &= ~(0xF << (4*channel));
    else if(channel <= 9)  GPIOB->CRL &= ~(0xF << (4*(channel-8)));

    // ADC clock = PCLK2 / 6 → 12 MHz < 14 MHz
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

    // Configurar ADC
    ADC1->SQR1 = 0;             // 1 conversión
    ADC1->SQR3 = channel;       // primer canal de la secuencia
    ADC1->SMPR2 |= (0x7 << (3*channel)); // tiempo máximo de sample

    ADC1->CR2 = ADC_CR2_ADON;   // encender ADC
}
*/
// Inicializa ADC1 en un canal específico (PA0–PA7, PB0–PB1)
void ADC1_Init(uint8_t channel)
{
    // Habilitar reloj ADC1 y GPIO
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    // Configurar pin como analógico
    if(channel <= 7)       GPIOA->CRL &= ~(0xF << (4*channel));
    else if(channel <= 9)  GPIOB->CRL &= ~(0xF << (4*(channel-8)));

    // ADC clock = PCLK2 / 6 → 12 MHz < 14 MHz
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

    ADC1->CR2 &= ~ADC_CR2_ADON;          // apagar ADC
    
    // Configurar ADC
    ADC1->SQR1 = 0;             // 1 conversión
    ADC1->SQR3 = channel;       // primer canal de la secuencia
    ADC1->SMPR2 |= (0x7 << (3*channel)); // tiempo máximo de sample

    //ADC1->CR2 &= ~ADC_CR2_ADON;          // apagar ADC
    //ADC1->SMPR2 &= ~(0x7 << (3*channel));
    //ADC1->SMPR2 |=  (0x7 << (3*channel)); 
   
    // Configurar tiempo de muestreo máximo
    //ADC1->SMPR2 &= ~(0x7 << (3*channel));  // limpiar bits
    //ADC1->SMPR2 |=  (0x7 << (3*channel));  // máximo sample time (~239 ciclos)

    ADC1->CR2 = ADC_CR2_ADON;   // encender ADC

    ADC1->CR2 |= ADC_CR2_CAL;           // iniciar calibración
    while(ADC1->CR2 & ADC_CR2_CAL);     // esperar a que termine
}



void ADC1_Init_Temperature(void)
{
    // Enable ADC1 clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // ADC clock = PCLK2 / 6 = 12 MHz
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

    // Enable temperature sensor and Vrefint
    ADC1->CR2 |= ADC_CR2_TSVREFE;

    // Single conversion
    ADC1->SQR1 = 0;
    ADC1->SQR3 = 16;   // Channel 16 = temperature sensor

    // Sampling time for channel 16 → MAX (239.5 cycles)
    ADC1->SMPR1 |= (0x7 << 18); // 3*(16-10)=18

    // Enable ADC
    ADC1->CR2 |= ADC_CR2_ADON;
}

uint16_t ADC1_Read(void)
{

    ADC1->CR2 |= ADC_CR2_ADON;          // iniciar conversión
    ADC1->CR2 |= ADC_CR2_ADON;          // doble escritura en F1 inicia la conversión
    while(!(ADC1->SR & ADC_SR_EOC));    // esperar fin de conversión
    
    return ADC1->DR;                     // valor 12 bits (0–4095)
}

uint16_t ADC1_ReadChannel(uint8_t ch)
{
    ADC1->SQR3 = ch;
    ADC1->CR2 |= ADC_CR2_ADON;
    while(!(ADC1->SR & ADC_SR_EOC));
    return ADC1->DR;
}

int32_t ADC_ReadVdda_mV(void)
{
    uint16_t adc_vref = ADC1_ReadChannel(17);
    return (1200 * 4095) / adc_vref;
}


int16_t ADC1_ReadTemperature(void)
{
    const int32_t V25_mV = 1430;
    const int32_t Avg_Slope_uV = 4300;

    int32_t Vdda_mV = ADC_ReadVdda_mV();
    uint16_t adc_temp = ADC1_ReadChannel(16);

    int32_t Vsense_mV = (adc_temp * Vdda_mV) / 4095;

    int32_t temp_x10 =
        ((V25_mV - Vsense_mV) * 10000 / Avg_Slope_uV) + 250;

    return (int16_t)temp_x10;
}

// Inicializa ADC2 en un canal específico (PA0–PA7, PB0–PB1)
void ADC2_Init(uint8_t channel)
{
    // Habilitar reloj ADC2 y GPIO
    RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    // Configurar pin como analógico
    if(channel <= 7)       GPIOA->CRL &= ~(0xF << (4*channel));
    else if(channel <= 9)  GPIOB->CRL &= ~(0xF << (4*(channel-8)));

    // ADC clock
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

    // Configurar ADC2
    ADC2->SQR1 = 0;             
    ADC2->SQR3 = channel;       
    ADC2->SMPR2 |= (0x7 << (3*channel));

    ADC2->CR2 = ADC_CR2_ADON;
}

uint16_t ADC2_Read(void)
{
    ADC2->CR2 |= ADC_CR2_ADON;
    ADC2->CR2 |= ADC_CR2_ADON;
    while(!(ADC2->SR & ADC_SR_EOC));
    return ADC2->DR;
}

void ADC1_Init_Multi(uint8_t *channels, uint8_t n)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN;

    // Configurar pines como analógico
    for(uint8_t i=0; i<n; i++)
    {
        uint8_t ch = channels[i];
        if(ch <= 7) GPIOA->CRL &= ~(0xF << (4*ch));
        // PB0/PB1 si fuera necesario
    }

    ADC1->SQR1 = ((n-1) << 20); // L = n conversions
    for(uint8_t i=0; i<n; i++)
    {
        if(i < 6) ADC1->SQR3 |= (channels[i] << (5*i)); // canales 1–6 en SQR3
        else ADC1->SQR2 |= (channels[i] << (5*(i-6))); // canales 7–14 en SQR2
    }

    ADC1->SMPR2 = 0x7FFFFFFF; // máximo sample time para todos
    ADC1->CR2 = ADC_CR2_ADON; // encender ADC
}

uint16_t ADC1_Read_Channel(uint8_t channel_index)
{
    ADC1->CR2 |= ADC_CR2_ADON; // iniciar conversión
    ADC1->CR2 |= ADC_CR2_ADON;
    while(!(ADC1->SR & ADC_SR_EOC));
    return ADC1->DR; // devuelve el valor del canal actual
}


void adc_avg_init(adc_avg_t *f)
{
    f->sum = 0;
    f->idx = 0;
    for(int i = 0; i < ADC_AVG_SIZE; i++)
        f->buffer[i] = 0;
}

uint16_t adc_avg_filter(adc_avg_t *f, uint16_t sample)
{
    f->sum -= f->buffer[f->idx];
    f->buffer[f->idx] = sample;
    f->sum += sample;

    f->idx = (f->idx + 1) & (ADC_AVG_SIZE - 1);

    return (uint16_t)(f->sum / ADC_AVG_SIZE);
}

void adc_iir_init(adc_iir_t *f, uint16_t init)
{
    f->y = init << 8;  // Q8
    f->alpha = 16;    // ajustar a gusto
}

uint16_t adc_iir_filter(adc_iir_t *f, uint16_t x)
{
    f->y += f->alpha * ((x << 8) - f->y) >> 8;
    return (uint16_t)(f->y >> 8);
}

uint16_t adc_median3(uint16_t a, uint16_t b, uint16_t c)
{
    if ((a > b) != (a > c)) return a;
    else if ((b > a) != (b > c)) return b;
    else return c;
}

void adc_filter_init(uint16_t first_sample)
{
    prev1 = first_sample;
    prev2 = first_sample;
    adc_iir_init(&iir, first_sample);
}

uint16_t adc_filter_process(uint16_t raw)
{
    uint16_t med;

    /* Filtro mediana (anti-spikes) */
    med = adc_median3(raw, prev1, prev2);

    /* Actualizar historial */
    prev2 = prev1;
    prev1 = raw;

    /* Filtro IIR (low-pass) */
    return adc_iir_filter(&iir, med);
}

#ifdef _TEST_ADC

#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define ADC_SAMPLES             100UL

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


static inline void ADC_WaitNextSample(uint32_t *next)
{
    *next += ADC_SAMPLE_CYCLES;

    while((int32_t)(DWT->CYCCNT - *next) < 0)
    {
        IWDG_Refresh();
    }
}


void test_adc(void)
{
    static uint8_t flaginit = 0;

    static uint8_t filter_init = 0;
    static int32_t current_filtered_ma = 0;

    if(!flaginit)
    {
        ADC1_Init(ADC_CH_PA1);
        flaginit = 1;
    }

    while(1)
    {
        uint64_t sum    = 0;
        uint64_t sum_sq = 0;

        uint32_t mv_min = ADC_VDDA_MV;
        uint32_t mv_max = 0;

        uint32_t next = DWT->CYCCNT;


        // =====================================================
        // Captura: 1000 muestras @ 1kHz = 1 segundo
        // =====================================================

        for(uint32_t i = 0; i < ADC_SAMPLES; i++)
        {
            uint16_t adc = ADC1_Read();

            uint32_t mv =
                ((uint32_t)adc * ADC_VDDA_MV) / 4095UL;

            sum += mv;

            sum_sq +=
                (uint64_t)mv * (uint64_t)mv;

            if(mv < mv_min)
                mv_min = mv;

            if(mv > mv_max)
                mv_max = mv;

            ADC_WaitNextSample(&next);
        }


        // =====================================================
        // Offset
        // =====================================================

        uint32_t offset_mv =
            (uint32_t)(sum / ADC_SAMPLES);


        // =====================================================
        // True RMS de la componente AC
        // =====================================================

        uint64_t mean_sq =
            sum_sq / ADC_SAMPLES;

        uint64_t offset_sq =
            (uint64_t)offset_mv *
            (uint64_t)offset_mv;

        uint32_t vrms_mv = 0;

        if(mean_sq > offset_sq)
        {
            uint64_t ac_mean_sq =
                mean_sq - offset_sq;

            vrms_mv =
                (uint32_t)sqrt((double)ac_mean_sq);
        }


        // =====================================================
        // Conversión a corriente
        // =====================================================

        uint32_t current_ma =
            (vrms_mv * 1000UL) /
            ACS712_MV_PER_AMP;


        // =====================================================
        // Filtro IIR sobre el resultado RMS
        // =====================================================

        if(!filter_init)
        {
            current_filtered_ma = current_ma;
            filter_init = 1;
        }
        else
        {
            current_filtered_ma +=
                ((int32_t)current_ma -
                 current_filtered_ma) /
                CURRENT_FILTER_DIV;
        }


        // =====================================================
        // Debug
        // =====================================================

        printf(
            "ADC RMS: "
            "min=%lu mV  "
            "max=%lu mV  "
            "offset=%lu mV  "
            "Vrms=%lu mV  "
            "I=%lu mA  "
            "Ifilt=%ld mA\r\n",

            (unsigned long)mv_min,
            (unsigned long)mv_max,
            (unsigned long)offset_mv,
            (unsigned long)vrms_mv,
            (unsigned long)current_ma,
            (long)current_filtered_ma
        );

        IWDG_Refresh();
    }
}
#endif