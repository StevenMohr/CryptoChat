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
#include <math.h>

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
						if (argc == 2 && strcmp(argv[1], "-a") == 0) {
							/* Implement print all clients*/
						} else {
							if (argc == 3 && strcmp(argv[1], "-t") == 0) {
								char* msg = argv[2];
								int cipher_len;
								BIGNUM *e = NULL, *N = NULL, *d = NULL;
								printf("Original Plain text: %s \n", msg);
								_generate_keys(&e, &d, &N);
								char* cipher = encrypt_msg(msg, e, N,
										&cipher_len);
								char* plaintext = decrypt_msg(cipher,
										cipher_len, d, N);
								printf("Decrypted plain text: %s \n",
										plaintext);
								return 0;
							}
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
					"-a \t\t\t Show all known clients with their responding public-key\n"
					"-t <test_msg>\t\t Test run of the RSA implementation. Generates keys and encrypts and decrypts a test message");
}

void _generate_keys(BIGNUM** newE, BIGNUM** newD, BIGNUM** newN) {
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
	*newD = d;
	*newE = e;
	*newN = N;
}

void generate_keys(char* nickname) {
	sqlite3 *db;
	open_db(&db);
	BIGNUM *e, *N, *d;
	_generate_keys(&e, &d, &N);

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
	struct hostent *hostinfo;
	int port;
	sscanf(port_as_char, "%i", &port);
	/* create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);

	/* Name the socket, as agreed with the server */
	hostinfo = gethostbyname(host); /* look for host's name */
	server.sin_addr = *(struct in_addr *) *hostinfo->h_addr_list;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_in))
			< 0) {
		exit(1);
	}
	return sock;
}

void myChat(int sock_nr) {
	BN_CTX* bn_ctx = BN_CTX_new();
	BN_CTX_init(bn_ctx);
	int result = 0;
	int end = 0;

	//Sende Key
	BIGNUM* keyE = BN_CTX_get(bn_ctx);
	int sizeKeyE = 0;
	int sizeKeyN = 0;
	BIGNUM* keyN = BN_CTX_get(bn_ctx);
	char* nickName = NULL;

	BIGNUM* keyD = BN_CTX_get(bn_ctx);

	fd_set readfds;
	int fd;

	//Retrieving own nickname, e and n from database.
	sqlite3* db;
	open_db(&db);
	get_own_data(db, &nickName, keyE, keyN, keyD);
	int sizeNick = strlen(nickName);

	//Converting e und n to big-endian binary form for network transmission
	sizeKeyE = BN_num_bytes(keyE);
	unsigned char* binaryKeyE = malloc(sizeKeyE);
	BN_bn2bin(keyE, binaryKeyE);

	sizeKeyN = BN_num_bytes(keyN);
	unsigned char* binaryKeyN = malloc(sizeKeyN);
	BN_bn2bin(keyN, binaryKeyN);
	int nl_sizeKeyE = htonl(sizeKeyE);
	int nl_sizeKeyN = htonl(sizeKeyN);
	int nl_sizeNick = htonl(sizeNick);

	send(sock_nr, &nl_sizeKeyE, sizeof(nl_sizeKeyE), 0);
	send(sock_nr, binaryKeyE, sizeKeyE, 0);
	send(sock_nr, &nl_sizeKeyN, sizeof(nl_sizeKeyN), 0);
	send(sock_nr, binaryKeyN, sizeKeyN, 0);
	send(sock_nr, &nl_sizeNick, sizeof(sizeNick), 0);
	send(sock_nr, nickName, sizeNick, 0);

	int remoteSizeKeyE = 0;
	BIGNUM* remoteKeyE = NULL;
	int remoteSizeKeyN = 0;
	BIGNUM* remoteKeyN = NULL;
	int remoteSizeNick = 0;
	char* remoteNickName = NULL;
	unsigned char* binaryRemoteE = NULL;
	unsigned char* binaryRemoteN = NULL;

	// Reading size of remote key e
	int bytesToRead = sizeof remoteSizeKeyE;
	while (bytesToRead > 0) {
		bytesToRead -= recv(sock_nr,
				&remoteSizeKeyE + (sizeof remoteSizeKeyE - bytesToRead),
				bytesToRead, 0);
	}
	remoteSizeKeyE = ntohl(remoteSizeKeyE);

	//Reading remote key e
	binaryRemoteE = malloc(remoteSizeKeyE);
	memset(binaryRemoteE, 0, remoteSizeKeyE);
	bytesToRead = remoteSizeKeyE;
	recv(sock_nr, binaryRemoteE, remoteSizeKeyE, 0);
	remoteKeyE = BN_bin2bn(binaryRemoteE, remoteSizeKeyE, NULL);

	fflush(stdout);
	recv(sock_nr, &remoteSizeKeyN, sizeof remoteSizeKeyN, 0);
	remoteSizeKeyN = ntohl(remoteSizeKeyN);
	binaryRemoteN = malloc(remoteSizeKeyN);
	recv(sock_nr, binaryRemoteN, remoteSizeKeyN, 0);

	remoteKeyN = BN_bin2bn(binaryRemoteN, remoteSizeKeyN, NULL);
	recv(sock_nr, &remoteSizeNick, sizeof remoteSizeNick, 0);
	remoteSizeNick = ntohl(remoteSizeNick);
	remoteNickName = malloc(remoteSizeNick);
	recv(sock_nr, remoteNickName, remoteSizeNick, 0);

	free(binaryKeyE);
	free(binaryKeyN);
	free(binaryRemoteE);
	free(binaryRemoteN);

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

	fd_set testfds, clientfds;
	char msg[MSG_SIZE + 1];
	char kb_msg[MSG_SIZE + 10];

	/*Client variables=======================*/
	int sockfd = sock_nr;

	FD_ZERO(&clientfds);
	FD_SET(sockfd, &clientfds);
	FD_SET(0, &clientfds);
	/*Event loop roughly taken from http://dejant.blogspot.com/2007/08/chat-program-in-c.html*/
	while (end == 0) {
		testfds = clientfds;
		select(FD_SETSIZE, &testfds, NULL, NULL, NULL);

		for (fd = 0; fd < FD_SETSIZE; fd++) {
			if (FD_ISSET(fd,&testfds)) {
				if (fd == sockfd) { /*Accept data from open socket */

					//read data from open socket
					int msg_len;
					recv(sock_nr, &msg_len, sizeof msg_len, 0);
					msg_len = ntohl(msg_len);
					//char* buffer = malloc(msg_len);
					int readBytes = recv(sock_nr, msg, msg_len, 0);
					if (readBytes > 0) {
						char* plaintext;
						plaintext = decrypt_msg(msg, msg_len, keyD, keyN);
						printf("%s: %s", remoteNickName, plaintext);
						fflush(stdout);
					}

				} else if (fd == 0) { /*process keyboard activiy*/
					fgets(kb_msg, MSG_SIZE + 1, stdin);
					if (strcmp(kb_msg, "quit\n") == 0) {
						sprintf(msg, "XClient is shutting down.\n");
						//write(sockfd, msg, strlen(msg));
						close(sockfd); //close the current client
						exit(0); //end program
					} else {
						int msg_len = 0;
						char* cipher = encrypt_msg(kb_msg, remoteKeyE,
								remoteKeyN, &msg_len); //TODO: Check it!
						msg_len = htonl(msg_len);
						send(sock_nr, &msg_len, sizeof msg_len, 0);
						send(sock_nr, cipher, msg_len, 0);
					}
				}
			}
		}
	}
}

char* encrypt_msg(char* message, BIGNUM* e, BIGNUM* n, int* cipher_len) {
	BN_CTX* bn_ctx = BN_CTX_new();
	BN_CTX_init(bn_ctx);
	int chunk_size = BN_num_bytes(n);
	const int length_msg = strlen(message);
	const int num_chunks = ceil(((double) length_msg) / chunk_size);
	char* buffer = malloc(num_chunks * chunk_size);
	*cipher_len = num_chunks * chunk_size;

	int i;
	for (i = 0; i < num_chunks; i++) {
		BIGNUM* plaintext_chunk = BN_CTX_get(bn_ctx);
		plaintext_chunk = BN_bin2bn(message + i * chunk_size, chunk_size, NULL);
		BIGNUM* cipher = BN_CTX_get(bn_ctx);
		BN_mod_exp(cipher, plaintext_chunk, e, n, bn_ctx);
		BN_bn2bin(cipher, &buffer[i * chunk_size]);
	}
	return buffer;

}

char* decrypt_msg(char* cipher, int cipher_len, BIGNUM* d, BIGNUM* n) {
	BN_CTX* bn_ctx = BN_CTX_new();
	BN_CTX_init(bn_ctx);
	int chunk_size = BN_num_bytes(n);
	const int num_chunks = ceil(((double) cipher_len) / chunk_size);
	char* buffer = malloc(num_chunks * chunk_size);
	int i;
	for (i = 0; i < num_chunks; i++) {
		BIGNUM* chunk = BN_bin2bn(cipher + (i * chunk_size), chunk_size, NULL);
		BIGNUM* plain_text = BN_CTX_get(bn_ctx);
		BN_mod_exp(plain_text, chunk, d, n, bn_ctx);
		BN_bn2bin(plain_text, &buffer[i * chunk_size]);
	}
	return buffer;
}

void DieWithError(char* string) {
	printf("%s", string);
}
