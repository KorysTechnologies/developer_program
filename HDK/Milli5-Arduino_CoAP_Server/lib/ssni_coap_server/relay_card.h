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
#ifndef RELAY_CARD_H_
#define RELAY_CARD_H_

#include <Arduino.h>
#include "errors.h"
#include "hbuf.h"

typedef enum
{
	RELAYCARD_OPEN = 0x0,
	RELAYCARD_CLOSE = 0x1,

} relaycard_state_t;


typedef struct relaycard_cfg
{
	uint8_t state;
	uint8_t enable;

} relaycard_cfg_t;

/******************************************************************************/
/*                      Public Methods                                        */
/******************************************************************************/

/**
 *
 * @brief CoAP Resource "put description here"
 *
 */
error_t crrelaycard(struct coap_msg_ctx *req, struct coap_msg_ctx *rsp, void *it);

/**
 * @brief Enable Relaycard
 *
 * @return error_t
 */
error_t arduino_enab_relaycard(void);

/**
 * @brief Disable Relaycard
 *
 * @return error_t
 */
error_t arduino_disab_relaycard(void);

/**
 * @brief Init Relaycard
 * @return error_t
 *
 */
error_t arduino_relaycard_init();

/**
 * @brief Set Relaycard state
 * @param[in] state Desired state
 * @return error_t
 */
error_t arduino_put_relaycard_state( relaycard_state_t state);

/**
 * @brief Get Relaycard state
 * @param[in] m Pointer to input mbuf
 * @param[in] len Length of input
 * @return error_t
 */
error_t arduino_get_relaycard_state(struct mbuf *m, uint8_t *len);


/******************************************************************************/
/*                     Private Methods                                        */
/******************************************************************************/

/**
 * @brief
 * @return error_t
 */
error_t relaycard_enable(void);

/**
 * @brief
 * @return error_t
 */
error_t relaycard_disable(void);

#endif /* RELAY_CARD_H_ */

