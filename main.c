#include "bank.h"

static void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static void readLine(char *buf, int maxLen) {
    if (fgets(buf, maxLen, stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
    }
}

static void printAsciiHeader(void) {
    printf("\n");
    printf("========================================================================================\n");
    printf("                              TEMPEST FEDERAL BANK                                      \n");
    printf("                  Core Banking System (Structures & File Handling)                      \n");
    printf("========================================================================================\n");
}

/* Display ASCII bar chart for transaction analytics */
static void renderAsciiChart(double deposited, double withdrawn, double transferred) {
    double maxVal = deposited;
    if (withdrawn > maxVal) maxVal = withdrawn;
    if (transferred > maxVal) maxVal = transferred;

    if (maxVal <= 0.0) {
        printf("\n  [!] No transaction volume available to graph.\n");
        return;
    }

    const int maxBars = 35;
    int depBars = (int)((deposited / maxVal) * maxBars);
    int wthBars = (int)((withdrawn / maxVal) * maxBars);
    int trnBars = (int)((transferred / maxVal) * maxBars);

    printf("\n  ------------------------------------------------------------------------------------\n");
    printf("                       TRANSACTION VOLUME VISUAL CHART (ASCII)                        \n");
    printf("  ------------------------------------------------------------------------------------\n");

    printf("  Deposits    : [");
    for (int i = 0; i < maxBars; i++) printf("%c", (i < depBars) ? '#' : ' ');
    printf("]  $ %.2f\n", deposited);

    printf("  Withdrawals : [");
    for (int i = 0; i < maxBars; i++) printf("%c", (i < wthBars) ? '#' : ' ');
    printf("]  $ %.2f\n", withdrawn);

    printf("  Transfers   : [");
    for (int i = 0; i < maxBars; i++) printf("%c", (i < trnBars) ? '#' : ' ');
    printf("]  $ %.2f\n", transferred);

    printf("  ------------------------------------------------------------------------------------\n");
}

/* CLI: Create Account (Sign Up) */
static void cliSignUp(void) {
    printAsciiHeader();
    printf("                             NEW CUSTOMER REGISTRATION                                  \n");
    printf("----------------------------------------------------------------------------------------\n");

    char name[NAME_LEN], dob[DOB_LEN], phone[PHONE_LEN], email[EMAIL_LEN];
    char accType[TYPE_LEN], pin1[PIN_LEN], pin2[PIN_LEN];
    int typeChoice = 0;
    double initialDeposit = 0.0;

    printf(" Enter Full Name              : ");
    readLine(name, sizeof(name));

    printf(" Enter Date of Birth (YYYY-MM-DD): ");
    readLine(dob, sizeof(dob));

    printf(" Enter Phone Number           : ");
    readLine(phone, sizeof(phone));

    /* Requirement: If account already exists for the specified phone number, say Already exists */
    char existingAcc[ACC_NUM_LEN];
    if (isPhoneRegistered(phone, existingAcc)) {
        printf("\n [!] ERROR: Already exists: An account is already registered for this phone number!\n");
        printf("     (Registered Account Number: %s)\n", existingAcc);
        printf("\n Press Enter to return to main menu...");
        getchar();
        return;
    }

    printf(" Enter Email Address (Optional): ");
    readLine(email, sizeof(email));
    if (strlen(email) == 0) strcpy(email, "customer@tfb.bank");

    printf("\n Select Account Type:\n");
    printf("   1. Savings Account\n");
    printf("   2. Current Account\n");
    printf(" Choice (1 or 2): ");
    if (scanf("%d", &typeChoice) != 1 || (typeChoice != 1 && typeChoice != 2)) {
        clearInputBuffer();
        printf("\n [!] Invalid selection. Registration aborted.\n");
        return;
    }
    clearInputBuffer();
    strcpy(accType, (typeChoice == 1) ? "Savings" : "Current");

    printf("\n Enter Initial Deposit Amount ($): ");
    if (scanf("%lf", &initialDeposit) != 1 || initialDeposit < 0) {
        clearInputBuffer();
        printf("\n [!] Invalid deposit amount. Registration aborted.\n");
        return;
    }
    clearInputBuffer();

    printf("\n Set 4-Digit Security PIN     : ");
    readLine(pin1, sizeof(pin1));

    printf(" Confirm 4-Digit Security PIN : ");
    readLine(pin2, sizeof(pin2));

    if (strcmp(pin1, pin2) != 0) {
        printf("\n [!] PIN error: PIN confirmation does not match! Registration aborted.\n");
        printf("\n Press Enter to return to main menu...");
        getchar();
        return;
    }

    Account newAcc;
    char errorMsg[256] = {0};
    if (!createAccount(name, dob, phone, email, accType, pin1, initialDeposit, &newAcc, errorMsg)) {
        printf("\n [!] Registration Failed: %s\n", errorMsg);
        printf("\n Press Enter to return to main menu...");
        getchar();
        return;
    }

    printf("\n========================================================================================\n");
    printf("                    ACCOUNT SUCCESSFULLY CREATED AND ACTIVATED!                         \n");
    printf("========================================================================================\n");
    printf("  Assigned Account Number : %s\n", newAcc.accountNumber);
    printf("  Account Holder Name     : %s\n", newAcc.name);
    printf("  Date of Birth           : %s\n", newAcc.dob);
    printf("  Account Type            : %s\n", newAcc.accountType);
    printf("  Starting Balance        : $ %.2f\n", newAcc.balance);
    printf("----------------------------------------------------------------------------------------\n");

    /* Show downloadable digital card */
    printf("\n [i] Downloadable Card Generated: cards/card_%s.txt\n", newAcc.accountNumber);
    char cardFile[128];
    snprintf(cardFile, sizeof(cardFile), "cards/card_%s.txt", newAcc.accountNumber);
    FILE *cfp = fopen(cardFile, "r");
    if (cfp) {
        char line[256];
        printf("\n");
        while (fgets(line, sizeof(line), cfp)) {
            printf("%s", line);
        }
        fclose(cfp);
    }

    printf("\n Press Enter to return to main menu...");
    getchar();
}

/* CLI: Forgot Account Number */
static void cliForgotAccount(void) {
    printAsciiHeader();
    printf("                            RECOVER ACCOUNT NUMBER                                      \n");
    printf("----------------------------------------------------------------------------------------\n");

    char phone[PHONE_LEN], dob[DOB_LEN];
    printf(" Enter Registered Phone Number       : ");
    readLine(phone, sizeof(phone));

    printf(" Enter Date of Birth (YYYY-MM-DD)    : ");
    readLine(dob, sizeof(dob));

    Account acc;
    if (findAccountByPhoneAndDOB(phone, dob, &acc)) {
        printf("\n [✓] VERIFICATION SUCCESSFUL!\n");
        printf("----------------------------------------------------------------------------------------\n");
        printf("  Account Holder Name : %s\n", acc.name);
        printf("  Account Number      : %s\n", acc.accountNumber);
        printf("  Account Type        : %s\n", acc.accountType);
        printf("  Account Status      : ACTIVE\n");
        printf("----------------------------------------------------------------------------------------\n");
    } else {
        printf("\n [!] ERROR: No active account found matching phone '%s' and date of birth '%s'.\n", phone, dob);
    }

    printf("\n Press Enter to continue...");
    getchar();
}

/* CLI: Customer Dashboard after Sign In */
static void cliCustomerDashboard(Account loggedInAcc) {
    while (1) {
        /* Reload latest account state */
        if (!isAccountExists(loggedInAcc.accountNumber, &loggedInAcc) || !loggedInAcc.isActive) {
            printf("\n [!] This account is no longer active or has been deleted. Returning to main menu.\n");
            break;
        }

        printAsciiHeader();
        printf("  CUSTOMER PORTAL | Welcome, %s | Account: %s (%s)\n",
               loggedInAcc.name, loggedInAcc.accountNumber, loggedInAcc.accountType);
        printf("----------------------------------------------------------------------------------------\n");
        printf("  1.  Check Balance               (Requires PIN)\n");
        printf("  2.  View Account Details        (Requires PIN)\n");
        printf("  3.  Deposit Money               (Requires PIN)\n");
        printf("  4.  Withdraw Money              (Requires PIN)\n");
        printf("  5.  Transfer Money to Account   (Requires PIN - Previews Receiver Name)\n");
        printf("  6.  View Transaction History    (Requires PIN)\n");
        printf("  7.  Search Transactions         (Search by ID, Date, or Type)\n");
        printf("  8.  Transaction Analytics       (Summary & ASCII Visual Chart)\n");
        printf("  9.  Change 4-Digit Security PIN (Requires Existing PIN + New PIN Twice)\n");
        printf("  10. Update Personal Details     (Name, Phone, DOB)\n");
        printf("  11. View / Print Digital Card\n");
        printf("  12. Delete Account              (Requires Phone & PIN)\n");
        printf("  13. Sign Out\n");
        printf("----------------------------------------------------------------------------------------\n");
        printf(" Enter choice (1-13): ");

        int choice = 0;
        if (scanf("%d", &choice) != 1) {
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        if (choice == 1) {
            /* Check Balance */
            char pin[PIN_LEN];
            printf("\n Enter 4-Digit Security PIN: ");
            readLine(pin, sizeof(pin));

            double bal = 0.0;
            char err[256];
            if (checkBalance(loggedInAcc.accountNumber, pin, &bal, err)) {
                printf("\n [✓] Security Verification Successful!\n");
                printf("  --------------------------------------------------------\n");
                printf("   Available Balance: $ %.2f\n", bal);
                printf("  --------------------------------------------------------\n");
            } else {
                printf("\n [!] %s\n", err);
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 2) {
            /* Account Details */
            char pin[PIN_LEN];
            printf("\n Enter 4-Digit Security PIN: ");
            readLine(pin, sizeof(pin));

            if (strcmp(loggedInAcc.pin, pin) != 0) {
                printf("\n [!] Incorrect PIN.\n");
            } else {
                printf("\n  ======================================================\n");
                printf("                  ACCOUNT INFORMATION                   \n");
                printf("  ======================================================\n");
                printf("   Account Number   : %s\n", loggedInAcc.accountNumber);
                printf("   Account Holder   : %s\n", loggedInAcc.name);
                printf("   Account Type     : %s\n", loggedInAcc.accountType);
                printf("   Current Balance  : $ %.2f\n", loggedInAcc.balance);
                printf("   Phone Number     : %s\n", loggedInAcc.phone);
                printf("   Email Address    : %s\n", loggedInAcc.email);
                printf("   Date of Birth    : %s\n", loggedInAcc.dob);
                printf("   Created Timestamp: %s\n", loggedInAcc.createdAt);
                printf("   Account Status   : ACTIVE\n");
                printf("  ======================================================\n");
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 3) {
            /* Deposit */
            char pin[PIN_LEN];
            double amt = 0.0;
            printf("\n Enter 4-Digit Security PIN: ");
            readLine(pin, sizeof(pin));

            printf(" Enter Deposit Amount ($)  : ");
            if (scanf("%lf", &amt) != 1) {
                clearInputBuffer();
                printf("\n [!] Invalid amount.\n");
                continue;
            }
            clearInputBuffer();

            Transaction txn;
            char err[256];
            if (depositMoney(loggedInAcc.accountNumber, pin, amt, &txn, err)) {
                printf("\n [✓] DEPOSIT SUCCESSFUL!\n");
                printf("  Transaction ID : %s\n", txn.transactionId);
                printf("  Amount         : $ %.2f\n", txn.amount);
                printf("  New Balance    : $ %.2f\n", txn.updatedBalance);
                printf("  Receipt saved  : receipts/receipt_%s.txt\n", txn.transactionId);
            } else {
                printf("\n [!] Deposit Failed: %s\n", err);
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 4) {
            /* Withdrawal */
            char pin[PIN_LEN];
            double amt = 0.0;
            printf("\n Enter 4-Digit Security PIN: ");
            readLine(pin, sizeof(pin));

            printf(" Enter Withdrawal Amount ($): ");
            if (scanf("%lf", &amt) != 1) {
                clearInputBuffer();
                printf("\n [!] Invalid amount.\n");
                continue;
            }
            clearInputBuffer();

            Transaction txn;
            char err[256];
            if (withdrawMoney(loggedInAcc.accountNumber, pin, amt, &txn, err)) {
                printf("\n [✓] WITHDRAWAL SUCCESSFUL!\n");
                printf("  Transaction ID : %s\n", txn.transactionId);
                printf("  Amount         : $ %.2f\n", txn.amount);
                printf("  Updated Balance: $ %.2f\n", txn.updatedBalance);
                printf("  Receipt saved  : receipts/receipt_%s.txt\n", txn.transactionId);
            } else {
                printf("\n [!] Withdrawal Failed: %s\n", err);
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 5) {
            /* Transfer Money: Prompt target account -> Preview Receiver Name before confirming */
            char pin[PIN_LEN], targetAcc[ACC_NUM_LEN];
            double amt = 0.0;

            printf("\n Enter 4-Digit Security PIN      : ");
            readLine(pin, sizeof(pin));

            printf(" Enter Destination Account (TFBXXXX): ");
            readLine(targetAcc, sizeof(targetAcc));

            printf(" Enter Transfer Amount ($)       : ");
            if (scanf("%lf", &amt) != 1) {
                clearInputBuffer();
                printf("\n [!] Invalid amount.\n");
                continue;
            }
            clearInputBuffer();

            /* Requirement: Display receiver name and account number before confirming */
            char targetName[NAME_LEN] = {0};
            char err[256];
            if (!transferPreview(loggedInAcc.accountNumber, pin, targetAcc, targetName, err)) {
                printf("\n [!] Transfer Preparation Failed: %s\n", err);
                printf("\n Press Enter to continue...");
                getchar();
                continue;
            }

            printf("\n  ==============================================================\n");
            printf("                  TRANSFER VERIFICATION & CONFIRMATION          \n");
            printf("  ==============================================================\n");
            printf("   Receiver Name   : %s\n", targetName);
            printf("   Receiver Account: %s\n", targetAcc);
            printf("   Transfer Amount : $ %.2f\n", amt);
            printf("   Sender Balance  : $ %.2f\n", loggedInAcc.balance);
            printf("  --------------------------------------------------------------\n");
            printf("   Confirm transfer? (Y/N): ");

            char confirm[16];
            readLine(confirm, sizeof(confirm));
            if (confirm[0] == 'Y' || confirm[0] == 'y') {
                Transaction txn;
                if (transferMoney(loggedInAcc.accountNumber, pin, targetAcc, amt, &txn, err)) {
                    printf("\n [✓] TRANSFER COMPLETED SUCCESSFULLY!\n");
                    printf("  Transaction ID : %s\n", txn.transactionId);
                    printf("  Sent Amount    : $ %.2f\n", txn.amount);
                    printf("  Updated Balance: $ %.2f\n", txn.updatedBalance);
                    printf("  Receipt saved  : receipts/receipt_%s.txt\n", txn.transactionId);
                } else {
                    printf("\n [!] Transfer Execution Failed: %s\n", err);
                }
            } else {
                printf("\n [i] Transfer cancelled by user.\n");
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 6) {
            /* Transaction History */
            char pin[PIN_LEN];
            printf("\n Enter 4-Digit Security PIN: ");
            readLine(pin, sizeof(pin));

            if (strcmp(loggedInAcc.pin, pin) != 0) {
                printf("\n [!] Incorrect PIN.\n");
            } else {
                Transaction *txns = NULL;
                int count = 0;
                getTransactionHistory(loggedInAcc.accountNumber, &txns, &count);

                printf("\n========================================================================================\n");
                printf("                                TRANSACTION LEDGER HISTORY                              \n");
                printf("========================================================================================\n");
                printf(" %-15s | %-19s | %-14s | %-10s | %-10s | %-12s\n",
                       "Transaction ID", "Date & Time", "Type", "Amount ($)", "Balance ($)", "Target Acc");
                printf("----------------------------------------------------------------------------------------\n");

                if (count == 0) {
                    printf("   No transactions recorded yet.\n");
                } else {
                    for (int i = 0; i < count; i++) {
                        printf(" %-15s | %-19s | %-14s | %10.2f | %10.2f | %-12s\n",
                               txns[i].transactionId, txns[i].timestamp, txns[i].type,
                               txns[i].amount, txns[i].updatedBalance,
                               (strlen(txns[i].targetAccount) > 0 ? txns[i].targetAccount : "-"));
                    }
                }
                printf("========================================================================================\n");
                if (txns) free(txns);
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 7) {
            /* Search Transactions */
            char pin[PIN_LEN];
            printf("\n Enter 4-Digit Security PIN: ");
            readLine(pin, sizeof(pin));

            if (strcmp(loggedInAcc.pin, pin) != 0) {
                printf("\n [!] Incorrect PIN.\n");
            } else {
                char qId[32] = {0}, qDate[32] = {0}, qType[32] = {0};
                printf("\n Enter Search Criteria (leave empty to skip):\n");
                printf("   Search by Transaction ID (or prefix)  : ");
                readLine(qId, sizeof(qId));
                printf("   Search by Date (YYYY-MM-DD)           : ");
                readLine(qDate, sizeof(qDate));
                printf("   Search by Type (DEPOSIT/WITHDRAWAL/TRANSFER): ");
                readLine(qType, sizeof(qType));

                Transaction *matched = NULL;
                int matchCount = 0;
                searchTransactions(loggedInAcc.accountNumber, qId, qDate, qType, &matched, &matchCount);

                printf("\n----------------------------------------------------------------------------------------\n");
                printf(" SEARCH RESULTS (%d matches found):\n", matchCount);
                printf("----------------------------------------------------------------------------------------\n");
                for (int i = 0; i < matchCount; i++) {
                    printf(" [%s] %s | Type: %-14s | Amt: $%8.2f | Bal: $%8.2f\n",
                           matched[i].transactionId, matched[i].timestamp,
                           matched[i].type, matched[i].amount, matched[i].updatedBalance);
                }
                if (matched) free(matched);
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 8) {
            /* Transaction Analytics with ASCII Chart */
            AnalyticsSummary summary;
            MonthlySummary *monthly = NULL;
            int mCount = 0;
            getTransactionAnalytics(loggedInAcc.accountNumber, &summary, &monthly, &mCount);

            printf("\n========================================================================================\n");
            printf("                              TRANSACTION ANALYTICS REPORT                              \n");
            printf("========================================================================================\n");
            printf("  Total Money Deposited   : $ %12.2f  (%d operations)\n", summary.totalDeposited, summary.depositCount);
            printf("  Total Money Withdrawn   : $ %12.2f  (%d operations)\n", summary.totalWithdrawn, summary.withdrawCount);
            printf("  Total Money Transferred : $ %12.2f  (%d operations)\n", summary.totalTransferred, summary.transferCount);
            printf("  Total Ledger Records    : %d\n", summary.totalCount);
            printf("----------------------------------------------------------------------------------------\n");

            if (mCount > 0) {
                printf("  MONTHLY TRANSACTION SUMMARY:\n");
                for (int m = 0; m < mCount; m++) {
                    printf("    Month %-7s | Inflow: $ %8.2f | Outflow: $ %8.2f | Transferred: $ %8.2f\n",
                           monthly[m].month, monthly[m].deposited, monthly[m].withdrawn, monthly[m].transferred);
                }
            }

            renderAsciiChart(summary.totalDeposited, summary.totalWithdrawn, summary.totalTransferred);
            if (monthly) free(monthly);

            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 9) {
            /* Change PIN: Require verifying old PIN, entering new PIN twice */
            char oldPin[PIN_LEN], newPin1[PIN_LEN], newPin2[PIN_LEN];
            printf("\n Enter Current 4-Digit Security PIN : ");
            readLine(oldPin, sizeof(oldPin));

            printf(" Enter New 4-Digit Security PIN     : ");
            readLine(newPin1, sizeof(newPin1));

            printf(" Confirm New 4-Digit Security PIN   : ");
            readLine(newPin2, sizeof(newPin2));

            char err[256];
            if (changePin(loggedInAcc.accountNumber, oldPin, newPin1, newPin2, err)) {
                printf("\n [✓] Security PIN updated successfully!\n");
                strcpy(loggedInAcc.pin, newPin1);
            } else {
                printf("\n [!] PIN Update Failed: %s\n", err);
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 10) {
            /* Update Personal Details */
            char pin[PIN_LEN];
            printf("\n Enter 4-Digit Security PIN to authorize: ");
            readLine(pin, sizeof(pin));

            if (strcmp(loggedInAcc.pin, pin) != 0) {
                printf("\n [!] Incorrect PIN authorization failed.\n");
            } else {
                char newName[NAME_LEN], newPhone[PHONE_LEN], newDob[DOB_LEN];
                printf(" Enter Updated Name (Current: %s, or press Enter to keep): ", loggedInAcc.name);
                readLine(newName, sizeof(newName));

                printf(" Enter Updated Phone (Current: %s, or press Enter to keep): ", loggedInAcc.phone);
                readLine(newPhone, sizeof(newPhone));

                printf(" Enter Updated DOB (Current: %s, or press Enter to keep): ", loggedInAcc.dob);
                readLine(newDob, sizeof(newDob));

                char err[256];
                if (updatePersonalDetails(loggedInAcc.accountNumber, pin,
                                          (strlen(newName) > 0 ? newName : NULL),
                                          (strlen(newPhone) > 0 ? newPhone : NULL),
                                          (strlen(newDob) > 0 ? newDob : NULL), err)) {
                    printf("\n [✓] Personal details updated successfully!\n");
                } else {
                    printf("\n [!] Update Failed: %s\n", err);
                }
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 11) {
            /* View / Print Digital Card */
            char cardPath[128];
            generateCardFile(&loggedInAcc, cardPath);
            FILE *cfp = fopen(cardPath, "r");
            if (cfp) {
                char line[256];
                printf("\n");
                while (fgets(line, sizeof(line), cfp)) {
                    printf("%s", line);
                }
                fclose(cfp);
            }
            printf("\n Digital card file location: %s\n", cardPath);
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 12) {
            /* Delete Account: NEED TO ENTER PHONE NUMBER AND PIN */
            printf("\n  ==============================================================\n");
            printf("                  DANGER ZONE: DELETE ACCOUNT                   \n");
            printf("  ==============================================================\n");
            printf("  WARNING: Closing this account will permanently deactivate it. \n");
            printf("----------------------------------------------------------------\n");

            char phone[PHONE_LEN], pin[PIN_LEN];
            printf(" Enter Registered Phone Number: ");
            readLine(phone, sizeof(phone));

            printf(" Enter 4-Digit Security PIN   : ");
            readLine(pin, sizeof(pin));

            printf("\n Are you absolutely sure you want to permanently delete account %s? (Y/N): ",
                   loggedInAcc.accountNumber);
            char conf[16];
            readLine(conf, sizeof(conf));

            if (conf[0] == 'Y' || conf[0] == 'y') {
                char err[256];
                if (deleteAccount(loggedInAcc.accountNumber, phone, pin, err)) {
                    printf("\n [✓] ACCOUNT PERMANENTLY CLOSED AND DELETED.\n");
                    printf("\n Press Enter to return to main menu...");
                    getchar();
                    break;
                } else {
                    printf("\n [!] Deletion Failed: %s\n", err);
                }
            } else {
                printf("\n [i] Account deletion aborted.\n");
            }
            printf("\n Press Enter to continue...");
            getchar();
        }
        else if (choice == 13) {
            printf("\n [i] Signed out successfully.\n");
            break;
        }
    }
}

/* CLI: Sign In */
static void cliSignIn(void) {
    printAsciiHeader();
    printf("                                CUSTOMER SIGN IN                                        \n");
    printf("----------------------------------------------------------------------------------------\n");

    char user[PHONE_LEN], pin[PIN_LEN];
    printf(" Enter Account Number (TFBXXXX) or Phone: ");
    readLine(user, sizeof(user));

    printf(" Enter 4-Digit Security PIN             : ");
    readLine(pin, sizeof(pin));

    Account acc;
    char err[256];
    if (verifyLogin(user, pin, &acc, err)) {
        printf("\n [✓] Login successful! Loading dashboard...\n");
        cliCustomerDashboard(acc);
    } else {
        printf("\n [!] Sign In Failed: %s\n", err);
        printf("\n Press Enter to return to main menu...");
        getchar();
    }
}

int main(int argc, char *argv[]) {
    initDirectories();

    /* If run with --server argument, launch web server immediately */
    if (argc > 1 && strcmp(argv[1], "--server") == 0) {
        int port = 8080;
        if (argc > 2) port = atoi(argv[2]);
        if (port <= 0) port = 8080;
        startWebServer(port);
        return 0;
    }

    while (1) {
        printAsciiHeader();
        printf("                                    MAIN MENU                                           \n");
        printf("----------------------------------------------------------------------------------------\n");
        printf("  1. Sign Up (Create New Account & Download Digital Card)\n");
        printf("  2. Sign In (Access Balance, Deposits, Withdrawals, Transfers, Receipts)\n");
        printf("  3. Forgot Account Number (Recover using Phone & DOB)\n");
        printf("  4. Launch Web Banking Portal (Embedded HTTP Server - Port 8080)\n");
        printf("  5. Exit Program\n");
        printf("----------------------------------------------------------------------------------------\n");
        printf(" Enter your choice (1-5): ");

        int choice = 0;
        if (scanf("%d", &choice) != 1) {
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1:
                cliSignUp();
                break;
            case 2:
                cliSignIn();
                break;
            case 3:
                cliForgotAccount();
                break;
            case 4:
                printf("\n Starting Tempest Federal Bank Web Server on port 8080...\n");
                startWebServer(8080);
                break;
            case 5:
                printf("\n Thank you for using Tempest Federal Bank. Goodbye!\n\n");
                return 0;
            default:
                printf("\n [!] Invalid choice. Please select an option between 1 and 5.\n");
                break;
        }
    }
    return 0;
}

