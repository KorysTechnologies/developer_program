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

#ifndef ALSOPT3001_SENSOR_H_
#define ALSOPT3001_SENSOR_H_

#include <Arduino.h>
#include "errors.h"
#include <ClosedCube_OPT3001.h>


typedef enum {
    ALS_SHUTDOWN = 0x0,
    ALS_SINGLESHOT = 0x1,
    ALS_CONTINUOUS1 = 0x2,
    ALS_CONTINUOUS2 = 0x3
} als_sensor_conversionmode_t;

typedef enum {
    ALS_CONVTIME_100MS = 0x0,
    ALS_CONVTIME_800MS = 0x1
} als_sensor_conversiontime_t;

typedef struct als_sensor_cfg
{
    int8_t  als_range_number;
    als_sensor_conversiontime_t  als_conversion_time;
    uint8_t  als_latch;
    als_sensor_conversionmode_t  als_conversion_mode;

} als_sensor_cfg_t;


typedef struct als_ctx
{
    als_sensor_cfg_t   cfg;
    uint8_t  enable;
} als_ctx_t;


/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/


/*
 * crals
 *
 * @brief CoAP Resource als sensor
 *
 */
error_t crals( struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it );

/*
 * arduino_disab_als
 *
 * @brief
 *
 */
error_t arduino_disab_als();

/*
 * arduino_enab_zld
 *
 * @brief
 *
 */
error_t arduino_enab_als();

/*
 * arduino_als_sensor_init
 *
* @brief
 *
 */
error_t arduino_als_sensor_init();

/**
 * @brief Get sensor als

 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_als(struct mbuf *m, uint8_t *len);

/**
 * @brief CoAP PUT als config conversion time
 *
 * @param[in] conversion time
 * @return error_t
 */
error_t arduino_put_als_cfg_conversiontime( als_sensor_conversiontime_t convtime );


/**
 * @brief CoAP GET als conversion time

 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_als_cfg_conversiontime(struct mbuf *m, uint8_t *len);


/**
 * @brief CoAP PUT als config conversion time
 *
 * @param[in] conversion time
 * @return error_t
 */
error_t arduino_put_als_cfg_conversionmode( als_sensor_conversionmode_t convmode );


/**
 * @brief CoAP GET als conversion time

 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_als_cfg_conversionmode(struct mbuf *m, uint8_t *len);



/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

void configureSensor();

void newconfigureSensor() ;

void printResult(String text, OPT3001 result);

void printError(String text, OPT3001_ErrorCode error);

/*
 * als_sensor_read
 *
 * @param p: if error_t is ERR_OK, als reading will be returned.
 *
 */
error_t als_sensor_read(float * p);


/*
 * @brief  Get thresholds for als sensor alerts
 * @return error_t
 */
error_t als_sensor_cfg_set(const struct als_sensor_cfg *cfg);

/*
 * @brief  Enable sensor
 * @return error_t
 */
error_t als_sensor_enable(void);

/*
 * @brief  Disable sensor
 * @return error_t
 */
error_t als_sensor_disable(void);



#endif /* ALSOPT3001_SENSOR_H_ */
