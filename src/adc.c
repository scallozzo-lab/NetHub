#include "adc.h"

static uint16_t prev1 = 0;
static uint16_t prev2 = 0;
static adc_iir_t iir;


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


void test_adc(void)
{
    static uint8_t flaginit = 0;
    if(!flaginit)
        adc_filter_init(0);
    flaginit = 1;

    //printf("Temp %lu\n", ADC1_ReadTemperature());
    
    int16_t t = ADC1_ReadTemperature();

    printf("Temp: %d.%d C\n", t / 10, abs(t % 10));
    //uint16_t val1 = ADC1_Read();
    //uint32_t mv = val1 * 3300 / 4095; // milivoltios
    //printf("adc1 = %lu mV\n", mv);

    uint16_t val2 = ADC2_Read();
    uint32_t mv2 = val2 * 3300 / 4095; // milivoltios
    printf("adc2 = %lu mV\n", mv2);

    uint16_t filtrado = adc_filter_process(val2);
    printf("adc2 Filtro= %lu mV\n", filtrado);

}