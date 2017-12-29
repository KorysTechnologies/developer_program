/*

Copyright (c) Silver Spring Networks, Inc.
All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the ""Software""), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Silver Spring Networks, Inc.
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Silver Spring
Networks, Inc.

*/

#ifndef MQ6_SENSOR_H_
#define MQ6_SENSOR_H_

#include <Arduino.h>
#include "errors.h"

typedef enum {
    MQ6_LPG = 0x0,
    MQ6_CH4 = 0x1

} mq6_gas_meas_t;

/* MQ6 RL and RO Values */
#define MQ6_RLVALUE 20 //define MQ6 board load resistance value in kOhms
#define MQ6_ROFACTOR 9.83 //define MQ6 sensor resistance in clean air factor

typedef struct mq6_ctx
{
	mq6_gas_meas_t mq6_gas_meas; //type of mq6 gas measurement
    float mq6_Rovalue;	//value of sensor resistor in clean air after calibration
    uint8_t  enable;
} mq6_ctx_t;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/


/*
 * crmq6
 *
 * @brief CoAP Resource MQ6 sensor
 *
 */
error_t crmq6( struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it );

/*
 * arduino_disab_mq6
 *
 * @brief
 *
 */
error_t arduino_disab_mq6();

/*
 * arduino_enab_mq6
 *
 * @brief
 *
 */
error_t arduino_enab_mq6();

/*
 * arduino_mq6_sensor_init
 *
 * @brief Initialize and calibrates MQ6 sensor. It lust be done in an clean air environment
 *
 */
error_t arduino_mq6_sensor_init();

/**
 * @brief Get sensor MQ6 measurement result

 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_mq6(struct mbuf *m, uint8_t *len);

/* @brief
 * arduino_put_mq6_cfg
 *
 * Set MQ6 Measurement Type
 */
error_t arduino_put_mq6_cfg( mq6_gas_meas_t cfg );

/**
 * @brief CoAP GET MQ6 conversion time

 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_mq6_cfg(struct mbuf *m, uint8_t *len);



/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

/*
 * mq6_sensor_calibration
 *
 * @param[in] rl - MQ6 board load resistance value
 * @return sensor resistor in clean air after calibration
 */
float mq6_sensor_calibration(float mq6_rlvalue);

/*
 * mq6_sensor_read
 *
 * @param p: if error_t is ERR_OK, MQ6 reading will be returned.
 *
 */
float mq6_sensor_read(float p);

/*
 * mq6_sensor_get_gasppm
 *
 * @param[in] rs_ro_ratio
 * @param[in] gas_type
 * @return ppm (parts per million) value based on gas type
 */
int mq6_sensor_get_gasppm(float rs_ro_ration, int gas_type);


 /* mq6_sensor_find_interval
 *
 * @param[in] array - LPG or CH4 arrays
 * @param[in] array size
 * @return index of Rs/Ro interval based on the datasheet curve
 */
int mq6_sensor_find_interval(float* array, int array_length, float search_value);

/*
 * @brief  Enable sensor
 * @return error_t
 */
error_t mq6_sensor_enable(void);

/*
 * @brief  Disable sensor
 * @return error_t
 */
error_t mq6_sensor_disable(void);



#endif /* MQ6_SENSOR_H_ */
