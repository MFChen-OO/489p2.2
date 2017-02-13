#include <iostream>
#include "DNSMsg.h"
#include <stdint.h>



using namespace std;


int main() {
	DNSHeader header;
	header.ID = 1 << 10 + 2;
	header.QR = true; // quesiotn
	header.OPCODE = 0;
	header.AA = 0; // query
	header.TC = false;
	header.RD = 0;
	header.RA = 0;
	header.Z = 0;
	header.RCODE = 0;
	header.QDCOUNT = 1;
	header.ANCOUNT = 1;
	header.NSCOUNT = 0;
	header.ARCOUNT = 0;

	DNSQuestion question;
	strcpy(question.QNAME, "www.google.com");
	question.QTYPE = 1;
	question.CLASS = 1;

	char qmsg[200];
	int len = to_qmsg(qmsg, header, question);
}