#include "DNSMsg.h"
#include <cstring>

ushort msg_to_ushort(char *msg, int &i) {
	return (ushort)(msg[i++] << 8) + (ushort)msg[i++];
}

void msg_to_header(char * msg, DNSHeader * header, int &i) {
	header->ID = msg_to_ushort(msg, i);
	header->QR = msg[i] >> 7;
	header->OPCODE = (msg[2] >> 3) & 0x0f;
	header->AA = (msg[i] >> 2) & 0x01;
	header->TC = (msg[i] >> 1) & 0x01;
	header->RD = msg[i] & 0x01;
	i++;
	header->RA = msg[i] >> 7;
	header->Z = (msg[i] >> 4) & 0x07;
	header->RCODE = msg[i] & 0x0f;
	i++;
	header->QDCOUNT = msg_to_ushort(msg, i);
	header->ANCOUNT = msg_to_ushort(msg, i);
	header->NSCOUNT = msg_to_ushort(msg, i);
	header->ARCOUNT = msg_to_ushort(msg, i);
}

void msg_to_name(char * msg, char * name, int &i) {
	int len = msg[i++];
	int name_idx = 0;
	while (len > 0) {
		strncpy(name + name_idx, msg + i, len)
		i += len;
		name_idx += len;
		name[name_idx++] = '.';
		len = msg[i++];
	}
	name[name_idx] = '\0';
}

void parse_qmsg(char * msg, DNSHeader * header, DNSQuestion * question) {
	int i = 0;
	msg_to_header(msg, header, i);

	// parse QNAME
	msg_to_name(msg, question->QNAME, i);

	question->QTYPE = msg_to_ushort(msg, i);
	question->QCLASS = msg_to_ushort(msg, i);
}

void parse_rmsg(char * msg, DNSHeader * header, DNSRecord * record) {
	int i = 0;
	msg_to_header(msg, header, i);

	msg_to_name(msg, name, i);
	record->TYPE = msg_to_ushort(msg, i);
	record->CLASS = msg_to_ushort(msg, i);
	record->TTL = msg_to_ushort(msg, i);
	record->RDLENGTH = msg_to_ushort(msg, i);
	strncpy(record->RDATA, msg + i, RDLENGTH);
}

void ushort_to_msg(char * msg, ushort value, int &i) {
	msg[i++] = value >> 8;
	msg[i++] = value & 0xff;
}
void name_to_msg(char * msg, char * name, int &i) {
	int label_len = 0; // len of one label
	int name_idx = 0; //
	while (question->QNAME[name_idx] != '\0') {
		while (question->QNAME[name_idx + label_len] != '.') {
			label_len++;
		}
		msg[i++] = label_len;
		strncpy(msg + i, question->QNAME + name_idx, label_len);
		i += label_len;
		name_idx += label_len + 1; 
		label_len = 0;
	}
}

void head_to_msg(char * msg, DNSHeader * header, int &i) {
	ushort_to_msg(msg, header->ID, i);
	msg[2] = 0;
	msg[2] += header->QR << 7;
	msg[2] += header->OPCODE << 3;
	msg[2] += header->AA << 2;
	msg[2] += header->TC << 1;
	msg[2] += header->RD;
	msg[3] = 0;
	msg[3] += header->RA << 7;
	msg[3] += header->Z << 4;
	msg[3] += header->RCODE;
	i = 4;
	ushort_to_msg(msg, header->QDCOUNT, i);
	ushort_to_msg(msg, header->ANCOUNT, i);
	ushort_to_msg(msg, header->NSCOUNT, i);
	ushort_to_msg(msg, header->ARCOUNT, i);
}

// convert DNSHeader and DNSQuestion to msg string
// return the length of msg
int to_qmsg(char * msg, DNSHeader * header, DNSQuestion * question) {
	int i = 0;
	header_to_msg(msg, header, i);
	name_to_msg(msg, question->QNAME, i);
	ushort_to_msg(msg, question->QTYPE, i);
	ushort_to_msg(msg, question->QCLASS, i);
	return i;
 }

// convert DNSHeader and DNSRecord to msg string
// return the length of msg
int to_rmsg(char * msg, DNSHeader * header, DNSRecord * record) {
	int i = 0;
	header_to_msg(msg, header, i);
	name_to_msg(msg, record->NAME, i);
	ushort_to_msg(msg, record->TYPE, i);
	ushort_to_msg(msg, record->CLASS, i);
	ushort_to_msg(msg, record->TTL, i);
	ushort_to_msg(msg, record->RDLENGTH, i);
	strncpy(msg + i, record->RDATA, record->RDLENGTH);
	return  i + RDLENGTH;
}



