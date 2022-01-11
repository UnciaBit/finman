#include <iostream>

using namespace std;

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



    cout << argc << endl;
    cout << argv[1] << endl;

    return 0;
}
