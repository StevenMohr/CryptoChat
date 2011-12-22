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

unsigned char* encrypt_msg(unsigned char* message, BIGNUM* n, BIGNUM* d, int* cipher_length);
unsigned char* decrypt_msg(unsigned char* cipher, int cipher_len, BIGNUM* e, BIGNUM* n);

#endif /* MAIN_H_ */
