#include "bank.h"
#include <assert.h>

int main(void) {
    printf("[*] Starting Tempest Federal Bank Core Unit Tests...\n");
    initDirectories();

    /* Remove test files if they exist */
    remove(ACCOUNTS_FILE);
    remove(TRANSACTIONS_FILE);

    /* 1. Test Create Account 1 */
    Account acc1;
    char errMsg[256] = {0};
    int ok = createAccount("Alexander Hamilton", "1990-01-11", "9876543210", 
                           "alex@tfb.bank", "Savings", "1234", 1000.0, &acc1, errMsg);
    assert(ok == 1);
    assert(strncmp(acc1.accountNumber, "TFB", 3) == 0);
    assert(strlen(acc1.accountNumber) == 7); /* TFB + 4 digits */
    assert(acc1.balance == 1000.0);
    printf("[✓] Account 1 created: %s, Name: %s, Balance: $%.2f\n", 
           acc1.accountNumber, acc1.name, acc1.balance);

    /* 2. Test Phone Duplicate Check */
    Account dupAcc;
    ok = createAccount("Alex Imposter", "1995-05-05", "9876543210", 
                       "imposter@tfb.bank", "Savings", "4321", 500.0, &dupAcc, errMsg);
    assert(ok == 0);
    assert(strstr(errMsg, "Already exists") != NULL);
    printf("[✓] Duplicate phone correctly rejected: '%s'\n", errMsg);

    /* 3. Test Create Account 2 */
    Account acc2;
    ok = createAccount("Thomas Jefferson", "1992-04-13", "8765432109", 
                       "thomas@tfb.bank", "Current", "5678", 500.0, &acc2, errMsg);
    assert(ok == 1);
    assert(strncmp(acc2.accountNumber, "TFB", 3) == 0);
    assert(strcmp(acc1.accountNumber, acc2.accountNumber) != 0);
    printf("[✓] Account 2 created: %s, Name: %s, Balance: $%.2f\n", 
           acc2.accountNumber, acc2.name, acc2.balance);

    /* 4. Test Login */
    Account loggedIn;
    ok = verifyLogin(acc1.accountNumber, "1234", &loggedIn, errMsg);
    assert(ok == 1);
    assert(strcmp(loggedIn.name, "Alexander Hamilton") == 0);

    /* Test Wrong PIN */
    ok = verifyLogin(acc1.accountNumber, "9999", &loggedIn, errMsg);
    assert(ok == 0);
    printf("[✓] Sign in authentication verified (correct & wrong PIN)\n");

    /* 5. Test Forgot Account Number */
    Account recovered;
    ok = findAccountByPhoneAndDOB("9876543210", "1990-01-11", &recovered);
    assert(ok == 1);
    assert(strcmp(recovered.accountNumber, acc1.accountNumber) == 0);
    printf("[✓] Forgot account number recovery verified: %s\n", recovered.accountNumber);

    /* 6. Test Deposit */
    Transaction depTxn;
    ok = depositMoney(acc1.accountNumber, "1234", 500.0, &depTxn, errMsg);
    assert(ok == 1);
    assert(depTxn.updatedBalance == 1500.0);
    printf("[✓] Deposit verified. New balance: $%.2f, Txn ID: %s\n", 
           depTxn.updatedBalance, depTxn.transactionId);

    /* 7. Test Withdrawal */
    Transaction wthTxn;
    ok = withdrawMoney(acc1.accountNumber, "1234", 200.0, &wthTxn, errMsg);
    assert(ok == 1);
    assert(wthTxn.updatedBalance == 1300.0);
    printf("[✓] Withdrawal verified. New balance: $%.2f, Txn ID: %s\n", 
           wthTxn.updatedBalance, wthTxn.transactionId);

    /* Test Overdraw Withdrawal */
    ok = withdrawMoney(acc1.accountNumber, "1234", 50000.0, &wthTxn, errMsg);
    assert(ok == 0);
    assert(strstr(errMsg, "Insufficient") != NULL);
    printf("[✓] Overdraw correctly prevented: '%s'\n", errMsg);

    /* 8. Test Transfer Preview (displays receiver name before confirmation) */
    char previewName[NAME_LEN];
    ok = transferPreview(acc1.accountNumber, "1234", acc2.accountNumber, previewName, errMsg);
    assert(ok == 1);
    assert(strcmp(previewName, "Thomas Jefferson") == 0);
    printf("[✓] Transfer preview verified. Receiver: %s (%s)\n", 
           previewName, acc2.accountNumber);

    /* 9. Test Transfer Money */
    Transaction trnTxn;
    ok = transferMoney(acc1.accountNumber, "1234", acc2.accountNumber, 300.0, &trnTxn, errMsg);
    assert(ok == 1);
    assert(trnTxn.updatedBalance == 1000.0); /* 1300 - 300 */

    Account acc2Check;
    isAccountExists(acc2.accountNumber, &acc2Check);
    assert(acc2Check.balance == 800.0); /* 500 + 300 */
    printf("[✓] Transfer verified. Sender balance: $%.2f, Receiver balance: $%.2f\n", 
           trnTxn.updatedBalance, acc2Check.balance);

    /* 10. Test Transaction History & Search */
    Transaction *allTxns = NULL;
    int txnCount = 0;
    ok = getTransactionHistory(acc1.accountNumber, &allTxns, &txnCount);
    assert(ok == 1);
    assert(txnCount == 4); /* Initial deposit + deposit + withdraw + transfer sent */
    printf("[✓] Transaction history verified (%d transactions recorded)\n", txnCount);
    free(allTxns);

    Transaction *searchTxns = NULL;
    int matchCount = 0;
    ok = searchTransactions(acc1.accountNumber, "", "", "DEPOSIT", &searchTxns, &matchCount);
    assert(ok == 1);
    assert(matchCount == 2); /* 2 deposits */
    printf("[✓] Transaction search by type DEPOSIT verified (%d matches)\n", matchCount);
    free(searchTxns);

    /* 11. Test Analytics */
    AnalyticsSummary summary;
    MonthlySummary *monthly = NULL;
    int mCount = 0;
    ok = getTransactionAnalytics(acc1.accountNumber, &summary, &monthly, &mCount);
    assert(ok == 1);
    assert(summary.totalDeposited == 1500.0);
    assert(summary.totalWithdrawn == 200.0);
    assert(summary.totalTransferred == 300.0);
    printf("[✓] Analytics verified: Dep: $%.2f, With: $%.2f, Trn: $%.2f\n", 
           summary.totalDeposited, summary.totalWithdrawn, summary.totalTransferred);
    if (monthly) free(monthly);

    /* 12. Test Change PIN */
    ok = changePin(acc1.accountNumber, "1234", "9876", "9876", errMsg);
    assert(ok == 1);
    ok = verifyLogin(acc1.accountNumber, "9876", &loggedIn, errMsg);
    assert(ok == 1);
    printf("[✓] PIN change verified (new PIN active)\n");

    /* 13. Test Update Personal Details */
    ok = updatePersonalDetails(acc1.accountNumber, "9876", "Alexander Hamilton Sr.", 
                               "9998887776", "1990-01-11", errMsg);
    assert(ok == 1);
    isAccountExists(acc1.accountNumber, &loggedIn);
    assert(strcmp(loggedIn.name, "Alexander Hamilton Sr.") == 0);
    assert(strcmp(loggedIn.phone, "9998887776") == 0);
    printf("[✓] Personal details update verified: Name: %s, Phone: %s\n", 
           loggedIn.name, loggedIn.phone);

    /* 14. Test Delete Account */
    ok = deleteAccount(acc1.accountNumber, "9998887776", "9876", errMsg);
    assert(ok == 1);
    ok = verifyLogin(acc1.accountNumber, "9876", &loggedIn, errMsg);
    assert(ok == 0);
    assert(strstr(errMsg, "closed") != NULL || strstr(errMsg, "deleted") != NULL);
    printf("[✓] Account deletion verified: '%s'\n", errMsg);

    printf("\n[SUCCESS] ALL 14 UNIT TESTS PASSED WITH 100%% SUCCESS!\n");
    return 0;
}

