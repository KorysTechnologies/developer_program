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

#ifndef BME280_SENSOR_H_
#define BME280_SENSOR_H_

#include <Arduino.h>
#include "errors.h"

#define SEALEVELPRESSURE_HPA (1013.25)

#define BME280_INVALID_HUMIDITY -100
#define BME280_INVALID_PRESSURE -100

typedef enum
{
	BME280_INVALID_TEMP = -100,
	BME280_CELSIUS_SCALE = 0x43,
	BME280_FAHRENHEIT_SCALE = 0x46,
} bme280_temp_scale_t;


typedef enum
{
    BME280_INVALID_ALTITUDE = -100,
    BME280_METER_SCALE = 0x4D,
    BME280_FEET_SCALE = 0x46,
} bme280_altitude_scale_t;


typedef struct bme280_ctx 
{
	bme280_temp_scale_t bme280_temp_scale;
	bme280_altitude_scale_t bme280_altitude_scale;
    uint8_t enable;
} bme280_ctx_t;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/


/*
 * crbme280
 *
 * @brief CoAP Resource bme280 sensor
 *
 */
error_t crbme280( struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it );

/*
 * disab_bme280
 *
 * @brief
 *
 */
error_t arduino_disab_bme280();

/*
 * enab_bme280
 *
 * @brief
 *
 */
error_t arduino_enab_bme280();

/*
 * bme280_sensor_init
 *
 *
 */
error_t arduino_bme280_sensor_init();

/**
 * @brief CoAP put bme280 temp config
 *
 * @param[in] scale Celsius or Fahrenheit
 * @return error_t
 */
error_t arduino_put_bme280_temp_cfg( bme280_temp_scale_t scale );

/**
 * @brief CoAP Server get bme280 temp config
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_bme280_temp_cfg(struct mbuf *m, uint8_t *len);


/**
 * @brief CoAP put bme280 altitude config
 *
 * @param[in] scale Meter or Feet
 * @return error_t
 */
error_t arduino_put_bme280_altitude_cfg( bme280_altitude_scale_t scale );

/**
 * @brief CoAP Server get bme280 altitude config
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_bme280_altitude_cfg(struct mbuf *m, uint8_t *len);


/**
 *
 * @brief Read bme280 sensor temperature and return a message and length of message
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_bme280_temp( struct mbuf *m, uint8_t *len );


/**
 *
 * @brief Read bme280 sensor pressure and return a message and length of message
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_bme280_pressure( struct mbuf *m, uint8_t *len );

/**
 *
 * @brief Read bme280 sensor altitude and return a message and length of message
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_bme280_altitude( struct mbuf *m, uint8_t *len );


/**
 *
 * @brief Read bme280 sensor humidity and return a message and length of message
 *
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_bme280_humidity( struct mbuf *m, uint8_t *len );


/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

/*
 * bme280_sensor_read_temp
 *
 * @param p: if error_t is ERR_OK, temperature reading will be returned.
 *
 */
error_t bme280_sensor_read_temp(float * p);


/*
 * bme280_sensor_read_pressure
 *
 * @param p: if error_t is ERR_OK, pressure reading will be returned.
 *
 */
error_t bme280_sensor_read_pressure(float * p);


/*
 * bme280_sensor_read_humidity
 *
 * @param p: if error_t is ERR_OK, humidity reading will be returned.
 *
 */
error_t bme280_sensor_read_humidity(float * p);

/*
 * bme280_sensor_read_altitude
 *
 * @param p: if error_t is ERR_OK, altitude reading will be returned.
 *
 */
error_t bme280_sensor_read_altitude(float * p);


/*
 * bme280_sensor_read_sealevelforaltitude
 *
 * @param p: if error_t is ERR_OK, sealevelforaltitude reading will be returned.
 *
 */
error_t bme280_sensor_read_sealevelforaltitude(float * p);

/*
 * @brief  Enable sensor
 * @return error_t
 */
error_t bme280_sensor_enable(void);

/*
 * @brief  Disable sensor
 * @return error_t
 */
error_t bme280_sensor_disable(void);



#endif /* BME280_SENSOR_H_ */
