#include <iostream>
#include <string>
#include "lib/sqlite3.h"
#include "config.h"

using namespace std;

char queryString;

int addBalance(double amount, char *label) {
    cout << "amount: " << amount;
    cout << "string: " << label;



    return 0;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    for(i = 0; i<argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int runQuery(char query){
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char *sql;

    rc = sqlite3_open("test.db", &db);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    /* Create SQL statement */
    sql = "CREATE TABLE IF NOT EXISTS BALANCE " \
          "(ID INTEGER PRIMARY KEY     AUTOINCREMENT," \
          " BALANCE           REAL    NOT NULL," \
          " LABEL             TEXT    NOT NULL);";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }

    sqlite3_close(db);
    return 0;
}

int main(int argc, char** argv) {
    // Command line budget calculator

    // finman <add|sub|new|undo> <number> <label>

    // finman new <label> <currency>

    // [add] finman add 500 smbc -> adds value 500 to label smbc
        // Add description? (y/n)
    // [sub] finman sub 500 ufj -> subtracts value 500 from label ufj
    // [new] finman new commbank USD -> creates new label 'commbank'
    // [bal1] finman bal ufj -> prints balance of label ufj with currency
    // [bal2] finman bal -> prints all balances

    // [undo] finman undo -> undo last transaction

    string action = argv[1];

    if (action == "add") {
        try {
            addBalance(stod(argv[2]), argv[3]);
        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    } else if (action == "sub") {
        cout << "sub" << endl;
    } else if (action == "new") {
        cout << "new" << endl;
    } else if (action == "undo") {
        cout << "undo" << endl;
    } else {
        cout << "Invalid action" << endl;
    }

    return 0;
}


