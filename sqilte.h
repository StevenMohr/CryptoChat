/*
 * sqilte.h
 *
 *  Created on: 01.12.2011
 *      Author: steven
 */

#ifndef SQILTE_H_
#define SQILTE_H_

#define NEW_CONTACT 1
#define NOT_VERIFIED 2

#include <openssl/bn.h>

void open_db(sqlite3 **ppDb);
int verify_contact(sqlite3* db, char* nickname, char* e, char* n);
void insert_new_contact(sqlite3* db, char* nickname, char* e, char* n);
void update_own_data(sqlite3* db, char* nickname, char* e, char* n, char* d);
void update_own_nickname(char* new_nick, sqlite3* db);
void get_own_data(sqlite3* db, char** nick_name, BIGNUM* e, BIGNUM* n, BIGNUM* d);
#endif /* SQILTE_H_ */
