#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <queue>
#include "DNSMsg.h"

using namespace std;

#define MAX_QLEN 120
#define MAX_RLEN 220

struct path {
	int id;
	int cost; // cost from client to the node
	path(int i, int c) : id(i), cost(c) {}
	path() {}
};

struct comp {
	bool operator()(path p1, path p2) {
		return p1.cost > p2.cost;
	}
};

int sendall(int s, char *buf, int *len) {
	int total = 0;
	int bytesleft = *len;
	int n;

	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
		if (n == - 1) { break; }
		total += n;
		bytesleft -= n;
	}

	*len = total;

	return n == -1 ? -1 : 0;
}

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void round_robin(char * log, char * port_num, char * servers) {
	// read file
	string ip_addr;
	vector<string> ip_addrs;
	int idx = 0;

	ifstream fin;
	fin.open(servers);
	if (!fin.is_open()) {
		cout << "open failed" << endl;
		exit(1);
	}
	while (fin >> ip_addr) {
		ip_addrs.push_back(ip_addr);
	}


	// set socket
	int sockfd, clientsd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char ipstr[INET6_ADDRSTRLEN];
	int rv, received_total, received_bytes, len;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, port_num, &hints, &servinfo)) != 0) {
		perror("getaddrinfo");
		abort();
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			abort();
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL) {
		perror("failed to bind");
		abort();
	}

	// fd_set readSet;
	// vector<int> fds;

	while (true) {
		// FD_ZERO(&readSet);
		// FD_SET(sd, &readSet);

		// for (int i = 0; i < (int) fds.size(); i++) {
		// 	FD_SET(fds[i], &readSet);
		// }
		
		// int maxfd = 0;
		// if (fds.size() > 0) {
		// 	maxfd = *max_element(fds.begin(), fds.end());
		// }
		// maxfd = max(maxfd, sd);

		// int err = select(maxfd + 1, &readSet, NULL, NULL, NULL);
		// assert(err != -1);

		// // if listener port is set
		// if (FD_ISSET(sd, &readSet)) {
		// 	sin_size = sizeof(their_addr);
		// 	int clientsd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		// 	if (clientsd == -1) {
		// 		cout << "Error on accept" << endl;
		// 	} else {
		// 		fds.push_back(clientsd);
		// 	}
		// }
		sin_size = sizeof(their_addr);
		clientsd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (clientsd == -1) {
			perror("accept");
			abort();
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			ipstr, sizeof(ipstr));



		// check all new sockets
		int bytesRecvd, numBytes;
		// for (int i = 0; i < (int) fds.size(); i++) {
		bytesRecvd = 0;
		char qmsg[MAX_QLEN];
		// if (FD_ISSET(fds[i], &readSet)) {
		while(bytesRecvd < MAX_QLEN) {
			numBytes = recv(clientsd, qmsg + bytesRecvd, MAX_QLEN - bytesRecvd, 0);
			if (numBytes < 0) {
				cout << "Error receiving bytes" << endl;
				exit(1);
			}
			bytesRecvd += numBytes;
		}

		DNSHeader header;
		DNSQuestion question;
		parse_qmsg(qmsg, &header, &question);

		char rmsg[MAX_RLEN];
		header.QR = 1; // reuse header
		header.AA = 1;

		if (strcmp(question.QNAME, "video.cse.umich.edu") != 0) {
			header.RCODE = 3;
			header.ANCOUNT = 0;
			int i = 0;
			header_to_msg(rmsg, &header, i);
		} else {
			header.RCODE = 0;
			header.ANCOUNT = 1;

			DNSRecord record;
			strcpy(record.NAME, question.QNAME);
			record.TYPE = 1;
			record.CLASS = 1;
			record.TTL = 0;

			string ip = ip_addrs[idx];
			idx = (idx + 1) % (int) ip_addrs.size();

			record.RDLENGTH = ip.size();
			strcpy(record.RDATA, ip.c_str());
			to_rmsg(rmsg, &header, &record);
		}

		int len = MAX_RLEN;
		if (sendall(clientsd, rmsg, &len) == -1) {
			cout << "Error on sendall" << endl;
			exit(1);
		}

		close(clientsd);
		// fds.erase(fds.begin() + i);
		// }
		// }

	} // while
	close(sockfd);

}

void geo_based(char * log, char * port_num, char * servers) {
	vector<string> ips;
	vector<string> types;
	unordered_map<string, int> id_table;
	unordered_map<int, vector<pair<int, int>>> network; // first is id, second is cost
	priority_queue<path, vector<path>, comp> pq;
	int num_nodes, num_links, id, id1, id2, cost;
	string ip, type, tok;

	ifstream fin;
	fin.open("geo_dist.txt");
	if (!fin.is_open()) {
		cout << "open failed" << endl;
		exit(1);
	}
	
	fin >> tok >> num_nodes;
	for (int i = 0; i < num_nodes; i++) {
		fin >> id >> type >> ip;
		cout << id << ' ' << type << ' ' << ip << endl;
		ips.push_back(ip);
		types.push_back(type);
		id_table[ip] = i;
	}

	fin >> tok;
	fin >> num_links;
	for (int i = 0; i < num_links; i++) {
		fin >> id1 >> id2 >> cost;
		network[id1].push_back(make_pair(id2, cost));
		network[id2].push_back(make_pair(id1, cost));
	}

	// set socket
	int sockfd, clientsd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char ipstr[INET6_ADDRSTRLEN];
	int rv, received_total, received_bytes, len;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, port_num, &hints, &servinfo)) != 0) {
		perror("getaddrinfo");
		abort();
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			abort();
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL) {
		perror("failed to bind");
		abort();
	}

	// fd_set readSet;
	// vector<int> fds;

	while (true) {
		sin_size = sizeof(their_addr);
		clientsd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (clientsd == -1) {
			perror("accept");
			abort();
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			ipstr, sizeof(ipstr));



		// check all new sockets
		int bytesRecvd, numBytes;
		// for (int i = 0; i < (int) fds.size(); i++) {
		bytesRecvd = 0;
		char qmsg[MAX_QLEN];
		// if (FD_ISSET(fds[i], &readSet)) {
		while(bytesRecvd < MAX_QLEN) {
			numBytes = recv(clientsd, qmsg + bytesRecvd, MAX_QLEN - bytesRecvd, 0);
			if (numBytes < 0) {
				cout << "Error receiving bytes" << endl;
				exit(1);
			}
			bytesRecvd += numBytes;
		}

		DNSHeader header;
		DNSQuestion question;
		parse_qmsg(qmsg, &header, &question);

		char rmsg[MAX_RLEN];
		header.QR = 1; // reuse header
		header.AA = 1;

		// find closest server
		string client_ip(ipstr);
		int start_id = id_table[client_ip];
		path start(start_id, 0);
		path cur_path, next_path;

		pq.push(start);
		while (!pq.empty()) {
			cur_path = pq.top();
			pq.pop();
			if (types[cur_path.id] == "SERVER") {
				break;
			}

			for (pair<int, int> next_node : network[cur_path.id]) {
				if (types[next_node.first] == "CLIENT") { // avoid client added to the queue
					continue;
				}
				next_path.id = next_node.first;
				next_path.cost = cur_path.cost + next_node.second;
				pq.push(next_path);
			}
		}

		if (strcmp(question.QNAME, "video.cse.umich.edu") != 0 || types[cur_path.id] != "SERVER") {
			header.RCODE = 3;
			header.ANCOUNT = 0;
			int i = 0;
			header_to_msg(rmsg, &header, i);
		} else {
			header.RCODE = 0;
			header.ANCOUNT = 1;

			DNSRecord record;
			strcpy(record.NAME, question.QNAME);
			record.TYPE = 1;
			record.CLASS = 1;
			record.TTL = 0;

			string ip = ips[cur_path.id];

			record.RDLENGTH = ip.size();
			strcpy(record.RDATA, ip.c_str());
			to_rmsg(rmsg, &header, &record);
		}

		int len = MAX_RLEN;
		if (sendall(clientsd, rmsg, &len) == -1) {
			cout << "Error on sendall" << endl;
			exit(1);
		}

		close(clientsd);
	} // while
	close(sockfd);
}


int main (int argc, char *argv[]) {
	bool geo = *argv[3] - '0';
	if (geo) {
		geo_based(argv[1], argv[2], argv[4]);
	} else {
		round_robin(argv[1], argv[2], argv[4]);
	}
	return 0;
}