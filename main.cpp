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

//    cout << "Received query: " << query << endl;

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

tuple<int,string,double,string> labelInfo(const string& label, const int mode) {
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
    string currency;
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
            currency = (const char*)sqlite3_column_text(stmt, 2);
        } else {
            cout << "["  << sqlite3_column_int(stmt,0) <<  "] " << (const char*)sqlite3_column_text(stmt, 1) << " -> ";
            cout << "Balance: " << sqlite3_column_double(stmt, 3) << " " << sqlite3_column_text(stmt,2) << endl;
        }
    }
    sqlite3_finalize(stmt);
    return {id,title,balance,currency};
}

int deposit(double amount, char *label, const string& description) {

    string dateTime = currentDateTime();
    auto [id,labelTitle,balance,currency] = labelInfo(label, 1);
    double total = balance + amount;

    if (description.empty()) {

        auto query = "INSERT INTO transactions (label_id, action ,amount, totalAfter, datetime) VALUES (" + to_string(id) + ", 0 ," + to_string(amount) + ", " + to_string(total) + ", '" + dateTime + "')";

        string result = runQuery(query);

        if (result == "Success") {

            auto query2 = "UPDATE label SET balance = " + to_string(total) + " WHERE id = " + to_string(id);
            string result2 = runQuery(query2);

            if (result2 == "Success") {
                cout << "\nDeposit successful" << endl;
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
                cout << "\nDeposit successful" << endl;
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

int withdraw(double amount, char *label, const string& description) {

    string dateTime = currentDateTime();
    auto [id,labelTitle,balance,currency] = labelInfo(label, 1);
    double total = balance - amount;

    if (description.empty()) {

        auto query = "INSERT INTO transactions (label_id, action ,amount, totalAfter, datetime) VALUES (" + to_string(id) + ", 1 ," + to_string(amount) + ", " + to_string(total) + ", '" + dateTime + "')";

        string result = runQuery(query);

        if (result == "Success") {

            auto query2 = "UPDATE label SET balance = " + to_string(total) + " WHERE id = " + to_string(id);
            string result2 = runQuery(query2);

            if (result2 == "Success") {
                cout << "\nWithdraw successful" << endl;
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

        auto query = "INSERT INTO transactions (label_id, action ,amount, totalAfter, datetime, description) VALUES (" + to_string(id) + ", 1 ," + to_string(amount) + ", " + to_string(total) + ", '" + dateTime + "', '" + description + "')";

        string result = runQuery(query);

        if (result == "Success") {

            auto query2 = "UPDATE label SET balance = " + to_string(total) + " WHERE id = " + to_string(id);
            string result2 = runQuery(query2);

            if (result2 == "Success") {
                cout << "\nWithdraw successful" << endl;
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

    auto result = runQuery(query);

    if (result == "Success") {
        cout << "Label created successfully" << endl;
        return 0;
    } else {
        cout << "\nError: " << result;
        return 1;
    }
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

    auto [id,labelTitle,balance,currency] = labelInfo(to_string(labelID), 0);

    if (action == 0) { //Deposit
        query = "UPDATE label SET balance = " + to_string(balance - amount) + " WHERE id = " + to_string(labelID);
        string result = runQuery(query);
        if (result == "Success") {
            auto query2 = "DELETE FROM transactions WHERE id = " + to_string(transactionID);
            string result2 = runQuery(query2);
            if (result2 == "Success") {
                cout << "\nTransaction undone successfully.";
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
                cout << "\nTransaction undone successfully.";
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

int rm(int mode, int id){
    // mode = 0 -> label, mode = 1 -> transaction
    if (mode == 0){
        auto query = "DELETE FROM label WHERE id = " + to_string(id);
        string result = runQuery(query);
        if (result == "Success") {
            cout << "Deleted Label" << endl;
            return 0;
        } else {
            cout << "\nError: " << result;
            return 1;
        }
    } else if (mode == 1) {
        auto query = "DELETE FROM transactions WHERE id = " + to_string(id);
        string result = runQuery(query);
        if (result == "Success") {
            cout << "Deleted Transaction" << endl;
            return 0;
        } else {
            cout << "\nError: " << result;
            return 1;
        }
    } else{
        cout << "\nError: Invalid mode";
        return 1;
    }
}

int ls(int mode, const string& labelTitle){
    // mode 0 -> list all labels, 1 -> list all transactions of a label

    openDatabase();
    string query;

    if (mode == 0 && labelTitle.empty()){
        query = "SELECT * FROM transactions INNER JOIN label ON transactions.label_id = label.id ";
//        cout << query;
    } else if (mode == 1 && !labelTitle.empty()){
        query = "SELECT * FROM transactions INNER JOIN label ON transactions.label_id = label.id WHERE label.id = (SELECT id FROM label WHERE labelTitle = '" + labelTitle + "')";
//        cout << query;
    } else {
        cout << "Error: Invalid arguments" << endl;
        return 1;
    }

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, query.c_str(), query.length(), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        const char *errMsg;
        errMsg = sqlite3_errmsg(db);
        cout << errMsg << endl;
        sqlite3_free(zErrMsg);
    }
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {


        cout << "[" << sqlite3_column_int(stmt, 0) << "] ";

        if (mode == 0){
            cout << "| " << sqlite3_column_text(stmt,8) << " | ";
        }

        if(sqlite3_column_int(stmt, 2) == 0){
            cout << "Deposit: ";
        } else {
            cout << "Withdraw: ";
        }
        cout << sqlite3_column_double(stmt, 3) << " (" << sqlite3_column_double(stmt,4 ) << ") | " << sqlite3_column_text(stmt, 5) << " | ";

        if (sqlite3_column_text(stmt, 6) != nullptr){
            cout << sqlite3_column_text(stmt, 6) << " |";
        } else {
            cout << "";
        }

        cout << endl;

    }
    sqlite3_finalize(stmt);


}

int main(int argc, char** argv) {

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
                withdraw(stod(argv[2]), argv[3], description);
            } else if (option == 'n') {
                withdraw(stod(argv[2]), argv[3], description);
            }

        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    } else if (action == "new") {
        try {
            newLabel(argv[2], argv[3], stod(argv[4]));
        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    } else if (action == "undo") {
        try {
            undo();
        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    } else if (action == "bal" && argc == 3) {
        try {
            auto [id, labelTitle, balance, currency] = labelInfo(argv[2], 1);
            cout << "Label: " << argv[2] << " -> Balance: " << balance << " " << currency;
        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }

    } else if (action == "bal" && argc == 2) {
        try {
            labelInfo("", 2);
        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    } else if (action == "ls" && argc == 3) {
        try {
            ls(1, argv[2]);
        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    } else if (action == "ls" && argc == 2) {
        try {
            ls(0, "");
        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    } else if (action == "rm") {
        try {
            string target = argv[2];
            if (target == "label") {
                rm(0,stoi(argv[3]));
            } else if (target == "transaction") {
                rm(1, stoi(argv[3]));
            }
        } catch (const exception& e) {
            cout << "Error: " << e.what() << endl;
        }

    } else if (action == "conv") {

    } else {
        cout << "Error: Invalid arguments" << endl;
    }

    return 0;
}


