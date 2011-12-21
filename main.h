/*
 * main.h
 *
 *  Created on: 27.11.2011
 *      Author: steven
 */

#ifndef MAIN_H_
#define MAIN_H_
#define MSG_SIZE 500

void print_help();
void generate_keys(char* nickName);
void change_nickname(char* newNick);
int myListen(char*);
int myConnect(char* host, char* port);
void myChat(int sock_nr);

void DieWithError(char* string);
void _generate_keys(BIGNUM** newE, BIGNUM** newD, BIGNUM** newN);

char* encrypt_msg(char* message, BIGNUM* n, BIGNUM* d);
char* decrypt_msg(char* cipher, int cipher_len, BIGNUM* e, BIGNUM* n);

#endif /* MAIN_H_ */
