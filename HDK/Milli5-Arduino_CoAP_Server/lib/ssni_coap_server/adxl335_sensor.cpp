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


The circuit configuration - ADXL355 Module GY-61
  - GND pin: ground (Milli5 Arduino Shield board - J15 pin6 )
  - VDD pin: 5V (Milli5 Arduino Shield board - J15 pin5 )
  - Connect 3.3V to AREF
  - X-OUT pin: A3 (Milli5 Arduino Shield board - A3 - J8 pin4)
  - Y-OUT pin: A4 (Milli5 Arduino Shield board - A4 - J8 pin5)
  - Z-OUT pin: A5 (Milli5 Arduino Shield board - A5 - J8 pin6)


To set the adxl335 sensor configuration:
Set adxl335 measurement config
PUT /sensor/arduino/adxl335?cfg=gforce      //return G for x, y, z
PUT /sensor/arduino/adxl335?cfg=rotation    //return rotation in degrees for x, y, z
PUT /sensor/arduino/adxl335?cfg=rawdata     //return analog value without any processing for x, y, z

To get the adxl335 sensor measurement configuration:
GET /sensor/arduino/adxl335?cfg

To read the adxl335 sensor values:
GET /sensor/arduino/adxl335?sens_x
GET /sensor/arduino/adxl335?sens_y
GET /sensor/arduino/adxl335?sens_z

To request an observe on the adxl335 sensor value:
GET /sensor/arduino/adxl335?sens_x with the CoAP observe header set
GET /sensor/arduino/adxl335?sens_y with the CoAP observe header set
GET /sensor/arduino/adxl335?sens_z with the CoAP observe header set


DELETE for disabling adxl335 sensor
DELETE /sensor/arduino/adxl335?all


*/

#include <string.h>

#include "log.h"
#include "bufutil.h"
#include "exp_coap.h"
#include "coap_rsp_msg.h"
#include "coappdu.h"
#include "coapobserve.h"
#include "coapsensorobs.h"
#include "adxl335_sensor.h"
#include "arduino_pins.h"
#include "wiring_analog.h"
#include "WMath.h"


/* PIN DECLARATION */
const int adxl335_xpin = A3;
const int adxl335_ypin = A4;
const int adxl335_zpin = A5;

// Measurement config
adxl335_ctx_t adxl335_ctx;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/

/*
 * cradxl335
 *
 * @brief CoAP Resource temperature sensor
 *
 */
error_t cradxl335(struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it)
{
    struct optlv *o;
	uint8_t obs = false;


    /* No URI path beyond /adxl335 is supported, so reject if present. */
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

        /* PUT /adxl335?cfg=<gforce|rotation|rawdata> */
        if (!coap_opt_strcmp(o, "cfg=gforce"))
        {
			arduino_put_adxl335_meas_cfg(ADXL335_MEAS_G);
        }
        else if (!coap_opt_strcmp(o, "cfg=rotation"))
        {
            arduino_put_adxl335_meas_cfg(ADXL335_MEAS_ROTATION);
        }
        else if (!coap_opt_strcmp(o, "cfg=rawdata"))
        {
            arduino_put_adxl335_meas_cfg(ADXL335_MEAS_RAWDATA);
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
        /* GET /adxl335?cfg */
        if (!coap_opt_strcmp(o, "cfg"))
        {
            /* get adxl335 measurement config */
            rc = arduino_get_adxl335_meas_cfg( rsp->msg, &len );
        }

        /* GET /adxl335?sens_x */
        else if (!coap_opt_strcmp(o, "sens_x"))
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
				/* Get sensor x-axis value */
				rc = arduino_get_adxl335_xaxis( rsp->msg, &len );
			} // if-else
        }
        /* GET /adxl335?sens_y */
        else if (!coap_opt_strcmp(o, "sens_y"))
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
                /* Get sensor y-axis value */
                rc = arduino_get_adxl335_yaxis( rsp->msg, &len );
            } // if-else
        }
        /* GET /adxl335?sens_z */
        else if (!coap_opt_strcmp(o, "sens_z"))
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
                /* Get sensor z-axis value */
                rc = arduino_get_adxl335_zaxis( rsp->msg, &len );
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
            if (arduino_disab_adxl335())
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


error_t arduino_adxl335_sensor_init()
{
    int xRead = 0;
    int yRead = 0;
    int zRead = 0;

    SerialUSB.println("ADXL335 Accelerometer Example");

    analogReference(AR_EXTERNAL); // use AREF for reference voltage - connect 3.3v to AREF

    //read the analog values from the accelerometer
    xRead = analogRead(adxl335_xpin);
    yRead = analogRead(adxl335_ypin);
    zRead = analogRead(adxl335_zpin);

    // read x, y, z axis - rawdata values
    SerialUSB.println("X-axis | Y-axis  | Z-axis");
    SerialUSB.print(xRead);
    SerialUSB.print("\t");
    SerialUSB.print(yRead);
    SerialUSB.print("\t");
    SerialUSB.print(zRead);

	// Enable adxl335 sensor
	arduino_enab_adxl335();

    //by default adxl measurement data format rawdata
    adxl335_ctx.meas_type = ADXL335_MEAS_RAWDATA;

	return ERR_OK;

} // arduino_adxl335_sensor_init()


/*
 * Enable adxl335 sensor.
 */
error_t arduino_enab_adxl335()
{
    error_t rc;
    rc = adxl335_sensor_enable();
    if (!rc) {
        coap_stats.sensors_enabled++;
    }
    return rc;
}


/*
 * Disable adxl335 sensor.
 */
error_t arduino_disab_adxl335()
{
    error_t rc;
    rc = adxl335_sensor_disable();
    if (!rc) {
        coap_stats.sensors_disabled++;
    }
    return rc;
}


/* @brief
 * arduino_put_adxl335_meas_cfg()
 *
 * Set adxl335 measurement config
 */
error_t arduino_put_adxl335_meas_cfg( adxl335_measurement_type_t meas_type )
{
	switch(meas_type)
	{
	case ADXL335_MEAS_G:
		adxl335_ctx.meas_type = ADXL335_MEAS_G;
        SerialUSB.println("G-force Measurement set!");
		break;

	case ADXL335_MEAS_ROTATION:
		adxl335_ctx.meas_type = ADXL335_MEAS_ROTATION;
        SerialUSB.println("Rotation Measurement set!");
		break;

    case ADXL335_MEAS_RAWDATA:
        adxl335_ctx.meas_type = ADXL335_MEAS_RAWDATA;
        SerialUSB.println("Rawdata Measurement set!");
        break;

	default:
		return ERR_INVAL;
	} // switch

	return ERR_OK;

} // arduino_put_adxl335_meas_cfg()

/*
 * arduino_get_adxl335_meas_cfg()
 *
 * Get adxl335 measurement config
 */
error_t arduino_get_adxl335_meas_cfg( struct mbuf *m, uint8_t *len )
{
	uint32_t count = 0;
	char meas_type[10] = " Rawdata"; // Default rawdata for measurements

	// Check adxl335 measurement config
	if ( adxl335_ctx.meas_type == ADXL335_MEAS_G )
	{
		// Change return meas_type to G-force
		strcpy( meas_type, " G-force" );
	} // if


    // Check adxl335 measurement config
    if ( adxl335_ctx.meas_type == ADXL335_MEAS_ROTATION )
    {
        // Change return meas_type to Rotation
        strcpy( meas_type, " Rotation" );
    } // if

	// Assemble response
	rsp_msg( m, len, count, NULL, meas_type );

	return ERR_OK;
} // arduino_get_adxl335_meas_cfg


/**
 *
 * @brief Read adxl335 sensor x-axis value and return a message and length of message
 *
 *
 */
error_t arduino_get_adxl335_xaxis( struct mbuf *m, uint8_t *len )
{
    const uint32_t count = 1;
    float conv_result = 0.0;
    char unit[10] = " Rawdata"; // Default no unit - rawdata
    error_t rc;

    if ( !adxl335_ctx.enable )
    {
        SerialUSB.println("ADXL335 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }


    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_RAWDATA)
    {
        /* Read adxl335 x axis */
        conv_result = analogRead(adxl335_xpin);
    } // if

    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_G)
    {
        strcpy( unit, " g" );
        float reading = analogRead(adxl335_xpin);

        rc = adxl335_sensor_convert_gforce( reading, &conv_result);
    } // if

    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_ROTATION)
    {
        strcpy( unit, " radians" );
        float yreading = analogRead(adxl335_ypin);
        float zreading = analogRead(adxl335_zpin);
        rc = adxl335_sensor_convert_rotation( yreading, zreading, &conv_result);
    } // if

    // Assemble response
    rc = rsp_msg( m, len, count, &conv_result, unit );

    // Return code
    return rc;
} // arduino_get_adxl335_xaxis


/**
 *
 * @brief Read adxl335 sensor y-axis value and return a message and length of message
 *
 *
 */
error_t arduino_get_adxl335_yaxis( struct mbuf *m, uint8_t *len )
{
    const uint32_t count = 1;
    float conv_result = 0.0;
    char unit[10] = " Rawdata"; // Default no unit - rawdata
    error_t rc;

    if ( !adxl335_ctx.enable )
    {
        SerialUSB.println("ADXL335 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }


    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_RAWDATA)
    {
        /* Read adxl335 y axis */
        conv_result = analogRead(adxl335_ypin);
    } // if

    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_G)
    {
        strcpy( unit, " g" );
        float reading = analogRead(adxl335_ypin);

        rc = adxl335_sensor_convert_gforce( reading, &conv_result);
    } // if

    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_ROTATION)
    {
        strcpy( unit, " radians" );
        float xreading = analogRead(adxl335_xpin);
        float zreading = analogRead(adxl335_zpin);
        rc = adxl335_sensor_convert_rotation( xreading, zreading, &conv_result);
    } // if

    // Assemble response
    rc = rsp_msg( m, len, count, &conv_result, unit );

    // Return code
    return rc;
} // arduino_get_adxl335_yaxis


/**
 *
 * @brief Read adxl335 sensor z-axis and return a message and length of message
 *
 *
 */
error_t arduino_get_adxl335_zaxis( struct mbuf *m, uint8_t *len )
{
    const uint32_t count = 1;
    float conv_result = 0.0;
    char unit[10] = " Rawdata"; // Default no unit - rawdata
    error_t rc;

    if ( !adxl335_ctx.enable )
    {
        SerialUSB.println("ADXL335 sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }


    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_RAWDATA)
    {
        /* Read adxl335 y axis */
        conv_result = analogRead(adxl335_ypin);
    } // if

    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_G)
    {
        strcpy( unit, " g" );
        float reading = analogRead(adxl335_zpin);

        rc = adxl335_sensor_convert_gforce( reading, &conv_result);
    } // if

    // Check measurement config
    if ( adxl335_ctx.meas_type  ==  ADXL335_MEAS_ROTATION)
    {
        strcpy( unit, " radians" );
        float yreading = analogRead(adxl335_ypin);
        float xreading = analogRead(adxl335_xpin);
        rc = adxl335_sensor_convert_rotation( yreading, xreading, &conv_result);
    } // if

    // Assemble response
    rc = rsp_msg( m, len, count, &conv_result, unit );

    // Return code
    return rc;
} // arduino_get_adxl335_zaxis




/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

error_t adxl335_sensor_convert_gforce( float value_raw, float * result )
{
    error_t rc = ERR_OK;
    float re = 0.0;
    float zero_G = 2048.0;
    float scale = 409.5;

    if ( !adxl335_ctx.enable )
    {
		SerialUSB.println("ADXL335 Sensor Disabled!");
        return ERR_OP_NOT_SUPP;
    }

	re = ( value_raw - zero_G ) / scale;
	rc = ERR_OK;

	// Assign output
	*result = re;
    return rc;
}


error_t adxl335_sensor_convert_rotation(double param1, double param2, float * result )
{
    //adxl335 min, max values while standing still
    int minVal = 255;
    int maxVal = 405;

    error_t rc = ERR_OK;
    double angle_radian = 0;

    adxl335_ctx.enable = 0x1;

    if ( !adxl335_ctx.enable )
    {
        SerialUSB.println("ADXL335 Sensor Disabled!");
        return ERR_OP_NOT_SUPP;
    }

    // Convert values to degrees ( -90 to 90 ) - required by atan2
    double param1Ang= map(param1, minVal, maxVal, -90, 90);
    double param2Ang= map(param2, minVal, maxVal, -90, 90);


    //Calculate radian angles using atan2(-yAng, -zAng)
    //atan2 outputs the value of -π to π (radians)
    angle_radian = atan2(-param1Ang, -param2Ang) + PI;

    // Assign output
    *result = angle_radian;
    return rc;
}


error_t adxl335_sensor_enable(void)
{
    adxl335_ctx.enable = 1;

    return ERR_OK;
}


error_t adxl335_sensor_disable(void)
{
    adxl335_ctx.enable = 0;

    return ERR_OK;
}
