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

#ifndef WEIGHT_SENSOR_H_
#define WEIGHT_SENSOR_H_

#include <Arduino.h>
#include "errors.h"

typedef enum {
    SCALE_KILOGRAMS = 0x0,
    SCALE_LBS = 0x1

} weight_scale_t;

typedef struct weight_ctx
{
    int   calibrating_factor;
    weight_scale_t weight_scale;
    uint8_t enable;
} weight_ctx_t;


/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/


/*
 * crweight
 *
 * @brief CoAP Resource weight sensor
 *
 */
error_t crweight( struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it );

/*
 * arduino_disab_weight
 *
 * @brief
 *
 */
error_t arduino_disab_weight();

/*
 * arduino_enab_weight
 *
 * @brief
 *
 */
error_t arduino_enab_weight();

/*
 * arduino_weight_sensor_init
 *
* @brief
 *
 */
error_t arduino_weight_sensor_init();

/**
 * @brief Get sensor weight

 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_weight(struct mbuf *m, uint8_t *len);


/* @brief CoAP Get weight calibration factor
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_weight_calib_factor(struct mbuf *m, uint8_t *len);

/**
 * @brief CoAP put weight sensor scale unit configuration
 *
 * @param[in] measurement scale (kg lbs)
 * @return error_t
 */
error_t arduino_put_weight_cfg_scale(weight_scale_t weight_scale);

/**
 * @brief CoAP GET weight conversion time
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_weight_cfg_scale(struct mbuf *m, uint8_t *len);





/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/
/*
 * weight_sensor_read
 *
 * @param p: if error_t is ERR_OK, weight reading will be returned.
 *
 */
error_t weight_sensor_read(float * p);

/*
 * @brief  Enable sensor
 * @return error_t
 */
error_t weight_sensor_enable(void);

/*
 * @brief  Disable sensor
 * @return error_t
 */
error_t weight_sensor_disable(void);



#endif /* WEIGHT_SENSOR_H_ */
