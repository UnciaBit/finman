# About this project

Command-line finance manager built using C++

# Build

```bash

$ git clone https://github.com/UnciaBit/finman.git
$ cd finman
$ mkdir build
$ cd build
$ cmake ../
$ cmake --build ./

```

Then add the executable to PATH environment variable

# Usage

Create labels (categories) to track different type of transactions.

## Create New Label (Space is not allowed)

```bash
$ finman new [label name] [currency] [initial balance]

// Example

$ finman new JPMorgan USD 0
$ finman new SMBC JPY 1000

or 

$ finman new utility AUD 0

```

## Deposit

```bash

$ finman add [amount] [label name]

// Example

$ finman add 150 JPMorgan
Add description? (y/n): y
Enter Description: Bonus
Deposit Successful

```

## Withdraw

```bash

$ finman sub [amount] [label name]

// Example

$ finman sub 200 JPMorgan
Add description? (y/n): n
Withdraw Successful

```

## Undo last transaction

``` bash
$ finman undo
```


## List transactions

### List all transactions across all labels

```bash
$ finman ls
```

### List transactions from a specific label

```bash
$ finman ls [label name]

// Example

$ finman ls JPMorgan

```

## Print Balance

### Print balance of all labels

```bash
$ finman bal
```

### Print balance of a specific label

```bash
$ finman bal [label name]

// Example

$ finman bal JPMorgan
```

## Remove Transaction

_Get Transaction ID and Label ID from ls function and bal function_

```
$ finman rm transaction [Transcation ID]
```

## Remove Label

```
$ finman rm label [Label ID]
```
