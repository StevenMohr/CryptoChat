/*
 * main.h
 *
 *  Created on: 27.11.2011
 *      Author: steven
 */

#ifndef MAIN_H_
#define MAIN_H_

void print_help();
void generate_keys(char* nickName);
void change_nickname(char* newNick);
int myListen(char*);
int myConnect(char* host, char* port);
void myChat(int sock_nr);

void DieWithError(char* string);
void extended_euclid(BIGNUM* a, BIGNUM* b, BIGNUM* d, BIGNUM* s, BIGNUM* t);

#endif /* MAIN_H_ */
