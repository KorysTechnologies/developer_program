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



The circuit configuration - ClosedCube OPT3001-PRO
  - GND pin: ground (Milli5 Arduino Shiled board - J15 pin6 )
  - VDD pin: 3.3V (Milli5 Arduino Shiled board - J15 pin4 )
  - SDA pin: 9 (Milli5 Arduino Shiled board - J6 pin9 - I2C_SDA M0 Pro)
  - SCL pin: 10 (Milli5 Arduino Shiled board - J6 pin10 - I2C_SCL M0 Pro )
  - INT pin: not connected


To set the als sensor configuration:

Set ALS Conversion time
PUT /sensor/arduino/als?cfg_conversiontime=0    100ms
PUT /sensor/arduino/als?cfg_conversiontime=1    800ms

Set ALS Conversion mode
PUT /sensor/arduino/als?cfg_conversionmode=0    Shutdown
PUT /sensor/arduino/als?cfg_conversionmode=1    Singleshot
PUT /sensor/arduino/als?cfg_conversionmode=3    Continuous


To get the  als sensor configuration:

Get ALS Conversion time
GET /sensor/arduino/als?cfg_conversiontime

Get ALS Conversion mode
GET /sensor/arduino/als?cfg_conversionmode


To read the als sensor value:
GET /sensor/arduino/als?sens


To request an observe on the als sensor value:
GET /sensor/arduino/als?sens with the CoAP observe header set

DELETE for disabling als sensor
DELETE /sensor/arduino/als?all

*/

#include <string.h>
#include <Arduino.h>

#include <Wire.h>
#include <ClosedCube_OPT3001.h>

#include "log.h"
#include "bufutil.h"
#include "exp_coap.h"
#include "coap_rsp_msg.h"
#include "coappdu.h"
#include "coapobserve.h"
#include "coapsensorobs.h"
#include "alsopt3001_sensor.h"
#include "arduino_pins.h"

#define OPT3001_ADDRESS 0x45
#define UNIT " lux"


als_ctx_t als_ctx;
ClosedCube_OPT3001 opt3001;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/

/*
 * crals
 *
 * @brief CoAP Resource ambient light sensor(als) sensor
 *
 */
error_t crals(struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it)
{
    struct optlv *o;
	uint8_t obs = false;


    /* No URI path beyond /als is supported, so reject if present. */
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

        /* PUT /als?cfg_conversiontime=<100ms|800ms> */
        if (!coap_opt_strcmp(o, "cfg_conversiontime=100ms"))
        {
		    arduino_put_als_cfg_conversiontime(ALS_CONVTIME_100MS);
        }
        else if (!coap_opt_strcmp(o, "cfg_conversiontime=800ms"))
        {
			arduino_put_als_cfg_conversiontime(ALS_CONVTIME_800MS);
        }
        /* PUT /als?cfg_conversionmode=<Shutdown|Singleshot|Continuous> */
        else if (!coap_opt_strcmp(o, "cfg_conversionmode=Shutdown"))
        {
            arduino_put_als_cfg_conversionmode(ALS_SHUTDOWN);
        }
        else if (!coap_opt_strcmp(o, "cfg_conversionmode=Singleshot"))
        {
            arduino_put_als_cfg_conversionmode(ALS_SINGLESHOT);
        }
        else if (!coap_opt_strcmp(o, "cfg_conversionmode=Continuous"))
        {
            arduino_put_als_cfg_conversionmode(ALS_CONTINUOUS2);
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
        /* GET /als?cfg_conversiontime */
        if (!coap_opt_strcmp(o, "cfg_conversiontime"))
        {
            /* get als config */
            rc = arduino_get_als_cfg_conversiontime( rsp->msg, &len );
        }
        /* GET /als?cfg_conversionmode */
        else if (!coap_opt_strcmp(o, "cfg_conversionmode"))
        {
            /* get als config */
            rc = arduino_get_als_cfg_conversionmode( rsp->msg, &len );
        }
        /* GET /als?sens */
        else if (!coap_opt_strcmp(o, "sens"))
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
				/* Get sensor value */
				rc = arduino_get_als( rsp->msg, &len );
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
    /* DELETE /als?all */
    else if (req->code == COAP_REQUEST_DELETE)
    {
        if (!coap_opt_strcmp(o, "all"))
        {
            if (arduino_disab_als())
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

} // crals

/*
 * Initialize als sensor.
 */
error_t arduino_als_sensor_init()
{

    OPT3001 result;

    // Initialize opt3001 ambient light sensor sensor
    opt3001.begin(OPT3001_ADDRESS);

    // Enable temp sensor
    arduino_enab_als();

    SerialUSB.println("OPT3001 Ambient Light Sensor Example");

    // Print als sensor details
    SerialUSB.println("------------------------------------");
    SerialUSB.print  ("OPT3001 Manufacturer ID:   "); SerialUSB.print(opt3001.readManufacturerID(), HEX);    SerialUSB.println("h");
    SerialUSB.print  ("OPT3001 Device ID:    "); SerialUSB.print(opt3001.readDeviceID(), HEX);  SerialUSB.println("h");

    configureSensor();

    printResult("High-Limit", opt3001.readHighLimit());
    printResult("Low-Limit", opt3001.readLowLimit());
    SerialUSB.println("------------------------------------");

    result = opt3001.readResult();
    SerialUSB.println("------------------------------------");
    printResult("OPT3001 ALS Sensor Measurement", result);

	return ERR_OK;

} // arduino_als_sensor_init()



/*
 * Enable als sensor.
 */
error_t arduino_enab_als()
{
    error_t rc;
    rc = als_sensor_enable();
    if (!rc) {
        coap_stats.sensors_enabled++;
    }
    return rc;
}


/*
 * Disable als sensor.
 */
error_t arduino_disab_als()
{
    error_t rc;
    rc = als_sensor_disable();
    if (!rc) {
        coap_stats.sensors_disabled++;
    }
    return rc;
}

/**
 *
 * @brief Read als sensor value and return a message and length of message
 *
 *
 */
error_t arduino_get_als( struct mbuf *m, uint8_t *len )
{
	const uint32_t count = 1;
	float reading = 0.0;
    error_t rc;

	// Check if disabled
    if (!als_ctx.enable)
	{
		return ERR_OP_NOT_SUPP;

	} // if */

	/* Read als sensor value, already in network order. */
    rc = als_sensor_read(&reading);

	// Assemble response
	rc = rsp_msg( m, len, count, &reading, "lux" );

	// Return code
    return rc;
}


/* @brief
 * arduino_put_als_cfg_conversiontime()
 *
 * Set als conversion time config to 100ms or 800ms
 */
error_t arduino_put_als_cfg_conversiontime( als_sensor_conversiontime_t convtime )
{
	switch(convtime)
	{
	case ALS_CONVTIME_100MS:
		als_ctx.cfg.als_conversion_time = ALS_CONVTIME_100MS;
        println("Conversion time 100ms set!");
		break;

	case ALS_CONVTIME_800MS:
		als_ctx.cfg.als_conversion_time = ALS_CONVTIME_800MS;
        println("Conversion time 800ms set!");
		break;

	default:
		return ERR_INVAL;

	} // switch

    //configure als new conversion time
    newconfigureSensor();

	// Enable
	arduino_enab_als();
	return ERR_OK;

} // arduino_put_als_cfg_conversiontime()

/* @brief
 * arduino_get_als_cfg_conversiontime()
 *
 * Get als convertion time interval
 */
error_t arduino_get_als_cfg_conversiontime( struct mbuf *m, uint8_t *len )
{
	uint32_t count = 0;
	char unit[6] = "100ms";
	// Check conversion time
	if ( als_ctx.cfg.als_conversion_time == ALS_CONVTIME_100MS )
	{
		// Conversion time 100ms
		strcpy( unit, "100ms" );

	} // if
    else if ( als_ctx.cfg.als_conversion_time == ALS_CONVTIME_800MS )
    {
        // Conversion time 800ms
        strcpy( unit, "800ms" );

    } // if

	// Assemble response
	rsp_msg( m, len, count, NULL, unit );

	return ERR_OK;
}   //arduino_get_als_cfg_conversiontime






/* @brief
 * arduino_put_als_cfg_conversionmode()
 *
 * Set als conversion mode config to Shutdown / Single-shot or Continuous
 */
error_t arduino_put_als_cfg_conversionmode( als_sensor_conversionmode_t convmode )
{
    switch(convmode)
    {
    case ALS_SHUTDOWN:
        als_ctx.cfg.als_conversion_mode = ALS_SHUTDOWN;
        SerialUSB.println("PUT Conversion Mode: Shutdown");
        break;

    case ALS_SINGLESHOT:
        als_ctx.cfg.als_conversion_mode = ALS_SINGLESHOT;
        SerialUSB.println("PUT Conversion Mode: Single-shot");
        break;

    case ALS_CONTINUOUS1:
        als_ctx.cfg.als_conversion_mode = ALS_CONTINUOUS1;
        SerialUSB.println("PUT Conversion Mode: Continuous");
        break;

    case ALS_CONTINUOUS2:
        als_ctx.cfg.als_conversion_mode = ALS_CONTINUOUS2;
        SerialUSB.println("PUT Conversion Mode: Continuous");
        break;

    default:
        return ERR_INVAL;

    } // switch

    //configure als new conversion mode
    newconfigureSensor();
    // Enable
    arduino_enab_als();
    return ERR_OK;

}   // arduino_put_als_cfg_conversionmode()

/* @brief
 * arduino_get_als_cfg_conversionmode())
 *
 * Get als conversion mode
 */
error_t arduino_get_als_cfg_conversionmode( struct mbuf *m, uint8_t *len )
{
    uint32_t count = 0;
    char unit[17] = "";

    // Check conversion mode
    switch(als_ctx.cfg.als_conversion_mode)
    {
    case ALS_SHUTDOWN:
         // Change return conversionmode
        strcpy( unit, "Shutdown mode" );
        break;

    case ALS_SINGLESHOT:
        // Change return conversionmode
        strcpy( unit, "Single-shot mode" );
        break;

    case ALS_CONTINUOUS1:
        // Change return conversionmode
        strcpy( unit, "Continuous mode" );
        break;

    case ALS_CONTINUOUS2:
        // Change return conversionmode
        strcpy( unit, "Continuous mode" );
        break;

    default:
        return ERR_INVAL;

    } // switch

    // Assemble response
    rsp_msg( m, len, count, NULL, unit);

    return ERR_OK;
}   //arduino_get_als_cfg_conversionmode


/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

/*
 * configureSensor
 *
* @brief
 *
 */
void configureSensor() {
    OPT3001_Config newConfig;

    newConfig.RangeNumber = B1100;  //  Automatic Full-scale
    newConfig.ConvertionTime = B0; //B0 - 100ms, B1 - 800ms
    newConfig.Latch = B1;      //
    newConfig.ModeOfConversionOperation = B11;  //00 = Shutdown (default), 01 = Single-shot, 11 = Continuous conversions

    OPT3001_ErrorCode errorConfig = opt3001.writeConfig(newConfig);
    if (errorConfig != NO_ERROR)
        printError("OPT3001 configuration", errorConfig);
    else {
        OPT3001_Config sensorConfig = opt3001.readConfig();
        SerialUSB.println("OPT3001 Current Config:");
        SerialUSB.println("------------------------------");

        SerialUSB.print("Conversion ready (R):");
        SerialUSB.print(sensorConfig.ConversionReady, HEX);   SerialUSB.println("h");

        SerialUSB.print("Conversion time (R/W):");
        SerialUSB.print(sensorConfig.ConvertionTime, HEX);    SerialUSB.println("h");

        SerialUSB.print("Fault count field (R/W):");
        SerialUSB.print(sensorConfig.FaultCount, HEX);    SerialUSB.println("h");

        SerialUSB.print("Flag high field (R-only):");
        SerialUSB.print(sensorConfig.FlagHigh, HEX);  SerialUSB.println("h");

        SerialUSB.print("Flag low field (R-only):");
        SerialUSB.print(sensorConfig.FlagLow, HEX);   SerialUSB.println("h");

        SerialUSB.print("Latch field (R/W):");
        SerialUSB.print(sensorConfig.Latch, HEX); SerialUSB.println("h");

        SerialUSB.print("Mask exponent field (R/W):");
        SerialUSB.print(sensorConfig.MaskExponent, HEX);  SerialUSB.println("h");

        SerialUSB.print("Mode of conversion operation (R/W):");
        SerialUSB.print(sensorConfig.ModeOfConversionOperation, HEX); SerialUSB.println("h");

        SerialUSB.print("Polarity field (R/W):");
        SerialUSB.print(sensorConfig.Polarity, HEX);  SerialUSB.println("h");

        SerialUSB.print("Overflow flag (R-only):");
        SerialUSB.print(sensorConfig.OverflowFlag, HEX);  SerialUSB.println("h");

        SerialUSB.print("Range number (R/W):");
        SerialUSB.print(sensorConfig.RangeNumber, HEX);   SerialUSB.println("h");

        SerialUSB.println("------------------------------");
    }


    als_ctx.cfg.als_range_number = 0xC;
    als_ctx.cfg.als_conversion_time = ALS_CONVTIME_100MS;
    als_ctx.cfg.als_latch = 0x1;
    als_ctx.cfg.als_conversion_mode = ALS_CONTINUOUS2;

}   //configureSensor

/*
 * newconfigureSensor
 *
* @brief
 *
 */
void newconfigureSensor() {
    OPT3001_Config newConfig;

    newConfig.RangeNumber = B1100;  //  Automatic Full-scale
    newConfig.ConvertionTime = als_ctx.cfg.als_conversion_time; //B0 - 100ms, B1 - 800ms
    newConfig.Latch = B1;      //
    newConfig.ModeOfConversionOperation = als_ctx.cfg.als_conversion_mode;  //00 = Shutdown (default), 01 = Single-shot, 11 = Continuous conversions

    OPT3001_ErrorCode errorConfig = opt3001.writeConfig(newConfig);
    if (errorConfig != NO_ERROR)
        printError("OPT3001 configuration", errorConfig);
    else {
        OPT3001_Config sensorConfig = opt3001.readConfig();
        SerialUSB.println("OPT3001 New Config:");
        SerialUSB.println("------------------------------");

        SerialUSB.print("Conversion ready (R):");
        SerialUSB.print(sensorConfig.ConversionReady, HEX);   SerialUSB.println("h");

        SerialUSB.print("Conversion time (R/W):");
        SerialUSB.print(sensorConfig.ConvertionTime, HEX);    SerialUSB.println("h");

        SerialUSB.print("Fault count field (R/W):");
        SerialUSB.print(sensorConfig.FaultCount, HEX);    SerialUSB.println("h");

        SerialUSB.print("Flag high field (R-only):");
        SerialUSB.print(sensorConfig.FlagHigh, HEX);  SerialUSB.println("h");

        SerialUSB.print("Flag low field (R-only):");
        SerialUSB.print(sensorConfig.FlagLow, HEX);   SerialUSB.println("h");

        SerialUSB.print("Latch field (R/W):");
        SerialUSB.print(sensorConfig.Latch, HEX); SerialUSB.println("h");

        SerialUSB.print("Mask exponent field (R/W):");
        SerialUSB.print(sensorConfig.MaskExponent, HEX);  SerialUSB.println("h");

        SerialUSB.print("Mode of conversion operation (R/W):");
        SerialUSB.print(sensorConfig.ModeOfConversionOperation, HEX); SerialUSB.println("h");

        SerialUSB.print("Polarity field (R/W):");
        SerialUSB.print(sensorConfig.Polarity, HEX);  SerialUSB.println("h");

        SerialUSB.print("Overflow flag (R-only):");
        SerialUSB.print(sensorConfig.OverflowFlag, HEX);  SerialUSB.println("h");

        SerialUSB.print("Range number (R/W):");
        SerialUSB.print(sensorConfig.RangeNumber, HEX);   SerialUSB.println("h");

        SerialUSB.println("------------------------------");
    }

}   //newconfigureSensor

/*
 * printResult
 *
* @brief
 *
 */
void printResult(String text, OPT3001 result) {
    if (result.error == NO_ERROR) {
        SerialUSB.print(text);
        SerialUSB.print(": ");
        SerialUSB.print(result.lux);
        SerialUSB.println(" lux");
    }
    else {
        printError(text,result.error);
    }
}   //printResult

/*
 * printError
 *
* @brief
 *
 */
void printError(String text, OPT3001_ErrorCode error) {
    SerialUSB.print(text);
    SerialUSB.print(": [ERROR] Code #");
    SerialUSB.println(error);
} //printError


/*
 * als_sensor_read
 *
* @brief
 *
 */
error_t als_sensor_read(float * p)
{
    error_t rc = ERR_OK;
    OPT3001 result;

    if (!als_ctx.enable)
    {
        println("Als sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }

    result = opt3001.readResult();
    delay(500);

    // Assign output
    *p = result.lux;
    return rc;
}   //als_sensor_read

/*
 * als_sensor_enable
 *
* @brief
 *
 */
error_t als_sensor_enable(void)
{
    als_ctx.enable = 1;
    return ERR_OK;
}   //als_sensor_enable

/*
 * als_sensor_disable
 *
* @brief
 *
 */
error_t als_sensor_disable(void)
{
    als_ctx.enable = 0;
    return ERR_OK;
}   //als_sensor_disable
