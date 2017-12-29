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



The circuit configuration - MQ6
  - GND pin: ground (Milli5 Arduino Shield board - J15 pin6 )
  - VDD pin: 5V (Milli5 Arduino Shield board - J15 pin5 )

  - AO pin: A3 (Milli5 Arduino Shield board - A3 - J8 pin4)

To set the mq6 sensor configuration LPG or CH4

Set mq6 measurement config
PUT /sensor/arduino/mq6?cfg=lpg      // measure LPG(ppm) concentration
PUT /sensor/arduino/mq6?cfg=ch4      // measure CH4(ppm) concentration

To Get mq6 sensor configuration ( LPG or CH4 )
GET /sensor/arduino/mq6?cfg

To read the mq6 sensor value:
GET /sensor/arduino/mq6?sens

To request an observe on the mq6 sensor value:
GET /sensor/arduino/mq6?sens with the CoAP observe header set

DELETE for disabling mq6 sensor
DELETE /sensor/arduino/mq6?all

*/

#include <string.h>
#include <Arduino.h>

#include "log.h"
#include "bufutil.h"
#include "exp_coap.h"
#include "coap_rsp_msg.h"
#include "coappdu.h"
#include "coapobserve.h"
#include "coapsensorobs.h"
#include "weight_sensor.h"
#include "arduino_pins.h"
#include <math.h>
#include "mq6_sensor.h"

/* PIN DECLARATION */
const int mq6_pin = A3; //analog input channel

#define LPG_PLOT_SIZE 21
#define CH4_PLOT_SIZE 20

float LPG_plot_ratio[LPG_PLOT_SIZE] =  {3.079, 2.041, 1.738, 1.534, 1.353, 1.26, 1.173, 1.092, 1.054, 1.017, 0.913, 0.835, 0.737, 0.639, 0.563, 0.515, 0.48, 0.455, 0.431, 0.408, 0.394}; // ratio Rs/Ro LPG plot
float LPG_plot_ppm[LPG_PLOT_SIZE] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1281, 1562, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000}; // ppm LPG plot

float CH4_plot_ratio[LPG_PLOT_SIZE] =  {3.463, 2.609, 2.213, 1.974, 1.802, 1.682, 1.578, 1.493, 1.414, 1.357, 1.146, 1.032, 0.895, 0.788, 0.716, 0.665, 0.621, 0.596, 0.567, 0.547}; // ratio Rs/Ro CH4 plot
float CH4_plot_ppm[LPG_PLOT_SIZE] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1562, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};   // ppm CH4 plot   

// based on MQ6 datasheet plot where x-axis is Rs/Ro ratio and y-axis is the concentration of gas in ppm.
// by taking 2 points from the curve of LPG or CH4 graphs it results the slope
// using Rs/R0 ratio with first point coordonates and slope the concentration of gas can be determinated using below formula
// Gas concentration(ppm) = 10^{(((log(Rs/R0)-(y1))/slope)+x1)}


//LPG Plot
//log of each point (lg_ppm, lg_rs_ro)=(2,0.4884)
//calculate the slope between intervals (y2 - y1) / (x2 - x1)

/*
ppm     Rs/Ro   log10(ppm)  log10(Rs/Ro)    slope
100     3.079   2.0000      0.4884
200     2.041   2.3010      0.3098      -0.5932
300     1.738   2.4771      0.2400      -0.3963
400     1.534   2.6021      0.1858      -0.4340
500     1.353   2.6990      0.1313      -0.5627
600     1.26    2.7782      0.1004      -0.3906
700     1.173   2.8451      0.0693      -0.4641
800     1.092   2.9031      0.0382      -0.5359
900     1.054   2.9542      0.0228      -0.3007
1000    1.017   3.0000      0.0073      -0.3392
1281    0.913   3.1075      -0.0395     -0.4356
1562    0.835   3.1937      -0.0783     -0.4503
2000    0.737   3.3010      -0.1325     -0.5051
3000    0.639   3.4771      -0.1945     -0.3519
4000    0.563   3.6021      -0.2495     -0.4402
5000    0.515   3.6990      -0.2882     -0.3994
6000    0.48    3.7782      -0.3188     -0.3860
7000    0.455   3.8451      -0.3420     -0.3470
8000    0.431   3.9031      -0.3655     -0.4058
9000    0.408   3.9542      -0.3893     -0.4656
10000   0.394   4.0000      -0.4045     -0.3314

CH4 Plot
ppm     Rs/Ro   log10(ppm)  log10(Rs/Ro)    slope
100     3.463   2.0000      0.5395
200     2.609   2.3010      0.4164      -0.4088
300     2.213   2.4771      0.3450      -0.4057
400     1.974   2.6021      0.2953      -0.3971
500     1.802   2.6990      0.2556      -0.4096
600     1.682   2.7782      0.2259      -0.3760
700     1.578   2.8451      0.1981      -0.4150
800     1.493   2.9031      0.1743      -0.4107
900     1.414   2.9542      0.1505      -0.4656
1000    1.357   3.0000      0.1326      -0.3904
1562    1.146   3.1937      0.0592      -0.3791
2000    1.032   3.3010      0.0135      -0.4252
3000    0.895   3.4771      -0.0480     -0.3494
4000    0.788   3.6021      -0.1036     -0.4448
5000    0.716   3.6990      -0.1453     -0.4301
6000    0.665   3.7782      -0.1770     -0.4010
7000    0.621   3.8451      -0.2068     -0.4447
8000    0.596   3.9031      -0.2246     -0.3080
9000    0.567   3.9542      -0.2465     -0.4268
10000   0.547   4.0000      -0.2623     -0.3470
*/

mq6_ctx_t mq6_ctx;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/

/*
 * crweight
 *
 * @brief CoAP Resource MQ6 sensor
 *
 */
error_t crmq6(struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it)
{
    struct optlv *o;
	uint8_t obs = false;


    /* No URI path beyond /mq6 is supported, so reject if present. */
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
     * PUT for writing configuration or enabling the relaycard
     */
    if (req->code == COAP_REQUEST_PUT)
    {
        error_t rc = ERR_OK;

        /* PUT /mq6?cfg=<lpg|ch4> */
        if (!coap_opt_strcmp(o, "cfg=lpg"))
        {
            arduino_put_mq6_cfg(MQ6_LPG);
        }
        else if (!coap_opt_strcmp(o, "cfg=ch4"))
        {
            arduino_put_mq6_cfg(MQ6_CH4);
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
        /* GET /mq6?cfg */
        if (!coap_opt_strcmp(o, "cfg"))
        {
            /* get mq6 config */
            rc = arduino_get_mq6_cfg( rsp->msg, &len );
        }
        /* GET /mq6?sens */
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
				rc = arduino_get_mq6( rsp->msg, &len );
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
    /* DELETE /mq6?all */
    else if (req->code == COAP_REQUEST_DELETE)
    {
        if (!coap_opt_strcmp(o, "all"))
        {
            if (arduino_disab_mq6())
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

} // crmq6

/*
 * Initialize MQ6 sensor.
 */
error_t arduino_mq6_sensor_init()
{

    SerialUSB.println("MQ6 LPG + CH4 Sensor Example");
    SerialUSB.println("------------------------------------");
    SerialUSB.println("Wait calibrating MQ6 sensor\n");

    //Calibrating MQ6 sensor in clean air
    mq6_ctx.mq6_Rovalue = mq6_sensor_calibration(MQ6_RLVALUE);
    // by default set LPG measurement
    mq6_ctx.mq6_gas_meas = MQ6_LPG;

    SerialUSB.println("MQ6 Ro after calibration:");
    SerialUSB.print("\t");
    SerialUSB.print(mq6_ctx.mq6_Rovalue);
    SerialUSB.print("kOhms");

    // Enable temp sensor
    arduino_enab_mq6();
    SerialUSB.println("------------------------------------");

	return ERR_OK;

} // arduino_mq6_sensor_init()



/*
 * Enable MQ6 sensor.
 */
error_t arduino_enab_mq6()
{
    error_t rc;
    rc = mq6_sensor_enable();
    if (!rc) {
        coap_stats.sensors_enabled++;
    }
    return rc;
}


/*
 * Disable MQ6 sensor.
 */
error_t arduino_disab_mq6()
{
    error_t rc;
    rc = mq6_sensor_disable();
    if (!rc) {
        coap_stats.sensors_disabled++;
    }
    return rc;
}

/**
 *
 * @brief Read MQ6 sensor value and return a message and length of message
 *
 *
 */
error_t arduino_get_mq6( struct mbuf *m, uint8_t *len )
{
	const uint32_t count = 1;
	float reading = 0;
    float rs_value;
    error_t rc;

	// Check if disabled
    if (!mq6_ctx.enable)
	{
		return ERR_OP_NOT_SUPP;

	} // if */

	/* Read mq6 sensor Rs value, already in network order. */
    rs_value = mq6_sensor_read(MQ6_RLVALUE);
    // Check MQ6 Measurement Configuration

    //  get mq6 sensor concentration ppm (parts per million) value based on gas type
    if ( mq6_ctx.mq6_gas_meas == MQ6_LPG )
    {
        reading = mq6_sensor_get_gasppm( (float) rs_value/mq6_ctx.mq6_Rovalue, MQ6_LPG );
    } // if
    else if ( mq6_ctx.mq6_gas_meas == MQ6_CH4 )
    {
        reading = mq6_sensor_get_gasppm( (float) rs_value/mq6_ctx.mq6_Rovalue, MQ6_CH4 );
    } // if

	// Assemble response
	rc = rsp_msg( m, len, count, &reading, "ppm" );
    // Return code
    return rc;
}


/* @brief
 * arduino_put_mq6_cfg()
 *
 * Set MQ6 Measurement Type
 */
error_t arduino_put_mq6_cfg( mq6_gas_meas_t cfg )
{
    switch(cfg)
    {
    case MQ6_LPG:
        mq6_ctx.mq6_gas_meas = MQ6_LPG;
        SerialUSB.println("MQ6 LPG Measurement Set!");
        break;

    case MQ6_CH4:
        mq6_ctx.mq6_gas_meas = MQ6_CH4;
        SerialUSB.println("MQ6 CH4 Measurement Set!");
        break;

    default:
        return ERR_INVAL;
    } // switch

    return ERR_OK;
}

/* @brief
 * arduino_get_mq6_cfg()
 *
 * Get MQ6 measurement configuration LPG or CH4
 */
error_t arduino_get_mq6_cfg( struct mbuf *m, uint8_t *len )
{
	uint32_t count = 0;
	char buf[17] = "";
	// Check MQ6 Measurement Configuration
	if ( mq6_ctx.mq6_gas_meas == MQ6_LPG )
	{
		strcpy( buf, " MQ6 LPG Meas Set" );
	} // if
    else if ( mq6_ctx.mq6_gas_meas == MQ6_CH4 )
    {
        strcpy( buf, " MQ6 CH4 Meas Set" );
    } // if

	// Assemble response
	rsp_msg( m, len, count, NULL, buf );

	return ERR_OK;
}   //arduino_get_mq6_cfg


/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

/*
 * mq6_sensor_calibration
 *
 * @param[in] rl - MQ6 board load resistance value
 * @return sensor resistor in clean air after calibration
 */
float mq6_sensor_calibration(float mq6_rlvalue)
{
    int i;
    float Rovalue = 0;
    float mq6_readvalue;

    for ( i=0; i<100; i++) {
        mq6_readvalue = analogRead(mq6_pin);
        Rovalue += ( ((float) mq6_rlvalue * (1023 - mq6_readvalue) / mq6_readvalue));
        delay(500);    //delay between each sample 500ms
    }

    //calculate Ro average value
    Rovalue = Rovalue / 100;

    //determine Ro value by dividing by Ro clean air value based on MQ6 datasheet
    Rovalue = Rovalue / MQ6_ROFACTOR;

    return Rovalue;
}

/*
 * mq6_sensor_read
 *
 * @param[in] rl - MQ6 board load resistance value
 * @return mq6 sensor resistor Rs value
 */
float mq6_sensor_read(float mq6_rlvalue)
{
    int i;
    float Rsvalue = 0;
    float mq6_readvalue;

    for ( i=0; i<10; i++) {
        mq6_readvalue = analogRead(mq6_pin);
        Rsvalue += ( ((float) mq6_rlvalue * (1023 - mq6_readvalue) / mq6_readvalue));
        delay(100);     //delay between each sample 100ms
    }

    //calculate Ro average value
    Rsvalue = Rsvalue / 10;

    return Rsvalue;
}

/*
 * mq6_sensor_get_gasppm
 *
 * @param[in] rs_ro_ration
 * @param[in] gas_type
 * @return mq6 sensor gas concentration ppm (parts per million) value based on gas type
 */
int mq6_sensor_get_gasppm(float rs_ro_ration, int gas_type)
{
    int gas_ppm = 0;
    int interval = 0;
    float gas_slope = 0;
    //Gas concentration(ppm) = 10^{(((log10(Rs/R0)-(y1))/slope)+x1)}
    //where slope is calculated based on (y2-y1)/(x2-x1)

    if ( gas_type == MQ6_LPG ) {
        //return the interval array index based Rs/Ro value
        interval = mq6_sensor_find_interval(LPG_plot_ratio, LPG_PLOT_SIZE, rs_ro_ration);

        //calculate the slope of the interval based on LPG datasheet curve
        gas_slope = ( log10(LPG_plot_ratio[interval]) - log10(LPG_plot_ratio[interval+1]) ) / ( log10(LPG_plot_ppm[interval]) - log10(LPG_plot_ppm[interval+1]) );

        //calculate the gas ppm concentration
        //gas concentration(ppm) = 10^{(((log10(Rs/R0)-(y1))/slope)+x1)}
        gas_ppm = pow(10,  ( ( log10(rs_ro_ration) - log10(LPG_plot_ratio[interval]) ) / gas_slope) + log10(LPG_plot_ppm[interval] ) );

    } else if ( gas_type == MQ6_CH4 ) {
        //return the interval array index based Rs/Ro value
        interval = mq6_sensor_find_interval(CH4_plot_ratio, CH4_PLOT_SIZE, rs_ro_ration);

        //calculate the slope of the interval based on CH4 datasheet curve
        gas_slope = ( log10(CH4_plot_ratio[interval]) - log10(CH4_plot_ratio[interval+1]) ) / ( log10(CH4_plot_ppm[interval]) - log10(CH4_plot_ppm[interval+1]) );

        //calculate the gas ppm concentration
        //gas concentration(ppm) = 10^{(((log10(Rs/R0)-(y1))/slope)+x1)}
        gas_ppm = pow(10,  ( ( log10(rs_ro_ration) - log10(CH4_plot_ratio[interval]) ) / gas_slope) + log10(CH4_plot_ppm[interval] ) );

    }
    SerialUSB.println("Curve interval index:");
    SerialUSB.println(interval);

    return gas_ppm;
}

/*
 * mq6_sensor_find_interval
 *
 * @param[in] array - LPG or CH4 arrays
 * @param[in] array_lenght
 * @param[in] search_value
 * @return index of Rs/Ro interval based on the datasheet curve
 */
int mq6_sensor_find_interval(float* array, int array_length, float search_value)
{
    int i = 0;
    while( (search_value < array[i]) && (i < array_length)) {
        i++;
    };
    return i;
}

/*
 * mq6_sensor_enable
 *
* @brief
 *
 */
error_t mq6_sensor_enable(void)
{
    mq6_ctx.enable = 1;
    return ERR_OK;
}   //mq6_sensor_enable

/*
 * mq6_sensor_disable
 *
* @brief
 *
 */
error_t mq6_sensor_disable(void)
{
    mq6_ctx.enable = 0;
    return ERR_OK;
}   //mq6_sensor_disable
