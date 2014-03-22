/*
 * Acess2 Network Stack Tester
 * - By John Hodge (thePowersGang)
 *
 * test_tcp.c
 * - Tests for the behavior of the "Transmission Control Protocol"
 */
#include "test.h"
#include "tests.h"
#include "net.h"
#include "stack.h"
#include "arp.h"
#include "tcp.h"
#include <string.h>

#define TEST_ASSERT_rx()	TEST_ASSERT( rxlen = Net_Receive(0, sizeof(rxbuf), rxbuf, ERX_TIMEOUT) )
#define TEST_ASSERT_no_rx()	TEST_ASSERT( Net_Receive(0, sizeof(rxbuf), rxbuf, NRX_TIMEOUT) == 0 )

bool Test_TCP_Basic(void)
{
	TEST_SETNAME(__func__);
	size_t	rxlen, ofs, len;
	char rxbuf[MTU];
	const int	ERX_TIMEOUT = 1000;	// Expect RX timeout (timeout=failure)
	const int	NRX_TIMEOUT = 250;	// Not expect RX timeout (timeout=success)
	const int	RETX_TIMEOUT = 1000;	// OS PARAM - Retransmit timeout
	const int	LOST_TIMEOUT = 1000;	// OS PARAM - Time before sending an ACK 
	const int	DACK_TIMEOUT = 500;	// OS PARAM - Timeout for delayed ACKs
	const size_t	DACK_BYTES = 4096;	// OS PARAM - Threshold for delayed ACKs

	tTCPConn	testconn = {
		.IFNum = 0, .AF = 4,
		.RAddr = BLOB(TEST_IP),
		.LAddr = BLOB(HOST_IP),
		.RPort = 80,
		.LPort = 11200,
		.Window = 0x1000,
		.LSeq = 0x1000,
		.RSeq = 0,
	};

	const char testblob[] = "HelloWorld, this is some random testing data for TCP\xFF\x00\x66\x12\x12";
	const size_t	testblob_len = sizeof(testblob);
	
	// 1. Test packets to closed port
	// > RFC793 Pg.65
	
	// 1.1. Send SYN packet
	TCP_SendC(&testconn, TCP_SYN, testblob_len, testblob);
	testconn.RSeq = 0;
	testconn.LSeq += testblob_len;
	// Expect RST,ACK with SEQ=0
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_RST|TCP_ACK) );
	TEST_ASSERT_REL(ofs, ==, rxlen);
	
	// 1.2. Send a SYN,ACK packet
	testconn.RSeq = 12345;
	TCP_SendC(&testconn, TCP_SYN|TCP_ACK, 0, NULL);
	// Expect a TCP_RST with SEQ=ACK
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_RST) );
	TEST_ASSERT_REL(ofs, ==, rxlen);
	testconn.LSeq ++;
	
	// 1.3. Send a RST packet
	TCP_SendC(&testconn, TCP_RST, 0, NULL);
	// Expect nothing
	TEST_ASSERT_no_rx();
	testconn.LSeq ++;
	
	// 1.3. Send a RST,ACK packet
	TCP_SendC(&testconn, TCP_RST|TCP_ACK, 0, NULL);
	// Expect nothing
	TEST_ASSERT_no_rx();
	testconn.LSeq ++;

	
	// 2. Establishing connection with a server
	testconn.RPort = 7;
	testconn.LPort = 11239;
	Stack_SendCommand("tcp_echo_server %i", testconn.RPort);

	// >>> STATE: LISTEN

	// 2.1. Send RST
	TCP_SendC(&testconn, TCP_RST, 0, NULL);
	// - Expect nothing
	TEST_ASSERT_no_rx();
	// 2.2. Send ACK
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	// - Expect RST
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_RST) );
	TEST_ASSERT_REL(ofs, ==, rxlen);

	// 2.3. Begin hanshake (SYN)
	// TODO: Test "If the SYN bit is set, check the security."
	TCP_SendC(&testconn, TCP_SYN, 0, NULL);
	testconn.LSeq ++;
	// - Expect SYN,ACK with ACK == SEQ+1
	TEST_ASSERT_rx();
	TCP_SkipCheck_Seq(true);
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_SYN|TCP_ACK) );
	testconn.RSeq = TCP_Pkt_GetSeq(rxlen, rxbuf, testconn.AF) + 1;

	// >>> STATE: SYN-RECEIVED
	// TODO: Test other transitions from SYN-RECEIVED
	
	// 2.4. Complete handshake, TCP ACK
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	// - Expect nothing
	TEST_ASSERT_no_rx();

	// >>> STATE: ESTABLISHED
	
	// 2.5. Send data
	TCP_SendC(&testconn, TCP_ACK|TCP_PSH, testblob_len, testblob);
	testconn.LSeq += testblob_len;

	// Expect echoed reponse with ACK
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL( len, ==, testblob_len );
	TEST_ASSERT( memcmp(rxbuf + ofs, testblob, testblob_len) == 0 );
	testconn.RSeq += testblob_len;

	// Send something short
	const char testblob2[] = "test blob two.";
	const size_t	testblob2_len = sizeof(testblob2);
	TCP_SendC(&testconn, TCP_ACK|TCP_PSH, testblob2_len, testblob2);
	testconn.LSeq += testblob2_len;
	// Expect response with data and ACK
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL( len, ==, testblob2_len );
	TEST_ASSERT( memcmp(rxbuf + ofs, testblob2, testblob2_len) == 0 );
	
	// Wait for just under retransmit time, expecting nothing
	#if TEST_TIMERS
	TEST_ASSERT( 0 == Net_Receive(0, sizeof(rxbuf), rxbuf, RETX_TIMEOUT-100) );
	// Now expect the previous message
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL( len, ==, testblob2_len );
	TEST_ASSERT( memcmp(rxbuf + ofs, testblob2, testblob2_len) == 0 );
	#else
	TEST_WARN("Not testing retransmit timer");
	#endif
	testconn.RSeq += testblob2_len;
	
	// Send explicit acknowledgement
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	TEST_ASSERT_no_rx();
	
	// TODO: Test delayed ACKs (Timeout and data)
	// > Requires inhibiting the server's echo response?
	
	// === Test out-of-order packets ===
	testconn.LSeq += testblob2_len;	// raise sequence number
	TCP_SendC(&testconn, TCP_ACK|TCP_PSH, testblob_len, testblob);
	// - previous data has not been sent, expect no response for ()
	// TODO: Should this ACK be delayed?
	//TEST_ASSERT_no_rx();
	// - Expect an ACK of the highest received packet
	testconn.LSeq -= testblob2_len;
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK) );
	TEST_ASSERT_REL( len, ==, 0 );
	// - Send missing data
	TCP_SendC(&testconn, TCP_ACK, testblob2_len, testblob2);
	testconn.LSeq += testblob_len+testblob2_len;	// raise sequence number
	// - Expect echo response with all sent data
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL( len, ==, testblob_len+testblob2_len );
	TEST_ASSERT( memcmp(rxbuf + ofs, testblob2, testblob2_len) == 0 );
	TEST_ASSERT( memcmp(rxbuf + ofs+testblob2_len, testblob, testblob_len) == 0 );
	testconn.RSeq += len;
	
	// 2.6. Close connection (TCP FIN)
	TCP_SendC(&testconn, TCP_ACK|TCP_FIN, 0, NULL);
	testconn.LSeq ++;	// Empty = 1 byte
	// Expect ACK? (Does acess do delayed ACKs here?)
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK) );
	TEST_ASSERT_REL( len, ==, 0 );
	// >>> STATE: CLOSE WAIT
	
	// Expect FIN
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_FIN) );
	TEST_ASSERT_REL( len, ==, 0 );
	
	// >>> STATE: LAST-ACK

	// 2.7 Send ACK of FIN
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	// Expect no response
	TEST_ASSERT_no_rx();
	
	// >>> STATE: CLOSED
	
	return true;
}

bool Test_TCP_SYN_RECEIVED(void)
{
	// 1. Get into SYN-RECEIVED
	
	// 2. Send various non-ACK packets
	return false;
}