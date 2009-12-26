/*
 * Acess2 IP Stack
 * - IPv6 Definitions
 */
#ifndef _IPV6_H_
#define _IPV6_H_

#include "ipstack.h"

typedef struct sIPv6Header	tIPv6Header;

struct sIPv6Header
{
	struct {
		unsigned Version:	4;
		unsigned TrafficClass:	8;
		unsigned FlowLabel:	20;
	};
	Uint16	PayloadLength;
	Uint8	NextHeader;	// Type of payload data
	Uint8	HopLimit;
	tIPv6	Source;
	tIPv6	Destination;
};

#define IPV6_ETHERNET_ID	0x86DD

#endif