#include <iostream>
#include <string>
#include <iomanip>
#include <ctime>
#include "lib/sqlite3.h"
#include "config.h"

using namespace std;

sqlite3 *db;
char *zErrMsg = 0;
int rc;

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    for(i = 0; i<argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int openDatabase(){

    rc = sqlite3_open("test.db", &db);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {
//        fprintf(stderr, "Opened database successfully\n");
    }
    return 0;
}

int createTable(){
    int rc2;
    char *labelQuery;
    char *transactionQuery;

    openDatabase();

    // action: 0 -> deposit, 1 -> withdraw
    labelQuery = "CREATE TABLE IF NOT EXISTS label (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, labelTitle TEXT UNIQUE, currency TEXT, balance DOUBLE)";
    transactionQuery = "CREATE TABLE IF NOT EXISTS transactions (id INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, label_id INTEGER  REFERENCES label (id) ON UPDATE CASCADE NOT NULL, action INT NOT NULL, amount DOUBLE NOT NULL DEFAULT (0), totalAfter  DOUBLE NOT NULL, datetime DATETIME NOT NULL,description TEXT)";

    rc = sqlite3_exec(db, labelQuery, callback, nullptr, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
//        fprintf(stdout, "Label Table created successfully\n");
    }

    rc2 = sqlite3_exec(db, transactionQuery, callback, nullptr, &zErrMsg);
    if( rc2 != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
//        fprintf(stdout, "Transactions Table created successfully\n");
    }

    sqlite3_close(db);
    return 0;
}

string runQuery(const string& query){

    openDatabase();

    cout << "Received query: " << query << endl;

    char *sql = (char*)query.c_str();

    rc = sqlite3_exec(db, sql, callback, nullptr, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        const char *errMsg;
        errMsg = sqlite3_errmsg(db);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return errMsg;
    } else {
        sqlite3_close(db);
        return "Success";
    }

}

string currentDateTime(){
    auto t = time(nullptr);
    auto tm = *localtime(&t);

    ostringstream oss;
    oss << put_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto datetime = oss.str();

    return datetime;
}

tuple<int,string,double> labelInfo(const string& label, const int mode) {
    openDatabase();

    //mode: 0 -> search using label_id, 1 -> search using labelTitle, 2 -> search all labels
    string query;

    if (mode == 0) {
        query = "SELECT * FROM label WHERE id = " + label;
    } else if (mode == 1) {
        query = "SELECT * FROM label WHERE labelTitle = '" + label + "'";
    } else if (mode == 2) {
        query = "SELECT * FROM label";
    }

    double balance;
    int id;
    string title;

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, query.c_str(), query.length(), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        const char *errMsg;
        errMsg = sqlite3_errmsg(db);
        cout << errMsg << endl;
        sqlite3_free(zErrMsg);
    }
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (mode != 2){
            balance = sqlite3_column_double(stmt, 3);
            title = (const char*)sqlite3_column_text(stmt, 1);
            id = sqlite3_column_int(stmt, 0);
        } else {
            cout << "Label: " << (const char*)sqlite3_column_text(stmt, 1) << " -> ";
            cout << "Balance: " << sqlite3_column_double(stmt, 3) << " " << sqlite3_column_text(stmt,2) << endl;
        }
    }
    sqlite3_finalize(stmt);
    return {id,title,balance};
}

int deposit(double amount, char *label, const string& description) {

    string dateTime = currentDateTime();
    auto [id,labelTitle, balance] = labelInfo(label, 1);
    double total = balance + amount;

    if (description.empty()) {

        auto query = "INSERT INTO transactions (label_id, action ,amount, totalAfter, datetime) VALUES (" + to_string(id) + ", 0 ," + to_string(amount) + ", " + to_string(total) + ", '" + dateTime + "')";

        string result = runQuery(query);

        if (result == "Success") {

            auto query2 = "UPDATE label SET balance = " + to_string(total) + " WHERE id = " + to_string(id);
            string result2 = runQuery(query2);

            if (result2 == "Success") {
                return 0;
            } else {
                cout << "\nError: " << result2;
                return 1;
            }
        } else {
            cout << "\nError: " << result;
            return 1;
        }

    } else {

        auto query = "INSERT INTO transactions (label_id, action ,amount, totalAfter, datetime, description) VALUES (" + to_string(id) + ", 0 ," + to_string(amount) + ", " + to_string(total) + ", '" + dateTime + "', '" + description + "')";

        string result = runQuery(query);

        if (result == "Success") {

            auto query2 = "UPDATE label SET balance = " + to_string(total) + " WHERE id = " + to_string(id);
            string result2 = runQuery(query2);

            if (result2 == "Success") {
                return 0;
            } else {
                cout << "\nError: " << result2;
                return 1;
            }

        } else {
            cout << "\nError: " << result;
            return 1;
        }
    }
}

int newLabel(const string& labelTitle, const string& currency, double balance) {

    auto query = "INSERT INTO label (labelTitle, currency, balance) VALUES ('" + labelTitle + "', '" + currency + "', '" + to_string(balance) + "')";

    runQuery(query);

    return 0;
}

int undo(){

    openDatabase();

    string query = "SELECT * FROM transactions ORDER BY id DESC LIMIT 1";
    int transactionID;
    int labelID;
    int action;
    double amount;

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, query.c_str(), query.length(), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        const char *errMsg;
        errMsg = sqlite3_errmsg(db);
        cout << errMsg << endl;
        sqlite3_free(zErrMsg);
    }
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        transactionID = sqlite3_column_int(stmt, 0);
        labelID = sqlite3_column_int(stmt, 1);
        action = sqlite3_column_int(stmt, 2);
        amount = sqlite3_column_double(stmt, 3);
    }

    auto [id,labelTitle, balance] = labelInfo(to_string(labelID), 0);

    if (action == 0) { //Deposit
        query = "UPDATE label SET balance = " + to_string(balance - amount) + " WHERE id = " + to_string(labelID);
        string result = runQuery(query);
        if (result == "Success") {
            auto query2 = "DELETE FROM transactions WHERE id = " + to_string(transactionID);
            string result2 = runQuery(query2);
            if (result2 == "Success") {
                sqlite3_finalize(stmt);
                return 0;
            } else {
                cout << "\nError: " << result2;
                return 1;
            }
        } else {
            cout << "\nError: " << result;
            return 1;
        }
    } else { //Withdraw
        query = "UPDATE label SET balance = " + to_string(balance + amount) + " WHERE id = " + to_string(labelID);
        string result = runQuery(query);
        if (result == "Success") {
            auto query2 = "DELETE FROM transactions WHERE id = " + to_string(transactionID);
            string result2 = runQuery(query2);
            if (result2 == "Success") {
                sqlite3_finalize(stmt);
                return 0;
            } else {
                cout << "\nError: " << result2;
                return 1;
            }
        } else {
            cout << "\nError: " << result;
            return 1;
        }
    }
}


int main(int argc, char** argv) {
    // Command line budget calculator

    // finman <add|sub|new|undo> <number> <label>

    // finman new <label> <currency>

    // [add] finman add 500 smbc -> adds value 500 to label smbc
        // Add description? (y/n)
    // [sub] finman sub 500 ufj -> subtracts value 500 from label ufj
    // [new] finman new commbank USD 0 -> creates new label 'commbank' with initial balance of 0
    // [bal1] finman bal ufj -> prints balance of label ufj with currency
    // [bal2] finman bal -> prints all balances

    // [undo] finman undo -> undo last transaction

    // [conv] finman conv 500 JPY USD -> converts 500 JPY to USD
    // [conv] finman conv ufj USD -> converts ufj balance to USD

    createTable();

    string action = argv[1];

    if (action == "add") {
        try {
            string description;
            char option = 0;
            while (option != 'y' && option != 'n') {
                cout << "Add description? (y/n): ";
                cin >> option;
            }
            if (option == 'y') {
                cout << "Enter Description: ";
                cin.ignore(1, '\n');
                getline(cin, description);
                deposit(stod(argv[2]), argv[3], description);
            } else if (option == 'n') {
                deposit(stod(argv[2]), argv[3], description);
            }

        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    } else if (action == "sub") {
        cout << "sub" << endl;
    } else if (action == "new") {
        newLabel(argv[2], argv[3], stod(argv[4]));
        cout << "new" << endl;
    } else if (action == "undo") {
        undo();
    } else if (action == "bal" && argc == 3) {
        auto [id, labelTitle, balance] = labelInfo(argv[2], 1);
        cout << balance;
    } else if (action == "bal" && argc == 2) {
        // Print all balances
        labelInfo("", 2);
    } else {
        cout << "Invalid action" << endl;
    }


    return 0;
}


