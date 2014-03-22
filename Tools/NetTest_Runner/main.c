/*
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net.h"
#include "stack.h"
#include "tests.h"
#include "test.h"
#include <stdarg.h>
#include <unistd.h>

// === PROTOTYPES ===
 int	ParseCommandline(int argc, char *argv[]);

// === GLOBALS ===
const char *gsTestName;

// === CODE ===
int main(int argc, char *argv[])
{
	if( ParseCommandline(argc, argv) )
		return 1;

	typedef bool t_test(void);
	t_test	*tests[] = {
		Test_ARP_Basic,
		Test_TCP_Basic,
		NULL
		};

	// TODO: Move to stack.c
	FILE	*fp;
	fp = fopen("stdout.txt", "w");	fclose(fp);
	fp = fopen("stderr.txt", "w");	fclose(fp);
	
	Net_Open(0, "/tmp/acess2net");

	for(int i = 0; tests[i]; i ++ )
	{
		Stack_AddDevice("/tmp/acess2net", (char[]){TEST_MAC});
		Stack_AddInterface("eth0", 4, (const char[]){TEST_IP}, 24);
		Stack_AddRoute(4, "\0\0\0\0", 0, (const char[]){HOST_IP});
		if( Stack_Start("cmdline") )
			goto teardown;
		
		if( Net_Receive(0, 1, &argc, 1000) == 0 )
			goto teardown;
		
		if( tests[i]() )
			printf("%s: PASS\n", gsTestName);
		else
			printf("%s: FAIL\n", gsTestName);
	
	teardown:
		Stack_Kill();
	}
	Net_Close(0);
	unlink("/tmp/acess2net");

	return 0;
}

void PrintUsage(const char *ProgName)
{
	fprintf(stderr, "Usage: %s\n", ProgName);
}

int ParseCommandline(int argc, char *argv[])
{
	const char	*progname = argv[0];
	for( int i = 1; i < argc; i ++ )
	{
		const char	*arg = argv[i];
		if( arg[0] != '-' ) {
			// bare args
		}
		else if( arg[1] != '-' ) {
			// short args
		}
		else {
			// long args
			if( strcmp(arg, "--help") == 0 ) {
				PrintUsage(progname);
				exit(0);
			}
			else {
				fprintf(stderr, "Unknown option: %s\n", arg);
				PrintUsage(progname);
				return 1;
			}
		}
	}
	return 0;
}

void test_setname(const char *name)
{
	gsTestName = name;
}

void test_message(const char *filename, int line, const char *msg, ...)
{
	fprintf(stderr, "%s:%i [%s] - ", filename, line, gsTestName);
	va_list	args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void test_assertion_fail(const char *filename, int line, const char *fmt, ...)
{
	fprintf(stderr, "%s:%i [%s] - ASSERT FAIL ", filename, line, gsTestName);
	va_list	args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void test_trace(const char *msg, ...)
{
	printf("TRACE: [%s] ", gsTestName);
	va_list args;
	va_start(args, msg);
	vfprintf(stdout, msg, args);
	va_end(args);
	printf("\n");
}
void test_trace_hexdump(const char *hdr, const void *data, size_t len)
{
	printf("TRACE: [%s] %s - %zi bytes\n", gsTestName, hdr, len);
	const uint8_t	*data8 = data;
	while( len > 16 )
	{
		printf("TRACE: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
			data8[0], data8[1], data8[ 2], data8[ 3], data8[ 4], data8[ 5], data8[ 6], data8[ 7],
			data8[8], data8[9], data8[10], data8[11], data8[12], data8[13], data8[14], data8[15]
			);
		len -= 16;
		data8 += 16;
	}
	printf("TRACE: ");
	while( len > 8 )
	{
		printf("%02x %02x %02x %02x %02x %02x %02x %02x  ",
			data8[0], data8[1], data8[ 2], data8[ 3], data8[ 4], data8[ 5], data8[ 6], data8[ 7]
			);
		len -= 8;
		data8 += 8;
	}
	while(len > 0)
	{
		printf("%02x ", data8[0]);
		len --;
		data8 ++;
	}
	printf("\n");
}
