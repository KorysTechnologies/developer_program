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


The circuit configuration - Relaycard Keyes_SR1y
  - minus pin: ground (Milli5 Arduino Shiled board - J15 pin6 )
  - plus pin: 5V (Milli5 Arduino Shiled board - J15 pin5 )
  - S pin: 10 (Milli5 Arduino Shiled board - J6 pin3 )



To set the relaycard state:

PUT /sensor/arduino/relaycard?state=open
PUT /sensor/arduino/relaycard?state=close


To get the relaycard state:
GET /sensor/arduino/relaycard?state

DELETE for disabling relaycard
DELETE /sensor/arduino/relaycard?all

*/


#include "log.h"
#include "bufutil.h"
#include "exp_coap.h"
#include "coap_rsp_msg.h"
#include "coappdu.h"
#include "coapmsg.h"
#include "coapobserve.h"
#include "coapsensorobs.h"
#include "arduino_pins.h"
#include "relay_card.h"

/* PIN DECLARATION */
const int relaycard_pin = 10;

// Set of relaycard State as OPEN
static relaycard_state_t relaycard_state = RELAYCARD_OPEN;

// Relaycard Config
relaycard_cfg_t relaycard_cfg;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/


/**
 * crrelaycard
 *
 * @brief CoAP Resource relaycard
 *
 */
error_t crrelaycard(struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it)
{
    struct optlv *o;

    /* No URI path beyond /relaycard is supported, so reject if present. */
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

        /* PUT /uri?state=<open|close> */
        if (!coap_opt_strcmp(o, "state=open"))
        {
			arduino_put_relaycard_state(RELAYCARD_OPEN);
        }
        else if (!coap_opt_strcmp(o, "cfg=close"))
        {
			arduino_put_relaycard_state(RELAYCARD_CLOSE);
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
     * GET for reading relaycard state
     */
    else if (req->code == COAP_REQUEST_GET)
    {
        uint8_t rc, len = 0;

        /* relaycard state */
        /* GET /relaycard?state */
        if (!coap_opt_strcmp(o, "state"))
        {
            /* get relaycard state */
            rc = arduino_get_relaycard_state( rsp->msg, &len );
        }
        else {
            /* Don't support other queries. */
            rsp->code = COAP_RSP_501_NOT_IMPLEMENTED;
            goto err;
        }
        dlog(LOG_DEBUG, "GET (status %d) read %d bytes.", rc, len);
        if (!rc) {
            rsp->plen = len;
            rsp->cf = COAP_CF_CSV;
            rsp->code = COAP_RSP_205_CONTENT;
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
    /* DELETE /relaycard?all */
    else if (req->code == COAP_REQUEST_DELETE)
    {
        if (!coap_opt_strcmp(o, "all"))
        {
            if (arduino_disab_relaycard())
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

err:
    rsp->plen = 0;

    return ERR_OK;

} // crrelaycard

/**
 * @brief
 *
 */
error_t arduino_relaycard_init()
{
    // Initialize relaycard pin - OPEN state by Default
    pinMode(relaycard_pin, OUTPUT);
    digitalWrite(relaycard_pin, LOW);

    // Enable relaycard
    arduino_enab_relaycard();
    SerialUSB.println("");
    SerialUSB.println("------------------------------------------");
    SerialUSB.println("Relaycard Example - By Default Open State");
    SerialUSB.println("-----------------------------------------");
    return ERR_OK;

} // arduino_init_relaycard


/**
 * @brief Enable relaycard.
 */
error_t arduino_enab_relaycard()
{
    error_t rc;
    rc = relaycard_enable();
    if (!rc) {
        coap_stats.sensors_enabled++;
    }
    return rc;
}


/**
 * @brief Disable relaycard.
 */
error_t arduino_disab_relaycard()
{
    error_t rc;
    rc = relaycard_disable();
    if (!rc) {
        coap_stats.sensors_disabled++;
    }
    return rc;
}



/**
 * @brief Get relaycard state
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_relaycard_state( struct mbuf *m, uint8_t *len )
{
	error_t rc = ERR_OK;
	char buf[32];
	const uint32_t count = 0;
	switch(relaycard_cfg.state)
	{
		case RELAYCARD_OPEN:
			strcpy(buf,"Relaycard State: Open");
			break;

		case RELAYCARD_CLOSE:
			strcpy(buf,"Relaycard State: Close");
			break;

		default:
			return ERR_FAIL;

	} // switch
	rc = rsp_msg( m, len, count, NULL, buf );
    return rc;
}



/* @brief
 * arduino_put_relaycard_state()
 *
 * Set relaycard State
 */
error_t arduino_put_relaycard_state( relaycard_state_t state )
{
    switch(state)
    {
    case RELAYCARD_OPEN:
        relaycard_state = RELAYCARD_OPEN;
        digitalWrite(relaycard_pin, LOW);
        SerialUSB.println("Relaycard Open State Set!");

        break;

    case RELAYCARD_CLOSE:
        relaycard_state = RELAYCARD_CLOSE;
        digitalWrite(relaycard_pin, HIGH);
        SerialUSB.println("Relaycard Close State Set!");
        break;

    default:
        return ERR_INVAL;

    } // switch

    // Enable relaycard
    arduino_enab_relaycard();
    return ERR_OK;

}


/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/
/*
 * @brief Enable sensor
 * @return error_t
 */
error_t relaycard_enable(void)
{
    relaycard_cfg.enable = 1;
	return ERR_OK;
}

/*
 * @brief Disable sensor
 * @return error_t
 */
error_t relaycard_disable(void)
{
    relaycard_cfg.enable = 0;
	return ERR_OK;
}

