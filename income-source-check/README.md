# Income source check

## Introduction

In this example, we present the statement for solving the following problem. An institution (some non-governmental organization) wants to prove its partners its financial reliability, i.e., that it is not financially dependent on income from suspicious sources. The partners consider income from all countries in some blacklist as suspicious and therefore undesirable. On the other hand, the partners have a list of welcomed countries and expect at least a certain fraction of the institutions income to originate from these countries. To verify the grassroots nature of the institution, the partners might also be interested that at most a certain percentage of income were obtained by "big" transactions whose value exceeds a certain limit.

The institution can procure a statement of its bank account, digitally signed by the bank, which shows that its income meets the required criteria. However, the institution does not want to reveal all details of the account statement to its partners because it may contain sensitive data about the institution itself and its clients. Hence the institution and its partners agree to use ZK proof that enables the institution to convince its partners of meeting the income percentage criteria while not revealing any data about its transactions, total amount of money involved etc.

## The statement

### Inputs

#### Prover's inputs

In this scenario, the institution is the prover. Its account statement is its private input. The account statement comes from a bank, and is formatted in some standard manner. In the current example, we let the format be [CSV](https://www.seb.ee/en/business/daily-banking/tools-and-online-services/description-account-statement-csv-format). The statement that is to be proven in ZK, inputs the account statement as a list of characters, where each character is a number between 0 and 255.

Another private input of the prover should be the bank's signature on the account statement. The verification of the signature should take place as part of the statement that is to be proven in ZK. The standard library of ZK-SecreC contains methods that are useful for verifying ECDSA signatures under ZK. We have decided to omit this part from the current example.

#### Verifier's inputs

In this scenario, the verifiers are the partners of the institution. Their input is the public key of the bank, used to sign the account statement. But as we discussed above, signature verification is not a part of the current example.

#### Public inputs

The public inputs describe the columns of the account statement (which is a CSV file). They also give the parameters of the checks that the example statement performs. We include the following public inputs in our example.


* Number of columns in the account statement;
* Indices of columns of important data (client's account, date, beneficiary's account, type of the transaction, amount) in the account statement;
* Bank account of the institution (for checking that the statement describes the right client's account indeed);
* An upper bound of the length of the amount;
* The list of two-character codes of bad countries (as used in International Bank Account Numbers (IBAN));
* The list of two-character codes of good countries;
* The minimal value of ``big'' transaction;
* The required minimum percentage of income from good countries;
* The allowed maximum percentage of income from bad countries;
* The allowed maximum percentage of income obtained by ``big'' transactions;
* The first date of the period of interest;
* The last date of the period of interest;

The size of the account statement (or at least some upper bound on it) also has to be public. In order to simplify the parsing of the account statement, we have decided to include the following parameters among the public inputs of the statement.

* The number of transactions (i.e. lines) in the account statement;
* The lengths of lines in the account statement;
* The total number of characters in the account statement.

### Checks and Computations

The statement that is to be proven in ZK, consists of the following checks.

* The lengths of lines are correct (i.e., if the whole account statement is split into parts with these lengths then intercalating newline characters between the obtained parts would restore the original statement);
* The client field of every subsequent row coincides with the bank account of the institution;
* Among the income that falls within the interesting time period, at least the required minimum percentage originates from good countries;
* Among the income that falls within the interesting time period, at most the allowed maximum percentage originates from bad countries;
* Among the income that falls within the interesting time period, at most the allowed maximum percentage has been received by "big" transactions.

However, these checks are not the main complexity of the statement. The main complexity is parsing the CSV file and reading from it the parameters of the transactions. As we have made the lengths of the lines in the account statement, a parsing operation only has to process a single line at a time. The parsing consists of locating the column separators, assigning each position in the line to one of the columns, and translating numeric data from textual to numeric representations.

## Implementation

### JSON objects

In the subdirectory `input`, we have included three possible private inputs of different size. They are located in files `witness0.json`, `witness1.json`, and `witness2.json`. The corresponding public parameters are located in files `public0?.json`, `public1?.json` and `public2?.json` of the same subdirectory, where different files in the same family differ by the number of good and bad countries. Recall that there is no "instance": inputs that are known both to the prover and the verifier, but are not public parameters.

### ZK-SecreC source programs

In the subdirectory `src`, we have included our implementation of the described relation in ZK-SecreC. The implementation contains two files, `Commas.zksc` and `Countries.zksc`, with auxiliary functionality related to locating the column separators, and parsing the country codes. It also contains four variants of the implementation of the actual statement, in files `bank-account_*.zksc`, differing on how the membership in the sets of good or bad countries is decided. The implementation (regardless of the variant) makes use of stores (see the documentation of the standard library); these may or may not be implemented with the help of verifier's challenges.

### Configuration files

In the subdirectory `config`, we have included two Circuit Configuration Communication (CCC) files. Both specify the same fields, conversions and plugins that the ZK back-end must support. In addition, `bank-account-chal.ccc` declares support for verifier's challenges, while `bank-account.ccc` does not. Depending on this, the ZK-SecreC compiler automatically chooses between stores implemented with verifier's challenges or without them.

## Helper script

The subdirectory `test-gen` contains a Haskell program `FinancialAuditing.hs` (together with its imports `JSON.hs` and `Util.hs`) that is capable of generating inputs (`public.json`, `instance.json` (empty), and `witness.json`) of various sizes for the income check statement. The "size" parameter for this script determines the number of rows in the account statement, with the increase of 1 in the parameter more-or-less corresponding to doubling of the account statement.

