/*
 * sqlite.c
 *
 *  Created on: 01.12.2011
 *      Author: steven
 */
#import <sqlite3.h>
#import <stdio.h>
#import "sqilte.h"

void open_db(sqlite3** db) {
	int retval;
	char create_table[1000] =
			"CREATE TABLE IF NOT EXISTS keys (id INTEGER PRIMARY KEY AUTOINCREMENT, nick TEXT NOT NULL, e TEXT NOT NULL, n TEXT NOT NULL, d TEXT);";
	sqlite3_open(".chatstore", db);
	retval = sqlite3_exec(*db, create_table, 0, 0, 0);
}

void update_own_nickname(char* new_nick, sqlite3* db) {
	char* statement;
	int retval;
	sprintf(statement, "UPDATE keys SET nick='%s' where id=0", new_nick);
	retval = sqlite3_exec(db, statement, 0, 0, 0);
}

void update_own_data(sqlite3* db, char* nickname, char* e, char* n, char* d) {
	int retval;
	sqlite3_stmt *stmt;
	char statement[10000];
	char* query = "SELECT * from keys where id=0";
	retval = sqlite3_prepare_v2(&db, query, -1, &stmt, 0);

	// Read the number of rows fetched
	int cols = sqlite3_column_count(stmt);
	if (cols > 1) {

		sprintf(
				statement,
				"UPDATE keys SET nick='%s', e='%s', n='%s', d='%s' where id=0", nickname, e, n, d);
		sqlite3_exec(db, statement, 0, 0, 0);
	} else {
		sprintf(
				statement,
				"INSERT INTO keys VALUES (0, '%s', '%s', '%s', '%s')", nickname, e, n, d);
		sqlite3_exec(db, statement, 0, 0, 0);
	}
}

void insert_new_contact(sqlite3* db, char* nickname, char* e, char* n) {
	int retval;
	sqlite3_stmt *stmt;
	char* statement;
	sprintf(statement,
			"INSERT INTO keys VALUES (0, '%s', '%s', '%s')", nickname, e, n);
	retval = sqlite3_exec(db, statement, 0, 0, 0);
}

int verify_contact(sqlite3* db, char* nickname, char* e, char* n) {
	int retval, cols;
	sqlite3_stmt *stmt;
	char* statement;
	sprintf(statement, "SELECT * FROM keys WHERE nick = '%s'", nickname);
	retval = sqlite3_exec(db, statement, 0, 0, 0);
	cols = sqlite3_column_count(stmt);
	if (cols < 2) {
		return NEW_CONTACT;
	} else {
		sprintf(
				statement,
				"SELECT * FROM keys WHERE nick = '%s' AND e = '%s' AND n ='%s'", nickname, e, n);
		retval = sqlite3_prepare_v2(db, statement,  -1, stmt, statement);
		int cols = sqlite3_column_count(stmt);
		if (cols < 2) {
			return NOT_VERIFIED;
		} else {
			return 0;
		}
	}
}

void get_own_data(sqlite3* db, char* nickname, BIGNUM* e, BIGNUM* n, BIGNUM* d) {
	int retval, cols;
	sqlite3_stmt *stmt;
	const char* statement = "SELECT * FROM keys WHERE id = 1";
	sqlite3_prepare_v2(&db, statement, -1, &stmt,0);
	nickname = sqlite3_column_text(stmt, 1);
	BN_hex2bn(&e, sqlite3_column_text(stmt, 2));
	BN_hex2bn(&n, sqlite3_column_text(stmt, 3));
	BN_hex2bn(&d, sqlite3_column_text(stmt, 4));
}
