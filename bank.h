#ifndef BANK_H
#define BANK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define ACCOUNTS_FILE "accounts.dat"
#define TRANSACTIONS_FILE "transactions.dat"
#define RECEIPTS_DIR "receipts"
#define CARDS_DIR "cards"

#define ACC_NUM_LEN 16
#define NAME_LEN 64
#define DOB_LEN 16
#define PHONE_LEN 24
#define EMAIL_LEN 64
#define TYPE_LEN 20
#define PIN_LEN 8
#define DATE_LEN 32
#define TXN_ID_LEN 24

/* Customer Account Structure for Binary File Handling */
typedef struct {
    char accountNumber[ACC_NUM_LEN]; /* e.g. "TFB1024" (TFB + 4 random digits) */
    char name[NAME_LEN];             /* Full Name */
    char dob[DOB_LEN];               /* YYYY-MM-DD */
    char phone[PHONE_LEN];           /* Phone number (Unique per customer) */
    char email[EMAIL_LEN];           /* Email address */
    char accountType[TYPE_LEN];      /* "Savings" or "Current" */
    char pin[PIN_LEN];               /* 4-digit security PIN */
    double balance;                  /* Current ledger balance */
    int isActive;                    /* 1 = Active, 0 = Deleted/Closed */
    char createdAt[DATE_LEN];        /* Creation date/time */
} Account;

/* Transaction Ledger Structure for Binary File Handling */
typedef struct {
    char transactionId[TXN_ID_LEN];  /* e.g. "TXN241102001" */
    char accountNumber[ACC_NUM_LEN]; /* Customer account */
    char targetAccount[ACC_NUM_LEN]; /* Target account (transfers only) */
    char targetName[NAME_LEN];       /* Target holder name (transfers only) */
    char type[TYPE_LEN];             /* "DEPOSIT", "WITHDRAWAL", "TRANSFER_SENT", "TRANSFER_RECV" */
    double amount;                   /* Transaction amount */
    double updatedBalance;           /* Resulting balance */
    char timestamp[DATE_LEN];        /* Date and time */
} Transaction;

/* Analytics Data Structures */
typedef struct {
    double totalDeposited;
    double totalWithdrawn;
    double totalTransferred;
    int depositCount;
    int withdrawCount;
    int transferCount;
    int totalCount;
} AnalyticsSummary;

typedef struct {
    char month[12]; /* "YYYY-MM" */
    double deposited;
    double withdrawn;
    double transferred;
    int count;
} MonthlySummary;

/* Core Function Prototypes */
void initDirectories(void);
void getCurrentTimestamp(char *buffer, size_t size);
int generateAccountNumber(char *accNumBuffer);
int generateTransactionId(char *txnIdBuffer);

/* Account Helpers */
int isPhoneRegistered(const char *phone, char *existingAccNum);
int isAccountExists(const char *accNum, Account *accOut);
int findAccountByPhoneAndDOB(const char *phone, const char *dob, Account *accOut);
int updateAccountRecord(const Account *acc);

/* Banking Operations */
int createAccount(const char *name, const char *dob, const char *phone,
                  const char *email, const char *accType, const char *pin,
                  double initialDeposit, Account *newAccOut, char *errorMsg);

int verifyLogin(const char *accNumOrPhone, const char *pin, Account *accOut, char *errorMsg);

int checkBalance(const char *accNum, const char *pin, double *balanceOut, char *errorMsg);

int depositMoney(const char *accNum, const char *pin, double amount,
                 Transaction *txnOut, char *errorMsg);

int withdrawMoney(const char *accNum, const char *pin, double amount,
                  Transaction *txnOut, char *errorMsg);

int transferPreview(const char *sourceAccNum, const char *pin, const char *targetAccNum,
                    char *targetNameOut, char *errorMsg);

int transferMoney(const char *sourceAccNum, const char *pin, const char *targetAccNum,
                  double amount, Transaction *txnOut, char *errorMsg);

int changePin(const char *accNum, const char *oldPin, const char *newPin1,
              const char *newPin2, char *errorMsg);

int updatePersonalDetails(const char *accNum, const char *pin, const char *newName,
                          const char *newPhone, const char *newDob, char *errorMsg);

int deleteAccount(const char *accNum, const char *phone, const char *pin, char *errorMsg);

/* History, Search & Analytics */
int getTransactionHistory(const char *accNum, Transaction **txnsOut, int *countOut);

int searchTransactions(const char *accNum, const char *queryId, const char *queryDate,
                       const char *queryType, Transaction **txnsOut, int *countOut);

int getTransactionAnalytics(const char *accNum, AnalyticsSummary *summaryOut,
                            MonthlySummary **monthlyOut, int *monthCountOut);

/* File Handling Exports */
int generateReceiptFile(const Transaction *txn, const char *accHolderName, char *filePathOut);
int generateCardFile(const Account *acc, char *filePathOut);

/* Interactive Frontends */
void runCLI(void);
int startWebServer(int port);

#endif /* BANK_H */

