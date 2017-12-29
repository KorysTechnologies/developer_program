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



The circuit configuration - HX711 ADC Module + 5kg Load Cell
  - GND pin: ground (Milli5 Arduino Shield board - J15 pin6 )
  - VDD pin: 5V (Milli5 Arduino Shield board - J15 pin5 )

  - DOUT pin: 4 (Milli5 Arduino Shield board - J5 pin5 )
  - SCK pin: 5 (Milli5 Arduino Shield board - J5 pin6 )

To set the weight sensor configuration:

no PUT implemented

To Get weight sensor calibration factor or scale unit
GET /sensor/arduino/weight?calib_f
GET /sensor/arduino/weight?cfg_scale


To read the weight sensor value:
GET /sensor/arduino/weight?sens

To request an observe on the weight sensor value:
GET /sensor/arduino/weight?sens with the CoAP observe header set

DELETE for disabling weight sensor
DELETE /sensor/arduino/weight?all

*/

#include <string.h>
#include <Arduino.h>

#include <HX711.h>

#include "log.h"
#include "bufutil.h"
#include "exp_coap.h"
#include "coap_rsp_msg.h"
#include "coappdu.h"
#include "coapobserve.h"
#include "coapsensorobs.h"
#include "weight_sensor.h"
#include "arduino_pins.h"

// HX711.DOUT - pin 4
// HX711.PD_SCK  - pin 5
// HX711 gain factor 128
HX711 HX711(4, 5, 128);

weight_ctx_t weight_ctx;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/

/*
 * crweight
 *
 * @brief CoAP Resource weight sensor
 *
 */
error_t crweight(struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it)
{
    struct optlv *o;
	uint8_t obs = false;


    /* No URI path beyond /weight is supported, so reject if present. */
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

        /* PUT /weight?cfg_scale=<kg|lbs> */
        if (!coap_opt_strcmp(o, "cfg_scale=kg"))
        {
            arduino_put_weight_cfg_scale(SCALE_KILOGRAMS);
        }
        else if (!coap_opt_strcmp(o, "cfg_scale=lbs"))
        {
            arduino_put_weight_cfg_scale(SCALE_LBS);
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

        /* Config or get sensor values. */
        /* GET /weight?calib_factor */
        if (!coap_opt_strcmp(o, "calib_f"))
        {
            /* get weight sensor calibration factor */
            rc = arduino_get_weight_calib_factor( rsp->msg, &len );
        }
        /* GET /weight?cfg_scale */
        if (!coap_opt_strcmp(o, "cfg_scale"))
        {
            /* get weight sensor scale units */
            rc = arduino_get_weight_cfg_scale( rsp->msg, &len );
        }
        /* GET /weight?sens */
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
				rc = arduino_get_weight( rsp->msg, &len );
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
    /* DELETE /weight?all */
    else if (req->code == COAP_REQUEST_DELETE)
    {
        if (!coap_opt_strcmp(o, "all"))
        {
            if (arduino_disab_weight())
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

} // crweight

/*
 * Initialize weight sensor.
 */
error_t arduino_weight_sensor_init()
{

    SerialUSB.println("Initializing the scale");
    // value obtained by calibrating the scale with known weights; see the README for details from HX711 Library
    HX711.set_scale(2280.f);
    // reset the scale to 0
    HX711.tare();
    weight_ctx.calibrating_factor = 2280;
    weight_ctx.weight_scale = SCALE_KILOGRAMS;

    // Enable HX711 24-Bit Analog-to-Digital Converter (ADC) for Weight Scales
    arduino_enab_weight();

    SerialUSB.println("Weight Sensor(HX711 24bitADC + Cell Loads) Example");
    SerialUSB.println("------------------------------------");

    SerialUSB.print("Scale Measurement: \t\t");
    // print the average of 5 readings from the ADC minus tare weight, divided
    // by the SCALE parameter set with set_scale
    SerialUSB.println(HX711.get_units(5), 1);
    SerialUSB.println("------------------------------------");

	return ERR_OK;

} // arduino_weight_sensor_init()



/*
 * Enable weight sensor.
 */
error_t arduino_enab_weight()
{
    error_t rc;
    rc = weight_sensor_enable();
    if (!rc) {
        coap_stats.sensors_enabled++;
    }
    return rc;
}


/*
 * Disable weight sensor.
 */
error_t arduino_disab_weight()
{
    error_t rc;
    rc = weight_sensor_disable();
    if (!rc) {
        coap_stats.sensors_disabled++;
    }
    return rc;
}

/**
 *
 * @brief Read weight sensor value and return a message and length of message
 *
 *
 */
error_t arduino_get_weight(struct mbuf *m, uint8_t *len)
{
	const uint32_t count = 1;
	float reading = 0.0;
    char scale_unit[4] = " kg";  // Default scale Kg, NULL terminated string
    error_t rc;

	// Check if disabled
    if (!weight_ctx.enable)
	{
		return ERR_OP_NOT_SUPP;

	} // if */

    // get the ADC out of sleep mode
    HX711.power_up();

	// average of 10 ADC readings minus tare weight, divided by the SCALE parameter set with set_scale function
    reading = HX711.get_units(10);

    // check scale measurement units
    if(weight_ctx.weight_scale == SCALE_LBS )
    {
        // Change return unit to lbs
        strcpy( scale_unit, " lbs" );
    } // if

	// Assemble response
	rc = rsp_msg( m, len, count, &reading, scale_unit );

	// Return code
    return rc;
}

/* @brief
 * arduino_get_weight_calib_factor()
 *
 * Get weight calibration factor
 */
error_t arduino_get_weight_calib_factor(struct mbuf *m, uint8_t *len)
{
    uint32_t count = 1;
    char buf[1] = "";
    float calib_factor = 0;

    calib_factor = (float) weight_ctx.calibrating_factor;
    // Assemble response
    rsp_msg( m, len, count, &calib_factor, buf );

    return ERR_OK;
}   //arduino_get_weight_calib_factor


/* @brief
 * arduino_put_weight_cfg_scale()
 *
 * Set weight sensor scale unit config
 */
error_t arduino_put_weight_cfg_scale(weight_scale_t weight_scale)
{
    switch(weight_scale)
    {
    case SCALE_KILOGRAMS:
        weight_ctx.weight_scale = SCALE_KILOGRAMS;
        SerialUSB.println("Kilograms scale set!");
        break;

    case SCALE_LBS:
        weight_ctx.weight_scale = SCALE_LBS;
        SerialUSB.println("Lbs scale set!");
        break;

    default:
        return ERR_INVAL;
    } // switch

    return ERR_OK;

} // arduino_put_weight_cfg_scale()

/* @brief
 * arduino_get_weight_cfg_scale()
 *
 * Get weight scale units
 */
error_t arduino_get_weight_cfg_scale(struct mbuf *m, uint8_t *len)
{
	uint32_t count = 0;
	char buf[11] = "";
	// Check weight scale
	if ( weight_ctx.weight_scale == SCALE_KILOGRAMS )
	{
		strcpy( buf, " Kilograms" );

	} // if
    else if ( weight_ctx.weight_scale == SCALE_LBS )
    {
        strcpy( buf, " Lbs" );

    } // if

	// Assemble response
	rsp_msg( m, len, count, NULL, buf );

	return ERR_OK;
}   //arduino_get_weight_cfg_scale


/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/
/*
 * weight_sensor_read
 *
* @brief
 *
 */
error_t weight_sensor_read(float * p)
{
    error_t rc = ERR_OK;
    float result;

    if (!weight_ctx.enable)
    {
        SerialUSB.println("Weight sensor disabled!");
        return ERR_OP_NOT_SUPP;
    }
    result = HX711.get_units(10);

    HX711.power_down();         // put the ADC in sleep mode

    // Assign output
    *p = result;
    return rc;
}   //weight_sensor_read

/*
 * weight_sensor_enable
 *
* @brief
 *
 */
error_t weight_sensor_enable(void)
{
    weight_ctx.enable = 1;
    return ERR_OK;
}   //weight_sensor_enable

/*
 * weight_sensor_disable
 *
* @brief
 *
 */
error_t weight_sensor_disable(void)
{
    weight_ctx.enable = 0;
    return ERR_OK;
}   //weight_sensor_disable
