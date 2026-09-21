#include "_netcontrol.h"

static stNetvalues NetValues = {0};
  
uint32_t _GetNetVoltage(void)
{
    return NetValues.netvoltage;    
}

uint32_t _GetNetCurrent(void)
{
    return NetValues.netcurrent;    
}

static inline void ADC_WaitNextSample(uint32_t *next)
{
    *next += ADC_SAMPLE_CYCLES;

    while((int32_t)(DWT->CYCCNT - *next) < 0)
    {
        //IWDG_Refresh();
    }
}

void ProcNetValues(void)
{
    static uint8_t flaginit = 0;

    static uint8_t filter_init = 0;
    static int32_t current_filtered_ma = 0;

    if(!flaginit)
    {
        ADC1_Init(ADC_CH_PA1);
        flaginit = 1;
        // trucho sacar
        NetValues.netvoltage = 219;
    }

    //while(1)
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
        /*
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
*/
        NetValues.netcurrent = current_ma;
        //IWDG_Refresh();
    }
}
