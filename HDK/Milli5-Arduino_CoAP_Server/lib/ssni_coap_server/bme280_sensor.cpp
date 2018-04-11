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


The circuit configuration - Adafruit BME280
  - GND pin: ground (Milli5 Arduino Shield board - J15 pin6 )
  - VDD pin: 5V (Milli5 Arduino Shield board - J15 pin5 )
  - SDA pin: 9 (Milli5 Arduino Shield board - J6 pin9 - I2C_SDA M0 Pro)
  - SCL pin: 10 (Milli5 Arduino Shield board - J6 pin10 - I2C_SCL M0 Pro )
  
To set the bme280 sensor configuration:

Set bme280 temperature
PUT /sensor/arduino/bme280?cfg_temp=C
PUT /sensor/arduino/bme280?cfg_temp=F

Set bme280 altitude Meter
PUT /sensor/arduino/bme280?cfg_altitude=M

Set bme280 altitude Feet
PUT /sensor/arduino/bme280?cfg_altitude=F

To get the bme280 sensor configuration:

Get bme280 temperature
GET /sensor/arduino/bme280?cfg_temp

Get bme280 altitude
GET /sensor/arduino/bme280?cfg_altitude

To read the bme280 sensor value:
GET /sensor/arduino/bme280?sens_temp
GET /sensor/arduino/bme280?sens_pressure
GET /sensor/arduino/bme280?sens_altitude
GET /sensor/arduino/bme280?sens_humidity

To request an observe on the bme280 sensor value:
GET /sensor/arduino/bme280?sens_temp with the CoAP observe header set
GET /sensor/arduino/bme280?sens_pressure with the CoAP observe header set
GET /sensor/arduino/bme280?sens_altitude with the CoAP observe header set
GET /sensor/arduino/bme280?sens_humidity with the CoAP observe header set

DELETE for disabling bme280 sensor
DELETE /sensor/arduino/bme280?all


*/

#include <string.h>

#include <Adafruit_BME280.h>

#include "log.h"
#include "bufutil.h"
#include "exp_coap.h"
#include "coap_rsp_msg.h"
#include "coappdu.h"
#include "coapobserve.h"
#include "coapsensorobs.h"
#include "bme280_sensor.h"
#include "arduino_pins.h"

Adafruit_BME280 bme;

bme280_ctx_t bme280_ctx;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/

/*
 * crtemperature
 *
 * @brief CoAP Resource Bosch bme280 sensor
 *
 */
error_t crbme280(struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it)
{
    struct optlv *o;
	uint8_t obs = false;


    /* No URI path beyond /bme280 is supported, so reject if present. */
    o = copt_get_next_opt_type((const sl_co*)&(req->oh), COAP_OPTION_URI_PATH, &it);
    if (o)
    {
        rsp->code = COAP_RSP_404_NOT_FOUND;
        goto err;
    }            

    /* All methods require a query, so return an error if missing. */
    if (!(o = copt_get_next_opt_type((const sl_co*)&(req->oh), COAP_OPTION_URI_QUERY, NULL))) 
    {
        rsp->code = COAP_RSP_405_METHOD_NOT_ALLOWED;
        goto err;
    }
    
    /*
     * PUT for writing configuration or enabling sensors
     */
    if (req->code == COAP_REQUEST_PUT) 
    {
        error_t rc = ERR_OK;

        /* PUT /bme280?cfg_temp=<C|F> */
        if (!coap_opt_strcmp(o, "cfg_temp=C"))
        {
			arduino_put_bme280_temp_cfg(BME280_CELSIUS_SCALE);
        } 
        else if (!coap_opt_strcmp(o, "cfg_temp=F"))
        {
			arduino_put_bme280_temp_cfg(BME280_FAHRENHEIT_SCALE);
        }
        /* PUT /bme280?cfg_altitude=<M|F> */
        else if (!coap_opt_strcmp(o, "cfg_altitude=M"))
        {
            arduino_put_bme280_altitude_cfg(BME280_METER_SCALE);
        }
        else if (!coap_opt_strcmp(o, "cfg_altitude=F"))
        {
            arduino_put_bme280_altitude_cfg(BME280_FEET_SCALE);
        }
        else
        {
            /* Not supported query. */
            rsp->code = COAP_RSP_501_NOT_IMPLEMENTED;
            goto err;
        }
        
        if (!rc)
        {
            rsp->code = COAP_RSP_204_CHANGED;
            rsp->plen = 0;
        }
        else
        {
            switch (rc)
            {
            case ERR_BAD_DATA:
            case ERR_INVAL:
                rsp->code = COAP_RSP_406_NOT_ACCEPTABLE;
                break;
            default:
                rsp->code = COAP_RSP_500_INTERNAL_ERROR;
                break;
            }
            goto err;
        }
    } // if PUT
    
    /*
     * GET for reading sensor or config information
     */
    else if (req->code == COAP_REQUEST_GET)
    {
        uint8_t rc, len = 0;

        /* Config or sensor values. */
        /* GET /bme280?cfg_temp */
        if (!coap_opt_strcmp(o, "cfg_temp"))
        {
            /* get temperature config */
            rc = arduino_get_bme280_temp_cfg( rsp->msg, &len );
        }
         /* GET bme280?cfg_altitude */
        else if (!coap_opt_strcmp(o, "cfg_altitude"))
        {
            /* get temperature config */
            rc = arduino_get_bme280_altitude_cfg( rsp->msg, &len );
        }
        /* GET /bme280?sens_temp */
        else if (!coap_opt_strcmp(o, "sens_temp"))
        {
			if ((o = copt_get_next_opt_type((sl_co*)&(req->oh), COAP_OPTION_OBSERVE, NULL)))
			{
				uint32_t obsval = co_uint32_n2h(o);
				switch(obsval)
				{
					case COAP_OBS_REG:
						rc = coap_obs_reg();
						obs = true;
						break;

					case COAP_OBS_DEREG:
						rc = coap_obs_dereg();
						break;

					default:
						rc = ERR_INVAL;

				} // switch
			}
			else
			{
				/* Get sensor temperature value */
				rc = arduino_get_bme280_temp( rsp->msg, &len );
			} // if-else
        }
        /* GET /bme280?sens_pressure */
        else if (!coap_opt_strcmp(o, "sens_pressure"))
        {
            if ((o = copt_get_next_opt_type((sl_co*)&(req->oh), COAP_OPTION_OBSERVE, NULL)))
            {
                uint32_t obsval = co_uint32_n2h(o);
                switch(obsval)
                {
                    case COAP_OBS_REG:
                        rc = coap_obs_reg();
                        obs = true;
                        break;

                    case COAP_OBS_DEREG:
                        rc = coap_obs_dereg();
                        break;

                    default:
                        rc = ERR_INVAL;

                } // switch
            }
            else
            {
                /* Get sensor pressure value */
                rc = arduino_get_bme280_pressure( rsp->msg, &len );
            } // if-else
        }
        /* GET /bme280?sens_altitude */
        else if (!coap_opt_strcmp(o, "sens_altitude"))
        {
            if ((o = copt_get_next_opt_type((sl_co*)&(req->oh), COAP_OPTION_OBSERVE, NULL)))
            {
                uint32_t obsval = co_uint32_n2h(o);
                switch(obsval)
                {
                    case COAP_OBS_REG:
                        rc = coap_obs_reg();
                        obs = true;
                        break;

                    case COAP_OBS_DEREG:
                        rc = coap_obs_dereg();
                        break;

                    default:
                        rc = ERR_INVAL;

                } // switch
            }
            else
            {
                /* Get sensor altitude value */
                rc = arduino_get_bme280_altitude( rsp->msg, &len );
            } // if-else
        }
                /* GET /bme280?sens_humidity */
        else if (!coap_opt_strcmp(o, "sens_humidity"))
        {
            if ((o = copt_get_next_opt_type((sl_co*)&(req->oh), COAP_OPTION_OBSERVE, NULL)))
            {
                uint32_t obsval = co_uint32_n2h(o);
                switch(obsval)
                {
                    case COAP_OBS_REG:
                        rc = coap_obs_reg();
                        obs = true;
                        break;

                    case COAP_OBS_DEREG:
                        rc = coap_obs_dereg();
                        break;

                    default:
                        rc = ERR_INVAL;

                } // switch
            }
            else
            {
                /* Get sensor humidity value */
                rc = arduino_get_bme280_humidity( rsp->msg, &len );
            } // if-else
        }
        else {
            /* Don't support other queries. */
            rsp->code = COAP_RSP_501_NOT_IMPLEMENTED;
            goto err;
        }
        dlog(LOG_DEBUG, "GET (status %d) read %d bytes.", rc, len);
        if (!rc) {
			if (obs)
			{                        
			/* Good code, but no content. */
				rsp->code = COAP_RSP_203_VALID;
				rsp->plen = 0;
			}
			else
			{
				rsp->plen = len;
				rsp->cf = COAP_CF_CSV;
				rsp->code = COAP_RSP_205_CONTENT;
			}
        } else {
            switch (rc) {
            case ERR_BAD_DATA:
            case ERR_INVAL:
                rsp->code = COAP_RSP_406_NOT_ACCEPTABLE;
                break;
            default:
                rsp->code = COAP_RSP_500_INTERNAL_ERROR;
                break;
            }
            goto err;
        }
    } // if GET
    
    /*
     * DELETE for disabling sensors.
     */
    /* DELETE /temp?all */
    else if (req->code == COAP_REQUEST_DELETE) 
    {
        if (!coap_opt_strcmp(o, "all"))
        {
            if (arduino_disab_bme280())
            {
                rsp->code = COAP_RSP_500_INTERNAL_ERROR;
                goto err;
            }
        } else {
            rsp->code = COAP_RSP_405_METHOD_NOT_ALLOWED;
            goto err;
        }
        rsp->code = COAP_RSP_202_DELETED;
        rsp->plen = 0;
    
    } // if DELETE 
    else 
    {
        /* no other operation is supported */
        rsp->code = COAP_RSP_405_METHOD_NOT_ALLOWED;
        goto err;
        
    } // Unknown operation

    return ERR_OK;

err:
    rsp->plen = 0;

    return ERR_OK;
    
} // crtemperature


error_t arduino_bme280_sensor_init()
{	

    if (! bme.begin()) {
        SerialUSB.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
    SerialUSB.println("BME280 Temperature/Humidity/Pressure Sensor Example");

    // weather monitoring
    SerialUSB.println("-- Weather Station Scenario --");
    SerialUSB.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
    SerialUSB.println("filter off");
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );

    // Only needed in forced mode! In normal mode, you can remove the next line.
    bme.takeForcedMeasurement(); // has no effect in normal mode

    // Scale of temperature measurement
    bme280_ctx.bme280_temp_scale = BME280_CELSIUS_SCALE;
    bme280_ctx.bme280_altitude_scale = BME280_METER_SCALE;

	// Enable bme280 sensor
	arduino_enab_bme280();

	return ERR_OK;
	
} // arduino_bme280_sensor_init()


/*
 * Enable bme280 sensor.
 */
error_t arduino_enab_bme280()
{
    error_t rc;
    rc = bme280_sensor_enable();
    if (!rc) {
        coap_stats.sensors_enabled++;
    }
    return rc;
}


/*
 * Disable bme280 sensor.
 */
error_t arduino_disab_bme280()
{
    error_t rc;
    rc = bme280_sensor_disable();
    if (!rc) {
        coap_stats.sensors_disabled++;
    }
    return rc;
}


/* @brief
 * arduino_put_bme280_temp_cfg()
 *
 * Set bme280 temperature sensor scale
 */
error_t arduino_put_bme280_temp_cfg( bme280_temp_scale_t scale )
{
	switch(scale)
	{
	case BME280_CELSIUS_SCALE:
		bme280_ctx.bme280_temp_scale = BME280_CELSIUS_SCALE;
        SerialUSB.println("Celsius scale set!");
		break;

	case BME280_FAHRENHEIT_SCALE:
		bme280_ctx.bme280_temp_scale = BME280_FAHRENHEIT_SCALE;
        SerialUSB.println("Fahrenheit scale set!");
		break;

	default:
		return ERR_INVAL;
		
	} // switch

	return ERR_OK;

} // arduino_put_bme280_temp_cfg()

/*
 * arduino_get_bme280_temp_cfg()
 *
 * Get bme280 temperature sensor scale
 */
error_t arduino_get_bme280_temp_cfg( struct mbuf *m, uint8_t *len )
{
	uint32_t count = 0;
	char unit[2] = "F"; // Default scale Fahrenheit, NULL terminated string

	// Check scale
	if ( bme280_ctx.bme280_temp_scale == BME280_CELSIUS_SCALE )
	{
		// Change return unit to Celsius
		strcpy( unit, "C" );
		
	} // if

	// Assemble response
	rsp_msg( m, len, count, NULL, unit );

	return ERR_OK;
} // arduino_get_bme280_temp_cfg


/* @brief
 * arduino_put_bme280_altitude_cfg()
 *
 * Set bme280 altitude sensor scale
 */
error_t arduino_put_bme280_altitude_cfg( bme280_altitude_scale_t scale )
{
    switch(scale)
    {
    case BME280_METER_SCALE:
        bme280_ctx.bme280_altitude_scale = BME280_METER_SCALE;
        SerialUSB.println("Altitude Meter Scale Set!");
        break;

    case BME280_FAHRENHEIT_SCALE:
        bme280_ctx.bme280_altitude_scale = BME280_FEET_SCALE;
        SerialUSB.println("Altitude Feet Scale Set!");
        break;

    default:
        return ERR_INVAL;

    } // switch

    return ERR_OK;

} // arduino_put_bme280_altitude_cfg()

/*
 * arduino_put_bme280_altitude_cfg()
 *
 * Get bme280 altitude sensor scale
 */
error_t arduino_get_bme280_altitude_cfg( struct mbuf *m, uint8_t *len )
{
    uint32_t count = 0;
    char unit[6] = " Feet"; // Default scale Feet, NULL terminated string

    // Check scale
    if ( bme280_ctx.bme280_altitude_scale == BME280_METER_SCALE )
    {
        // Change return unit to Meter
        strcpy( unit, " Meter" );
    } // if

    // Assemble response
    rsp_msg( m, len, count, NULL, unit );

    return ERR_OK;
} // arduino_put_bme280_altitude_cfg


/**
 *
 * @brief Read bme280 sensor temperature and return a message and length of message
 *
 *
 */
error_t arduino_get_bme280_temp( struct mbuf *m, uint8_t *len )
{
    const uint32_t count = 1;
    float reading = 0.0;
    char unit[3] = " F"; // Default scale Fahrenheit, NULL terminated string
    error_t rc;

    if ( !bme280_ctx.enable )
    {
        SerialUSB.println("BME280 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }

    /* Read temp, already in network order. */
    rc = bme280_sensor_read_temp(&reading);

    // Check scale
    if ( bme280_ctx.bme280_temp_scale == BME280_CELSIUS_SCALE )
    {
        // Change return unit to Celsius
        strcpy( unit, " C" );
    } // if

    // Assemble response
    rc = rsp_msg( m, len, count, &reading, unit );

    // Return code
    return rc;
}


/**
 *
 * @brief Read bme280 sensor pressure and return a message and length of message
 *
 *
 */
error_t arduino_get_bme280_pressure( struct mbuf *m, uint8_t *len )
{
    const uint32_t count = 1;
    float reading = 0.0;
    char unit[5] = " hPa"; // Default scale hPa, NULL terminated string
    error_t rc;

    if ( !bme280_ctx.enable )
    {
        SerialUSB.println("BME280 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }

    /* Read pressure, already in network order. */
    rc = bme280_sensor_read_pressure(&reading);

    // Assemble response
    rc = rsp_msg( m, len, count, &reading, unit );

    // Return code
    return rc;
}


/**
 *
 * @brief Read bme280 sensor altitude and return a message and length of message
 *
 *
 */
error_t arduino_get_bme280_altitude( struct mbuf *m, uint8_t *len )
{
    const uint32_t count = 1;
    float reading = 0.0;
    char unit[7] = " Meter"; // Default scale Meter, NULL terminated string
    error_t rc;

    if ( !bme280_ctx.enable )
    {
        SerialUSB.println("BME280 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }

    /* Read altitude, already in network order. */
    rc = bme280_sensor_read_altitude(&reading);

    // Check scale
    if ( bme280_ctx.bme280_altitude_scale == BME280_FEET_SCALE )
    {
        // Change return unit to Feet
        strcpy( unit, " Feet" );
    } // if

    // Assemble response
    rc = rsp_msg( m, len, count, &reading, unit );

    // Return code
    return rc;
}


/**
 *
 * @brief Read bme280 sensor humidity and return a message and length of message
 *
 *
 */
error_t arduino_get_bme280_humidity( struct mbuf *m, uint8_t *len )
{
    const uint32_t count = 1;
    float reading = 0.0;
    char unit[3] = " %"; // Default scale %, NULL terminated string
    error_t rc;

    if ( !bme280_ctx.enable )
    {
        SerialUSB.println("BME280 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }

    /* Read humidity, already in network order. */
    rc = bme280_sensor_read_humidity(&reading);

    // Assemble response
    rc = rsp_msg( m, len, count, &reading, unit );

    // Return code
    return rc;
}



/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

error_t bme280_sensor_read_temp(float * p)
{
    error_t rc = ERR_OK;
	float re = BME280_INVALID_TEMP;

    if ( !bme280_ctx.enable )
    {
		SerialUSB.println("BME280 Sensor Disabled!");
        return ERR_OP_NOT_SUPP;
    }

	// Get bme280 temperature value and print its value.
	re = bme.readTemperature();
	if (bme280_ctx.bme280_temp_scale == BME280_FAHRENHEIT_SCALE )
	{
		// Convert from Celsius to Fahrenheit
		re *= 1.8;
		re += 32;
	}

	rc = ERR_OK;

	// Assign output
	*p = re;
    return rc;
}


error_t bme280_sensor_read_pressure(float * p)
{
    error_t rc = ERR_OK;
    float re = BME280_INVALID_PRESSURE;

    if ( !bme280_ctx.enable )
    {
        SerialUSB.println("BME280 Sensor Disabled!");
        return ERR_OP_NOT_SUPP;
    }

    // Get bme280 pressure value and print its value.
    re = bme.readPressure() / 100.0F;
    
    // Assign output
    *p = re;
    return rc;
}


error_t bme280_sensor_read_altitude(float * p)
{
    error_t rc = ERR_OK;
    float re = BME280_INVALID_ALTITUDE;

    if ( !bme280_ctx.enable )
    {
        SerialUSB.println("BME280 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }

    // Get bme280 altitude value and print its value.
    re = bme.readAltitude(SEALEVELPRESSURE_HPA);
    if ( bme280_ctx.bme280_altitude_scale == BME280_FEET_SCALE )
    {
        // Convert from Meters to Feet
        re *= 3.28084;
    }

    rc = ERR_OK;

    // Assign output
    *p = re;
    return rc;
}


error_t bme280_sensor_read_humidity(float * p)
{
    error_t rc = ERR_OK;
    float re = BME280_INVALID_HUMIDITY;

    if ( !bme280_ctx.enable )
    {
        SerialUSB.println("BME280 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }

    // Get bme280 humidity value and print its value.
    re = bme.readHumidity();
    rc = ERR_OK;

    // Assign output
    *p = re;
    return rc;
}




error_t bme280_sensor_enable(void)
{
    bme280_ctx.enable = 1;
    return ERR_OK;
}

error_t bme280_sensor_disable(void)
{
    bme280_ctx.enable = 0;
    return ERR_OK;
}
