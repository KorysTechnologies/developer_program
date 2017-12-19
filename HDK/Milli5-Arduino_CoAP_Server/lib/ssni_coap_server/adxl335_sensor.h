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

#ifndef ADXL335_SENSOR_H_
#define ADXL335_SENSOR_H_

#include <Arduino.h>
#include "errors.h"

typedef enum
{
	ADXL335_INVALID_MEAS = -100,
	ADXL335_MEAS_G = 0x47,
	ADXL335_MEAS_ROTATION = 0x52,
	ADXL335_MEAS_RAWDATA = 0x4F
} adxl335_measurement_type_t;


typedef struct adxl335_ctx
{
	adxl335_measurement_type_t meas_type;
    uint8_t enable;
} adxl335_ctx_t;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/


/*
 * cradxl335
 *
 * @brief CoAP Resource adxl335 sensor
 *
 */
error_t cradxl335( struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it );

/*
 * disab_adxl335
 *
 * @brief
 *
 */
error_t arduino_disab_adxl335();

/*
 * enab_adxl335
 *
 * @brief
 *
 */
error_t arduino_enab_adxl335();

/*
 * adxl335_sensor_init
 *
 *
 */
error_t arduino_adxl335_sensor_init();

/**
 * @brief CoAP put adxl335 measurement configuration
 *
 * @param[in] measurement type G, Rotation or RawData
 * @return error_t
 */
error_t arduino_put_adxl335_meas_cfg( adxl335_measurement_type_t meas_type );

/**
 * @brief CoAP Server get adxl335 measurement configuration
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_adxl335_meas_cfg(struct mbuf *m, uint8_t *len);


/**
 * @brief Read adxl335 x axis value
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_adxl335_xaxis( struct mbuf *m, uint8_t *len );


/**
 * @brief Read adxl335 y axis value
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_adxl335_yaxis( struct mbuf *m, uint8_t *len );

/**
 * @brief Read adxl335 z axis value
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_adxl335_zaxis( struct mbuf *m, uint8_t *len );


/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

/*
 * adxl335_sensor_read_gforce
 *
 * @param p: if error_t is ERR_OK, temperature reading will be returned.
 *
 */
error_t adxl335_sensor_convert_gforce( float Value_raw, float * result );


/*
 * adxl335_sensor_read_rotation
 *
 * @param p: if error_t is ERR_OK, pressure reading will be returned.
 *
 */
error_t adxl335_sensor_convert_rotation(double param1, double param2, float * result );

/*
 * @brief  Enable sensor
 * @return error_t
 */
error_t adxl335_sensor_enable(void);

/*
 * @brief  Disable sensor
 * @return error_t
 */
error_t adxl335_sensor_disable(void);



#endif /* ADXL335_SENSOR_H_ */
