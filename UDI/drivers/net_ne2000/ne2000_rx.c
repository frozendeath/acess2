/*
 * UDI Ne2000 NIC Driver
 * By John Hodge (thePowersGang)
 *
 * ne2000_rx.c
 * - Receive Code
 */
#include <udi.h>
#include <udi_nic.h>
#include "ne2000_common.h"

// === PROTOTYPES ===
udi_pio_trans_call_t	ne2k_rx__complete;

// === CODE ===
void ne2k_nd_rx_channel_event_ind(udi_channel_event_cb_t *cb)
{
}
void ne2k_nd_rx_rx_rdy(udi_nic_rx_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	ne2k_rdata_t	*rdata = gcb->context;
	
	// Add cb(s) to avaliable list
	if( rdata->rx_last_cb ) {
		rdata->rx_last_cb->chain = cb;
	}
	else {
		rdata->rx_next_cb = cb;
	}
	rdata->rx_last_cb = cb;
	// Follow new chain
	while( rdata->rx_last_cb->chain )
		rdata->rx_last_cb = rdata->rx_last_cb->chain;
	
}
void ne2k_intr__rx_ok(udi_cb_t *gcb)
{
	ne2k_rdata_t	*rdata = gcb->context;
	if( rdata->rx_next_cb )
	{
		udi_nic_rx_cb_t	*rx_cb = rdata->rx_next_cb;
		rdata->rx_next_cb = rx_cb->chain;
		rx_cb->chain = NULL;
		udi_pio_trans(ne2k_rx__complete, UDI_GCB(rx_cb),
			rdata->pio_handles[NE2K_PIO_RX], 0, NULL, &rdata->rx_next_page);
	}
	else
	{
		// Drop packet due to no free cbs
		udi_debug_printf("ne2k_intr__rx_ok: Dropped due to no free rx cbs\n");
		// TODO: Tell hardware to drop packet
	}
}

void ne2k_rx__complete(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t result)
{
	udi_nic_rx_cb_t	*rx_cb = UDI_MCB(gcb, udi_nic_rx_cb_t);
	// TODO: Check result
	udi_nsr_rx_ind( rx_cb );
}


