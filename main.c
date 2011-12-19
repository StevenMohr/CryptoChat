/*
 * main.c
 *
 *  Created on: 27.11.2011
 *      Author: steven
 */
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sqlite3.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "main.h"
#include "sqilte.h"

int main(int argc, char *argv[]) {
	RAND_load_file("seed-file.dat", -1);
	if (argc == 2 && strcmp(argv[1], "-h") == 0) {
		print_help();
		return 0;
	} else {
		if (argc == 3 && strcmp(argv[1], "-i") == 0) {
			generate_keys(argv[2]);
			return 0;
		} else {
			if (argc == 3 && strcmp(argv[1], "-r") == 0) {
				change_nickname(argv[2]);
				return 0;
			} else {
				if (argc == 3 && strcmp(argv[1], "-l") == 0) {
					int socket = myListen(argv[2]);
					myChat(socket);
					return 0;
				} else {
					if (argc == 4 && strcmp(argv[1], "-c") == 0) {
						int socket = myConnect(argv[2], argv[3]);
						myChat(socket);
						return 0;
					} else {
						if (argc == 2 && strcmp(argv[1], "-a")) {
							/* Implement print all clients*/
						}
					}
				}

			}

		}
	}
	print_help();
	return 0;
}

void print_help() {
	printf(
			"%s",
			"Help:\n"
					"-l <port>\t\t Port to listen on for incoming connections.\n"
					"-c <hostname> <port>\t Connect to remote side using hostname and port.\n"
					"-i <nickname>\t\t Create keys for local user and use nickname\n"
					"-r <nickname>\t\t Change nickname of local user to <nickname>\n"
					"-a \t\t\t Show all known clients with their responding public-key");
}

void generate_keys(char* nickname) {
	sqlite3 *db;
	BN_CTX* bn_ctx = BN_CTX_new();
	BN_CTX_init(bn_ctx);
	BIGNUM* p = BN_generate_prime(NULL, 1024, 0, NULL, NULL, NULL, NULL);
	BIGNUM* q = BN_generate_prime(NULL, 1024, 0, NULL, NULL, NULL, NULL);
	BIGNUM* N = BN_new();
	BN_mul(N, p, q, bn_ctx);

	BIGNUM* pMinusOne = BN_CTX_get(bn_ctx);
	BN_sub(pMinusOne, p, BN_value_one());

	BIGNUM* qMinusOne = BN_CTX_get(bn_ctx);
	BN_sub(qMinusOne, q, BN_value_one());

	BIGNUM* eulerN = BN_CTX_get(bn_ctx);
	BN_mul(eulerN, pMinusOne, qMinusOne, bn_ctx);
	BIGNUM* e = BN_CTX_get(bn_ctx);
	BIGNUM* gcd = BN_CTX_get(bn_ctx);
	do {
		BN_generate_prime(e, 512, 0, NULL, NULL, NULL, NULL);
		BN_gcd(gcd, e, eulerN, bn_ctx);
	} while (!BN_is_one(gcd));

	BIGNUM* d = BN_CTX_get(bn_ctx);

	BN_mod_inverse(d, e, eulerN, bn_ctx);

	open_db(&db);
	printf("d: %s", BN_bn2dec(d));
	char* e_as_hex = BN_bn2hex(e);
	char* n_as_hex = BN_bn2hex(N);
	char* d_as_hex = BN_bn2hex(d);

	update_own_data(db, nickname, e_as_hex, n_as_hex, d_as_hex);
	sqlite3_close(db);
}

void change_nickname(char* newNick) {
	sqlite3** db;
	open_db(db);
	update_own_nickname(newNick, *db);
	sqlite3_close(*db);
}

int myListen(char* port_as_char) {
	int sock; /* socket to create */
	struct sockaddr_in echoServAddr; /* Local address */
	int port;
	struct sockaddr_storage their_addr;

	sscanf(port_as_char, "%i", &port);

	/* Create socket for incoming connections */
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	/* Zero out structure */
	echoServAddr.sin_family = AF_INET; /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	echoServAddr.sin_port = htons(port); /* Local port */

	/* Bind to the local address */
	if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(sock, 1) < 0)
		DieWithError("listen() failed");
	unsigned int addr_size = sizeof their_addr;
	int new_sock = accept(sock, (struct sockaddr *) &their_addr, &addr_size);
	return new_sock;

}
int myConnect(char* host, char* port_as_char) {
	int sock;
	struct sockaddr_in server;
	struct hostent *hp;
	int port;
	sscanf(port_as_char, "%i", &port);
	/* create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		exit(1);
	}
	server.sin_family = AF_INET;
	/* get internet address of host specified by command line */
	hp = gethostbyname(host);
	if (hp == NULL) {
		exit(2);
	}
	/* copies the internet address to server address */
	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	/* set port */
	server.sin_port = htons(port);
	if (connect(sock, &server, sizeof(struct sockaddr_in)) < 0) {
		exit(1);
	}
	return sock;
}

void myChat(int sock_nr) {
	int result;
	int end = 0;

	//Sende Key
	int sizeKeyE;
	BIGNUM* keyE;
	int sizeKeyN;
	BIGNUM* keyN;
	int sizeNick;
	char* nickName;
	BIGNUM* keyD;

	int remoteSizeKeyE;
	BIGNUM* remoteKeyE;
	int remoteSizeKeyN;
	BIGNUM* remoteKeyN;
	int remoteSizeNick;
	char* remoteNickName;

	fd_set readfds;
	int fd;
	char* kb_msg, msg = NULL;

	sqlite3* db;
	open_db(&db);
	get_own_data(&db, &nickName, &keyE, &keyN, &keyD);

	//TODO: Byte order convertieren!
	send(sock_nr, &sizeKeyE, sizeof(sizeKeyE), 0);
	send(sock_nr, keyE, sizeof(keyE), 0);
	send(sock_nr, &sizeKeyN, sizeof(sizeKeyN), 0);
	send(sock_nr, keyN, sizeof(keyN), 0);
	send(sock_nr, &sizeNick, sizeof(sizeNick), 0);
	send(sock_nr, nickName, strlen(nickName), 0);

	recv(sock_nr, &remoteSizeKeyE, 4, 0);
	recv(sock_nr, remoteKeyE, remoteSizeKeyE, 0);
	recv(sock_nr, &remoteSizeKeyN, 4, 0);
	recv(sock_nr, remoteKeyN, remoteSizeKeyN, 0);
	recv(sock_nr, &remoteSizeNick, 4, 0);
	recv(sock_nr, remoteNickName, remoteSizeNick, 0);

	result = verify_contact(db, remoteNickName, BN_bn2hex(remoteKeyE),
			BN_bn2hex(remoteKeyN));
	switch (result) {
	case NEW_CONTACT:
		printf("New contact: %s\n", remoteNickName);
		insert_new_contact(db, remoteNickName, BN_bn2hex(remoteKeyE),
				BN_bn2hex(remoteKeyN));
		break;
	case NOT_VERIFIED:
		printf("Public key doesn't match stored public key");
		exit(0);
		break;
	};

	/*Event loop roughly taken from http://dejant.blogspot.com/2007/08/chat-program-in-c.html*/
	while (end == 0) {
		int lenght_msg = 0;
		FD_ZERO(&readfds);
		FD_SET(sock_nr, &readfds);
		FD_SET(0, &readfds);
		/* Add keyboard to file descriptor set */

		if (FD_ISSET(fd,&readfds)) {

			if (fd == sock_nr) { /*Accept data from open socket */

				//printf("client - read\n");

				//read data from open socket
				char* buffer;

				recv(sock_nr, &lenght_msg, 4, 0);
				lenght_msg = ntohl(lenght_msg);
				buffer = malloc(lenght_msg);
				recv(sock_nr, buffer, lenght_msg, 0);
				char* plain_text = decrypt_msg(buffer, remoteKeyE, remoteKeyN);
				printf("%s", plain_text);
			}

			else if (fd == 0) {
				/*process keyboard activiy*/

				// printf("client - send\n");
				fgets(kb_msg, MSG_SIZE + 1, STDIN_FILENO);

				encrypt_msg(kb_msg, keyN, keyD);
				send(sock_nr, htonl(sizeof kb_msg), 4, 0);
				//TODO: htonl
				send(sock_nr, kb_msg, sizeof kb_msg, 0);

				printf("%s %s\n", nickName, kb_msg);

				if (strcmp(kb_msg, "quit\n") == 0) {

					sprintf(msg, "XClient is shutting down.\n");

					write(sock_nr, msg, strlen(msg));

					close(sock_nr); //close the current client

					exit(0); //end program

				}

				else {

					/* sprintf(kb_msg,"%s",alias);

					 msg[result]='\0';

					 strcat(kb_msg,msg+1);*/

					sprintf(msg, "M%s", kb_msg);

					write(sock_nr, msg, strlen(msg));

				}

			}

		}

	}
}

void encrypt_msg(char* message, BIGNUM* n, BIGNUM* d) {

}

char* decrypt_msg(char* cipher, BIGNUM* e, BIGNUM* n) {
	return NULL;
}

void DieWithError(char* string) {
	printf("%s", string);
}
