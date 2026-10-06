#ifndef __ADC_H_
#define __ADC_H_
struct adc_struct;
typedef struct adc_struct* adc_handler;
void adc_init(adc_handler adc_x);
float vbat_get(adc_handler adc_x);
#endif
