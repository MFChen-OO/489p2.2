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
#include <chrono>
#include <ctime>

using namespace std;

#define CHUNCK_SIZE 1000

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


void server_mode(char *port_num) {
	// int sd, new_sd;
	// struct sockaddr_in sin;
	// struct sockaddr_storage their_addr;
	// int addr_size;
	// int yes = 1;
	// int qlen = 20;
	// char ipstr[INET6_ADDRSTRLEN];

	// if ((sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
	// 	perror("opening TCP socket");
	// 	abort();
	// }

	// if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes), sizeof(int)) < 0) {
	// 	perror("setsockopt");
	// 	abort();
	// }

	// memset(&sin, 0, sizeof(sin));
	// sin.family = AF_INET;
	// sin.sin_addr.s_addr = INADDR_ANY;
	// sin.sin_port = htons(port_num);

	// if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	// 	perror("bind");
	// 	printf("Cannot bind socket to address \n");
	// 	abort();
	// }

	// if (listen(sd, qlen) < 0) {
	// 	perror("error listening");
	// 	abort();
	// }

	// addr_size = sizeof(their_addr);
	// if ((new_sd = accept(sd, (struct sockaddr *)&their_addr, &addr_size)) < 0) {
	// 	perror("accept");
	// 	abort();
	// }

	// inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), ipstr, sizeof(ipstr));
	// cout << "Server: got connection from " << ipstr << endl;

	int sockfd, new_fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char ipstr[INET6_ADDRSTRLEN];
	int rv, received_total, received_bytes, len;
	char buf[CHUNCK_SIZE + 1];

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

	sin_size = sizeof(their_addr);
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (new_fd == -1) {
		perror("accept");
		abort();
	}

	inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),
		ipstr, sizeof(ipstr));

	received_total = 0;
	memset(buf, '0', CHUNCK_SIZE);
	buf[CHUNCK_SIZE] = '\0';

    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();

	// receive zeros
	while (*buf != '1') {
		received_bytes = recv(new_fd, &buf, sizeof(buf), 0);
		if (received_bytes == -1) {
			perror("recv");
			abort();
		}

		received_total += received_bytes;
	}

	memset(buf, '2', CHUNCK_SIZE);
	buf[CHUNCK_SIZE + 1] = '\0';

	end = chrono::system_clock::now();
    chrono::duration<double> send_time = end - start;
    cout << send_time.count() << endl;

	// send acknowledgement
	len = strlen(buf);
    if (sendall(sockfd, buf, &len) == -1) {
    	perror("server: sendall");
    	abort();
    }

    close(sockfd);
    close(new_fd);

	

}

void client_mode(char *server_name, char *port_num, int t) {
	int sockfd, num, len, total, received_bytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char ipstr[INET6_ADDRSTRLEN];
	char buf[CHUNCK_SIZE + 1];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(server_name, port_num, &hints, &servinfo)) != 0) {
		perror("getaddrinfo");
		abort();
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client:connect");
			continue;
		}
		break;
	}

	if (p == NULL) {
		perror("client: fail to connect\n");
		abort();
	}

	inet_ntop(p->ai_family,
		get_in_addr((struct sockaddr *)p->ai_addr),
		ipstr, sizeof(ipstr));

	cout << "client connecting to " << ipstr <<endl;

	freeaddrinfo(servinfo);

	memset(buf, '0', CHUNCK_SIZE);
	buf[CHUNCK_SIZE] = '\0';

	total = 0;
	chrono::microseconds micro_sec(t * 1000000);
    chrono::duration<double> stime(micro_sec); // send duration
    chrono::duration<double> elapsed_seconds;
    chrono::time_point<chrono::system_clock> start, current, end;
    start = chrono::system_clock::now();

    // send 
    do {
    	len = strlen(buf);
        if (sendall(sockfd, buf, &len) == -1) {
        	perror("sendall");
        	abort();
        }
        total += len;
        current = chrono::system_clock::now();
        elapsed_seconds = current - start;
    } while(elapsed_seconds < stime);


    // send FIN message
    memset(buf, '1', CHUNCK_SIZE);
    if (sendall(sockfd, buf, &len) == -1) {
    	perror("sendall");
    	abort();
    }

    // receive acknowledgement
    while (*buf == '1') {
    	received_bytes = recv(sockfd, buf, sizeof(buf), 0);
    	if (received_bytes == -1) {
			perror("recv");
			abort();
		}
    }

    end = chrono::system_clock::now();
    chrono::duration<double> send_time = end - start;
    cout << send_time.count() << endl;

    close(sockfd);
}

int main (int argc, char *argv[]) {
	int c;
	char mode = 'u';
	char port_num[6];
	char hostname[20];
	int t = 0;
	while ((c = getopt(argc, argv, "sch:p:t:")) != -1) {
		switch(c) {
			case 's': {
				if (argc != 4) {
					cout << "Error: missing or additional arguments" << endl;
					abort();
				}
				mode = 's';
			}
				break;
			case 'c': {
				if (argc != 8) {
					cout << "Error: missing or additional arguments" << endl;
					abort();
				}
				mode = 'c';
			}
				break;
			case 'h': {
				if (mode == 'u') {
					cout << "Error: missing or additional arguments" << endl;
					abort();
				}
				strcpy(hostname, optarg);
			}
				break;
			case 'p': {
				if (mode == 'u') {
					cout << "Error: missing or additional arguments" << endl;
					abort();
				}
				int portNum = atoi(optarg);
				if (portNum < 1024 || portNum > 65535) {
					cout<< "Error: port number must be in the range 1024 to 65535" << endl;
					abort();
				}
				strcpy(port_num, optarg);
			}
				break;
			case 't': {
				if (mode == 'u') {
					cout << "Error: missing or additional arguments" << endl;
					abort();
				}
				t = atoi(optarg);
			}
				break;
			case '?': {
				cout << "Error: missing or additional arguments" << endl;
				abort();
			}
			default: {
				cout << "Error: missing or additional arguments" << endl;
				abort();
			}
		}
	}

	if (mode == 's') {
		cout<<port_num<<endl;
		server_mode(port_num);
	} else if (mode == 'c') {
		cout<<port_num<<endl<<hostname<<endl<<t<<endl;
		client_mode(hostname, port_num, t);
	}
	return 0;
}