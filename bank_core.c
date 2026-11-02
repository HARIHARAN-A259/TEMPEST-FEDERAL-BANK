#include "bank.h"
#include <direct.h> /* For _mkdir on Windows */

void initDirectories(void) {
    _mkdir(RECEIPTS_DIR);
    _mkdir(CARDS_DIR);
    srand((unsigned int)time(NULL));
}

void getCurrentTimestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
    } else {
        snprintf(buffer, size, "2026-11-02 12:00:00");
    }
}

/* Check if account number exists in binary file */
int isAccountExists(const char *accNum, Account *accOut) {
    FILE *fp = fopen(ACCOUNTS_FILE, "rb");
    if (!fp) return 0;

    Account acc;
    while (fread(&acc, sizeof(Account), 1, fp) == 1) {
        if (strcmp(acc.accountNumber, accNum) == 0) {
            if (accOut) *accOut = acc;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

/* Helper to normalize phone numbers (digits only) */
static void normalizePhone(const char *in, char *out, size_t maxLen) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j < maxLen - 1; i++) {
        if (isdigit((unsigned char)in[i])) {
            out[j++] = in[i];
        }
    }
    out[j] = '\0';
}

/* Check if phone number is already registered to an existing account */
int isPhoneRegistered(const char *phone, char *existingAccNum) {
    FILE *fp = fopen(ACCOUNTS_FILE, "rb");
    if (!fp) return 0;

    char normInput[PHONE_LEN];
    normalizePhone(phone, normInput, sizeof(normInput));

    Account acc;
    while (fread(&acc, sizeof(Account), 1, fp) == 1) {
        if (acc.isActive) {
            char normAccPhone[PHONE_LEN];
            normalizePhone(acc.phone, normAccPhone, sizeof(normAccPhone));
            if (strcmp(normAccPhone, normInput) == 0) {
                if (existingAccNum) snprintf(existingAccNum, ACC_NUM_LEN, "%s", acc.accountNumber);
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}

/* Search account by phone and DOB (for Forgot Account Number) */
int findAccountByPhoneAndDOB(const char *phone, const char *dob, Account *accOut) {
    FILE *fp = fopen(ACCOUNTS_FILE, "rb");
    if (!fp) return 0;

    char normInput[PHONE_LEN];
    normalizePhone(phone, normInput, sizeof(normInput));

    Account acc;
    while (fread(&acc, sizeof(Account), 1, fp) == 1) {
        if (acc.isActive) {
            char normAccPhone[PHONE_LEN];
            normalizePhone(acc.phone, normAccPhone, sizeof(normAccPhone));
            if (strcmp(normAccPhone, normInput) == 0 && strcmp(acc.dob, dob) == 0) {
                if (accOut) *accOut = acc;
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}

/* Generate unique account number matching TFBXXXX format */
int generateAccountNumber(char *accNumBuffer) {
    int attempts = 0;
    while (attempts < 10000) {
        int randNum = 1000 + (rand() % 9000); /* 1000 to 9999 */
        snprintf(accNumBuffer, ACC_NUM_LEN, "TFB%04d", randNum);
        if (!isAccountExists(accNumBuffer, NULL)) {
            return 1; /* Unique found */
        }
        attempts++;
    }
    return 0;
}

/* Generate unique transaction ID */
int generateTransactionId(char *txnIdBuffer) {
    static int counter = 100;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char datePrefix[12];
    if (t) {
        strftime(datePrefix, sizeof(datePrefix), "%y%m%d", t);
    } else {
        snprintf(datePrefix, sizeof(datePrefix), "261102");
    }
    counter = (counter + 1) % 10000;
    snprintf(txnIdBuffer, TXN_ID_LEN, "TXN%s%04d", datePrefix, counter);
    return 1;
}

/* Update an existing account record in accounts.dat */
int updateAccountRecord(const Account *acc) {
    FILE *fp = fopen(ACCOUNTS_FILE, "r+b");
    if (!fp) return 0;

    Account current;
    while (fread(&current, sizeof(Account), 1, fp) == 1) {
        if (strcmp(current.accountNumber, acc->accountNumber) == 0) {
            fseek(fp, -((long)sizeof(Account)), SEEK_CUR);
            fwrite(acc, sizeof(Account), 1, fp);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

/* Append transaction to transactions.dat */
static int recordTransaction(const Transaction *txn) {
    FILE *fp = fopen(TRANSACTIONS_FILE, "ab");
    if (!fp) return 0;
    fwrite(txn, sizeof(Transaction), 1, fp);
    fclose(fp);
    return 1;
}

/* Generate downloadable digital card text file */
int generateCardFile(const Account *acc, char *filePathOut) {
    initDirectories();
    char filename[128];
    snprintf(filename, sizeof(filename), "%s/card_%s.txt", CARDS_DIR, acc->accountNumber);

    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;

    fprintf(fp, "====================================================================\n");
    fprintf(fp, "                     TEMPEST FEDERAL BANK                           \n");
    fprintf(fp, "                 OFFICIAL DIGITAL ACCOUNT CARD                      \n");
    fprintf(fp, "====================================================================\n");
    fprintf(fp, "  [EMV CHIP]                                                        \n");
    fprintf(fp, "                                                                    \n");
    fprintf(fp, "  ACCOUNT NUMBER : %-15s                                  \n", acc->accountNumber);
    fprintf(fp, "  ACCOUNT HOLDER : %-30s                   \n", acc->name);
    fprintf(fp, "  ACCOUNT TYPE   : %-15s                                  \n", acc->accountType);
    fprintf(fp, "  DATE OF BIRTH  : %-15s                                  \n", acc->dob);
    fprintf(fp, "  PHONE NUMBER   : %-18s                                \n", acc->phone);
    fprintf(fp, "  MEMBER SINCE   : %-20s                               \n", acc->createdAt);
    fprintf(fp, "  STATUS         : %-10s                                       \n", acc->isActive ? "ACTIVE" : "CLOSED");
    fprintf(fp, "====================================================================\n");
    fprintf(fp, "   Security Notice: Keep your 4-digit PIN confidential at all times. \n");
    fprintf(fp, "   Tempest Federal Bank - 24/7 Priority Support: support@tfb.bank    \n");
    fprintf(fp, "====================================================================\n");
    fclose(fp);

    if (filePathOut) strncpy(filePathOut, filename, 128);
    return 1;
}

/* Generate downloadable transaction receipt */
int generateReceiptFile(const Transaction *txn, const char *accHolderName, char *filePathOut) {
    initDirectories();
    char filename[128];
    snprintf(filename, sizeof(filename), "%s/receipt_%s.txt", RECEIPTS_DIR, txn->transactionId);

    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;

    fprintf(fp, "====================================================================\n");
    fprintf(fp, "                     TEMPEST FEDERAL BANK                           \n");
    fprintf(fp, "                 OFFICIAL TRANSACTION RECEIPT                       \n");
    fprintf(fp, "====================================================================\n");
    fprintf(fp, "  TRANSACTION ID : %-20s                             \n", txn->transactionId);
    fprintf(fp, "  DATE & TIME    : %-25s                        \n", txn->timestamp);
    fprintf(fp, "  ACCOUNT NUMBER : %-15s                             \n", txn->accountNumber);
    if (accHolderName && strlen(accHolderName) > 0) {
        fprintf(fp, "  ACCOUNT HOLDER : %-30s              \n", accHolderName);
    }
    fprintf(fp, "  TYPE           : %-20s                             \n", txn->type);
    fprintf(fp, "  AMOUNT         : $ %-15.2f                             \n", txn->amount);
    fprintf(fp, "  UPDATED BALANCE: $ %-15.2f                             \n", txn->updatedBalance);
    if (strlen(txn->targetAccount) > 0) {
        fprintf(fp, "  RECEIVER ACC   : %-15s                             \n", txn->targetAccount);
        fprintf(fp, "  RECEIVER NAME  : %-30s              \n", txn->targetName);
    }
    fprintf(fp, "====================================================================\n");
    fprintf(fp, "   Status: SUCCESSFUL - VERIFIED BY TEMPEST FEDERAL BANK CORE       \n");
    fprintf(fp, "   Thank you for banking with Tempest Federal Bank.                 \n");
    fprintf(fp, "====================================================================\n");
    fclose(fp);

    if (filePathOut) strncpy(filePathOut, filename, 128);
    return 1;
}

/* Create a new account with all requirements and phone duplicate check */
int createAccount(const char *name, const char *dob, const char *phone,
                  const char *email, const char *accType, const char *pin,
                  double initialDeposit, Account *newAccOut, char *errorMsg) {
    initDirectories();

    if (!name || strlen(name) < 2) {
        if (errorMsg) strcpy(errorMsg, "Invalid name. Name must be at least 2 characters.");
        return 0;
    }
    if (!dob || strlen(dob) < 8) {
        if (errorMsg) strcpy(errorMsg, "Invalid date of birth format.");
        return 0;
    }
    if (!phone || strlen(phone) < 7) {
        if (errorMsg) strcpy(errorMsg, "Invalid phone number. Must contain at least 7 digits.");
        return 0;
    }
    if (!accType || (strcmp(accType, "Savings") != 0 && strcmp(accType, "Current") != 0)) {
        if (errorMsg) strcpy(errorMsg, "Invalid account type. Must be either 'Savings' or 'Current'.");
        return 0;
    }
    if (!pin || strlen(pin) != 4) {
        if (errorMsg) strcpy(errorMsg, "PIN must be exactly 4 digits.");
        return 0;
    }
    for (int i = 0; i < 4; i++) {
        if (!isdigit((unsigned char)pin[i])) {
            if (errorMsg) strcpy(errorMsg, "PIN must contain only numbers (0-9).");
            return 0;
        }
    }
    if (initialDeposit < 0) {
        if (errorMsg) strcpy(errorMsg, "Initial deposit cannot be negative.");
        return 0;
    }

    /* Requirement: Check if account already exists for the specified phone number */
    char existingAcc[ACC_NUM_LEN];
    if (isPhoneRegistered(phone, existingAcc)) {
        if (errorMsg) strcpy(errorMsg, "Already exists: An account is already registered for this phone number.");
        return 0;
    }

    /* Requirement: Generate unique TFBXXXX account number */
    Account newAcc;
    memset(&newAcc, 0, sizeof(Account));
    if (!generateAccountNumber(newAcc.accountNumber)) {
        if (errorMsg) strcpy(errorMsg, "System error: Unable to generate unique account number.");
        return 0;
    }

    strncpy(newAcc.name, name, NAME_LEN - 1);
    strncpy(newAcc.dob, dob, DOB_LEN - 1);
    strncpy(newAcc.phone, phone, PHONE_LEN - 1);
    if (email && strlen(email) > 0) {
        strncpy(newAcc.email, email, EMAIL_LEN - 1);
    } else {
        strcpy(newAcc.email, "customer@tfb.bank");
    }
    strncpy(newAcc.accountType, accType, TYPE_LEN - 1);
    strncpy(newAcc.pin, pin, PIN_LEN - 1);
    newAcc.balance = initialDeposit;
    newAcc.isActive = 1;
    getCurrentTimestamp(newAcc.createdAt, sizeof(newAcc.createdAt));

    /* Append to binary file */
    FILE *fp = fopen(ACCOUNTS_FILE, "ab");
    if (!fp) {
        if (errorMsg) strcpy(errorMsg, "File system error: Unable to open accounts file.");
        return 0;
    }
    fwrite(&newAcc, sizeof(Account), 1, fp);
    fclose(fp);

    /* Generate digital card file */
    generateCardFile(&newAcc, NULL);

    /* Record initial deposit transaction if > 0 */
    if (initialDeposit > 0) {
        Transaction txn;
        memset(&txn, 0, sizeof(Transaction));
        generateTransactionId(txn.transactionId);
        snprintf(txn.accountNumber, sizeof(txn.accountNumber), "%s", newAcc.accountNumber);
        strcpy(txn.type, "DEPOSIT");
        txn.amount = initialDeposit;
        txn.updatedBalance = initialDeposit;
        getCurrentTimestamp(txn.timestamp, sizeof(txn.timestamp));
        recordTransaction(&txn);
        generateReceiptFile(&txn, newAcc.name, NULL);
    }

    if (newAccOut) *newAccOut = newAcc;
    return 1;
}

/* Sign In verification */
int verifyLogin(const char *accNumOrPhone, const char *pin, Account *accOut, char *errorMsg) {
    if (!accNumOrPhone || !pin) {
        if (errorMsg) strcpy(errorMsg, "Account number and PIN are required.");
        return 0;
    }

    FILE *fp = fopen(ACCOUNTS_FILE, "rb");
    if (!fp) {
        if (errorMsg) strcpy(errorMsg, "No accounts on file. Please create an account first.");
        return 0;
    }

    char normInput[PHONE_LEN];
    normalizePhone(accNumOrPhone, normInput, sizeof(normInput));

    Account acc;
    int found = 0;
    while (fread(&acc, sizeof(Account), 1, fp) == 1) {
        char normAccPhone[PHONE_LEN];
        normalizePhone(acc.phone, normAccPhone, sizeof(normAccPhone));

        if (strcmp(acc.accountNumber, accNumOrPhone) == 0 ||
            (strlen(normInput) >= 7 && strcmp(normAccPhone, normInput) == 0)) {
            found = 1;
            break;
        }
    }
    fclose(fp);

    if (!found) {
        if (errorMsg) strcpy(errorMsg, "Account not found. Please check your account number or phone.");
        return 0;
    }
    if (!acc.isActive) {
        if (errorMsg) strcpy(errorMsg, "This account has been closed/deleted.");
        return 0;
    }
    if (strcmp(acc.pin, pin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Incorrect 4-digit PIN.");
        return 0;
    }

    if (accOut) *accOut = acc;
    return 1;
}

/* Check balance (requires PIN) */
int checkBalance(const char *accNum, const char *pin, double *balanceOut, char *errorMsg) {
    Account acc;
    if (!isAccountExists(accNum, &acc)) {
        if (errorMsg) strcpy(errorMsg, "Account not found.");
        return 0;
    }
    if (!acc.isActive) {
        if (errorMsg) strcpy(errorMsg, "Account is inactive.");
        return 0;
    }
    if (strcmp(acc.pin, pin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Security verification failed: Incorrect PIN.");
        return 0;
    }
    if (balanceOut) *balanceOut = acc.balance;
    return 1;
}

/* Deposit money (requires PIN) */
int depositMoney(const char *accNum, const char *pin, double amount,
                 Transaction *txnOut, char *errorMsg) {
    if (amount <= 0.0) {
        if (errorMsg) strcpy(errorMsg, "Deposit amount must be strictly greater than zero.");
        return 0;
    }

    Account acc;
    if (!isAccountExists(accNum, &acc)) {
        if (errorMsg) strcpy(errorMsg, "Account not found.");
        return 0;
    }
    if (!acc.isActive) {
        if (errorMsg) strcpy(errorMsg, "Account is inactive.");
        return 0;
    }
    if (strcmp(acc.pin, pin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Security verification failed: Incorrect PIN.");
        return 0;
    }

    acc.balance += amount;
    if (!updateAccountRecord(&acc)) {
        if (errorMsg) strcpy(errorMsg, "Database error: Failed to update balance.");
        return 0;
    }

    Transaction txn;
    memset(&txn, 0, sizeof(Transaction));
    generateTransactionId(txn.transactionId);
    snprintf(txn.accountNumber, sizeof(txn.accountNumber), "%s", acc.accountNumber);
    strcpy(txn.type, "DEPOSIT");
    txn.amount = amount;
    txn.updatedBalance = acc.balance;
    getCurrentTimestamp(txn.timestamp, sizeof(txn.timestamp));

    recordTransaction(&txn);
    generateReceiptFile(&txn, acc.name, NULL);

    if (txnOut) *txnOut = txn;
    return 1;
}

/* Withdraw money (requires PIN and balance verification) */
int withdrawMoney(const char *accNum, const char *pin, double amount,
                  Transaction *txnOut, char *errorMsg) {
    if (amount <= 0.0) {
        if (errorMsg) strcpy(errorMsg, "Withdrawal amount must be strictly greater than zero.");
        return 0;
    }

    Account acc;
    if (!isAccountExists(accNum, &acc)) {
        if (errorMsg) strcpy(errorMsg, "Account not found.");
        return 0;
    }
    if (!acc.isActive) {
        if (errorMsg) strcpy(errorMsg, "Account is inactive.");
        return 0;
    }
    if (strcmp(acc.pin, pin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Security verification failed: Incorrect PIN.");
        return 0;
    }
    if (acc.balance < amount) {
        if (errorMsg) snprintf(errorMsg, 128, "Insufficient funds! Current balance: $%.2f", acc.balance);
        return 0;
    }

    acc.balance -= amount;
    if (!updateAccountRecord(&acc)) {
        if (errorMsg) strcpy(errorMsg, "Database error: Failed to update balance.");
        return 0;
    }

    Transaction txn;
    memset(&txn, 0, sizeof(Transaction));
    generateTransactionId(txn.transactionId);
    snprintf(txn.accountNumber, sizeof(txn.accountNumber), "%s", acc.accountNumber);
    strcpy(txn.type, "WITHDRAWAL");
    txn.amount = amount;
    txn.updatedBalance = acc.balance;
    getCurrentTimestamp(txn.timestamp, sizeof(txn.timestamp));

    recordTransaction(&txn);
    generateReceiptFile(&txn, acc.name, NULL);

    if (txnOut) *txnOut = txn;
    return 1;
}

/* Transfer Preview: Display receiver name and account number before confirming */
int transferPreview(const char *sourceAccNum, const char *pin, const char *targetAccNum,
                    char *targetNameOut, char *errorMsg) {
    if (strcmp(sourceAccNum, targetAccNum) == 0) {
        if (errorMsg) strcpy(errorMsg, "Cannot transfer money to your own account.");
        return 0;
    }

    Account src;
    if (!isAccountExists(sourceAccNum, &src)) {
        if (errorMsg) strcpy(errorMsg, "Source account not found.");
        return 0;
    }
    if (!src.isActive) {
        if (errorMsg) strcpy(errorMsg, "Source account is inactive.");
        return 0;
    }
    if (strcmp(src.pin, pin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Security verification failed: Incorrect PIN.");
        return 0;
    }

    Account target;
    if (!isAccountExists(targetAccNum, &target) || !target.isActive) {
        if (errorMsg) strcpy(errorMsg, "Destination account not found or is closed.");
        return 0;
    }

    if (targetNameOut) snprintf(targetNameOut, NAME_LEN, "%s", target.name);
    return 1;
}

/* Execute Money Transfer between two accounts */
int transferMoney(const char *sourceAccNum, const char *pin, const char *targetAccNum,
                  double amount, Transaction *txnOut, char *errorMsg) {
    if (amount <= 0.0) {
        if (errorMsg) strcpy(errorMsg, "Transfer amount must be strictly greater than zero.");
        return 0;
    }
    if (strcmp(sourceAccNum, targetAccNum) == 0) {
        if (errorMsg) strcpy(errorMsg, "Cannot transfer money to yourself.");
        return 0;
    }

    Account src, dest;
    if (!isAccountExists(sourceAccNum, &src) || !src.isActive) {
        if (errorMsg) strcpy(errorMsg, "Source account invalid or closed.");
        return 0;
    }
    if (strcmp(src.pin, pin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Security verification failed: Incorrect PIN.");
        return 0;
    }
    if (src.balance < amount) {
        if (errorMsg) snprintf(errorMsg, 128, "Insufficient balance! Available: $%.2f", src.balance);
        return 0;
    }
    if (!isAccountExists(targetAccNum, &dest) || !dest.isActive) {
        if (errorMsg) strcpy(errorMsg, "Destination account does not exist or is inactive.");
        return 0;
    }

    /* Update balances */
    src.balance -= amount;
    dest.balance += amount;

    if (!updateAccountRecord(&src) || !updateAccountRecord(&dest)) {
        if (errorMsg) strcpy(errorMsg, "Database error: Failed to complete transfer.");
        return 0;
    }

    /* Record sender transaction */
    Transaction txnSender;
    memset(&txnSender, 0, sizeof(Transaction));
    generateTransactionId(txnSender.transactionId);
    snprintf(txnSender.accountNumber, sizeof(txnSender.accountNumber), "%s", src.accountNumber);
    snprintf(txnSender.targetAccount, sizeof(txnSender.targetAccount), "%s", dest.accountNumber);
    snprintf(txnSender.targetName, sizeof(txnSender.targetName), "%s", dest.name);
    strcpy(txnSender.type, "TRANSFER_SENT");
    txnSender.amount = amount;
    txnSender.updatedBalance = src.balance;
    getCurrentTimestamp(txnSender.timestamp, sizeof(txnSender.timestamp));
    recordTransaction(&txnSender);
    generateReceiptFile(&txnSender, src.name, NULL);

    /* Record receiver transaction */
    Transaction txnReceiver;
    memset(&txnReceiver, 0, sizeof(Transaction));
    generateTransactionId(txnReceiver.transactionId);
    snprintf(txnReceiver.accountNumber, sizeof(txnReceiver.accountNumber), "%s", dest.accountNumber);
    snprintf(txnReceiver.targetAccount, sizeof(txnReceiver.targetAccount), "%s", src.accountNumber);
    snprintf(txnReceiver.targetName, sizeof(txnReceiver.targetName), "%s", src.name);
    strcpy(txnReceiver.type, "TRANSFER_RECV");
    txnReceiver.amount = amount;
    txnReceiver.updatedBalance = dest.balance;
    getCurrentTimestamp(txnReceiver.timestamp, sizeof(txnReceiver.timestamp));
    recordTransaction(&txnReceiver);

    if (txnOut) *txnOut = txnSender;
    return 1;
}

/* Change 4-digit PIN: Verify existing PIN, require entering new PIN twice */
int changePin(const char *accNum, const char *oldPin, const char *newPin1,
              const char *newPin2, char *errorMsg) {
    Account acc;
    if (!isAccountExists(accNum, &acc) || !acc.isActive) {
        if (errorMsg) strcpy(errorMsg, "Account not found or inactive.");
        return 0;
    }
    if (strcmp(acc.pin, oldPin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Current PIN verification failed.");
        return 0;
    }
    if (strlen(newPin1) != 4 || strlen(newPin2) != 4) {
        if (errorMsg) strcpy(errorMsg, "New PIN must be exactly 4 digits.");
        return 0;
    }
    for (int i = 0; i < 4; i++) {
        if (!isdigit((unsigned char)newPin1[i])) {
            if (errorMsg) strcpy(errorMsg, "New PIN must contain only digits.");
            return 0;
        }
    }
    if (strcmp(newPin1, newPin2) != 0) {
        if (errorMsg) strcpy(errorMsg, "New PIN confirmation does not match.");
        return 0;
    }
    if (strcmp(oldPin, newPin1) == 0) {
        if (errorMsg) strcpy(errorMsg, "New PIN cannot be the same as your existing PIN.");
        return 0;
    }

    strncpy(acc.pin, newPin1, PIN_LEN - 1);
    if (!updateAccountRecord(&acc)) {
        if (errorMsg) strcpy(errorMsg, "Database error: Failed to update PIN.");
        return 0;
    }
    return 1;
}

/* Update personal details: Name, Phone (with duplicate check), DOB */
int updatePersonalDetails(const char *accNum, const char *pin, const char *newName,
                          const char *newPhone, const char *newDob, char *errorMsg) {
    Account acc;
    if (!isAccountExists(accNum, &acc) || !acc.isActive) {
        if (errorMsg) strcpy(errorMsg, "Account not found or inactive.");
        return 0;
    }
    if (strcmp(acc.pin, pin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Security verification failed: Incorrect PIN.");
        return 0;
    }

    if (newName && strlen(newName) >= 2) {
        strncpy(acc.name, newName, NAME_LEN - 1);
    }
    if (newDob && strlen(newDob) >= 8) {
        strncpy(acc.dob, newDob, DOB_LEN - 1);
    }
    if (newPhone && strlen(newPhone) >= 7) {
        char existingAcc[ACC_NUM_LEN];
        if (isPhoneRegistered(newPhone, existingAcc) && strcmp(existingAcc, accNum) != 0) {
            if (errorMsg) strcpy(errorMsg, "Phone number is already registered to another account.");
            return 0;
        }
        strncpy(acc.phone, newPhone, PHONE_LEN - 1);
    }

    if (!updateAccountRecord(&acc)) {
        if (errorMsg) strcpy(errorMsg, "Database error: Failed to update personal details.");
        return 0;
    }
    generateCardFile(&acc, NULL); /* Refresh digital card */
    return 1;
}

/* Delete account: Prompts phone number and PIN verification */
int deleteAccount(const char *accNum, const char *phone, const char *pin, char *errorMsg) {
    Account acc;
    if (!isAccountExists(accNum, &acc) || !acc.isActive) {
        if (errorMsg) strcpy(errorMsg, "Account not found or already closed.");
        return 0;
    }

    char normInput[PHONE_LEN];
    char normAccPhone[PHONE_LEN];
    normalizePhone(phone, normInput, sizeof(normInput));
    normalizePhone(acc.phone, normAccPhone, sizeof(normAccPhone));

    if (strcmp(normInput, normAccPhone) != 0) {
        if (errorMsg) strcpy(errorMsg, "Verification failed: Phone number does not match this account.");
        return 0;
    }
    if (strcmp(acc.pin, pin) != 0) {
        if (errorMsg) strcpy(errorMsg, "Verification failed: Incorrect PIN.");
        return 0;
    }

    acc.isActive = 0; /* Mark as deleted */
    if (!updateAccountRecord(&acc)) {
        if (errorMsg) strcpy(errorMsg, "Database error: Failed to delete account.");
        return 0;
    }
    return 1;
}

/* Retrieve transaction history for an account */
int getTransactionHistory(const char *accNum, Transaction **txnsOut, int *countOut) {
    FILE *fp = fopen(TRANSACTIONS_FILE, "rb");
    if (!fp) {
        *txnsOut = NULL;
        *countOut = 0;
        return 0;
    }

    Transaction txn;
    int count = 0;
    while (fread(&txn, sizeof(Transaction), 1, fp) == 1) {
        if (strcmp(txn.accountNumber, accNum) == 0) {
            count++;
        }
    }

    if (count == 0) {
        fclose(fp);
        *txnsOut = NULL;
        *countOut = 0;
        return 1;
    }

    Transaction *list = (Transaction *)malloc(count * sizeof(Transaction));
    if (!list) {
        fclose(fp);
        *txnsOut = NULL;
        *countOut = 0;
        return 0;
    }

    rewind(fp);
    int idx = 0;
    while (fread(&txn, sizeof(Transaction), 1, fp) == 1) {
        if (strcmp(txn.accountNumber, accNum) == 0 && idx < count) {
            list[idx++] = txn;
        }
    }
    fclose(fp);

    *txnsOut = list;
    *countOut = count;
    return 1;
}

/* Search transactions by ID, Date, or Type */
int searchTransactions(const char *accNum, const char *queryId, const char *queryDate,
                       const char *queryType, Transaction **txnsOut, int *countOut) {
    Transaction *allTxns = NULL;
    int totalCount = 0;
    if (!getTransactionHistory(accNum, &allTxns, &totalCount) || totalCount == 0) {
        *txnsOut = NULL;
        *countOut = 0;
        return 1;
    }

    Transaction *matched = (Transaction *)malloc(totalCount * sizeof(Transaction));
    if (!matched) {
        free(allTxns);
        *txnsOut = NULL;
        *countOut = 0;
        return 0;
    }

    int matchedCount = 0;
    for (int i = 0; i < totalCount; i++) {
        int match = 1;
        if (queryId && strlen(queryId) > 0) {
            if (strstr(allTxns[i].transactionId, queryId) == NULL) match = 0;
        }
        if (match && queryDate && strlen(queryDate) > 0) {
            if (strstr(allTxns[i].timestamp, queryDate) == NULL) match = 0;
        }
        if (match && queryType && strlen(queryType) > 0 && strcmp(queryType, "ALL") != 0) {
            if (strstr(allTxns[i].type, queryType) == NULL) match = 0;
        }
        if (match) {
            matched[matchedCount++] = allTxns[i];
        }
    }

    free(allTxns);
    *txnsOut = matched;
    *countOut = matchedCount;
    return 1;
}

/* Transaction Analytics */
int getTransactionAnalytics(const char *accNum, AnalyticsSummary *summaryOut,
                            MonthlySummary **monthlyOut, int *monthCountOut) {
    memset(summaryOut, 0, sizeof(AnalyticsSummary));
    if (monthlyOut) *monthlyOut = NULL;
    if (monthCountOut) *monthCountOut = 0;

    Transaction *txns = NULL;
    int count = 0;
    if (!getTransactionHistory(accNum, &txns, &count) || count == 0) {
        return 1;
    }

    MonthlySummary months[24];
    int numMonths = 0;
    memset(months, 0, sizeof(months));

    for (int i = 0; i < count; i++) {
        summaryOut->totalCount++;
        char monthKey[8] = {0};
        strncpy(monthKey, txns[i].timestamp, 7); /* YYYY-MM */

        int mIdx = -1;
        for (int m = 0; m < numMonths; m++) {
            if (strcmp(months[m].month, monthKey) == 0) {
                mIdx = m;
                break;
            }
        }
        if (mIdx == -1 && numMonths < 24) {
            mIdx = numMonths++;
            strncpy(months[mIdx].month, monthKey, sizeof(months[mIdx].month) - 1);
        }

        if (strcmp(txns[i].type, "DEPOSIT") == 0 || strcmp(txns[i].type, "TRANSFER_RECV") == 0) {
            summaryOut->totalDeposited += txns[i].amount;
            summaryOut->depositCount++;
            if (mIdx != -1) {
                months[mIdx].deposited += txns[i].amount;
                months[mIdx].count++;
            }
        } else if (strcmp(txns[i].type, "WITHDRAWAL") == 0) {
            summaryOut->totalWithdrawn += txns[i].amount;
            summaryOut->withdrawCount++;
            if (mIdx != -1) {
                months[mIdx].withdrawn += txns[i].amount;
                months[mIdx].count++;
            }
        } else if (strcmp(txns[i].type, "TRANSFER_SENT") == 0) {
            summaryOut->totalTransferred += txns[i].amount;
            summaryOut->transferCount++;
            if (mIdx != -1) {
                months[mIdx].transferred += txns[i].amount;
                months[mIdx].count++;
            }
        }
    }

    if (monthlyOut && monthCountOut && numMonths > 0) {
        MonthlySummary *mAlloc = (MonthlySummary *)malloc(numMonths * sizeof(MonthlySummary));
        if (mAlloc) {
            memcpy(mAlloc, months, numMonths * sizeof(MonthlySummary));
            *monthlyOut = mAlloc;
            *monthCountOut = numMonths;
        }
    }

    free(txns);
    return 1;
}
