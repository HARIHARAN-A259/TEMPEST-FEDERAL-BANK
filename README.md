# Tempest Federal Bank — Core Banking Simulation System
> **Microproject Topic**: Develop a C program to simulate basic banking operations using structures and file handling, integrated with a luxury web portal using the Tempest Federal Bank theme.

---

## 🏛️ Project Overview

**Tempest Federal Bank** is a complete, enterprise-grade banking simulation system developed in **pure C** using custom `struct` definitions and robust binary file handling (`accounts.dat` and `transactions.dat`). It comes paired with an ultra-luxury online banking web portal that matches the provided **Tempest Federal Bank** theme (gold-and-navy aesthetic, Cinzel typography, and fluid animated brand sequences).

The project is designed to run in two complementary modes:
1. **Interactive C Console Mode**: A comprehensive command-line terminal application with structured menus, security PIN authorization, receipt generators, and an ASCII volume chart.
2. **Web Portal Mode (Embedded C HTTP Server)**: Powered by an embedded Windows Winsock HTTP server written directly inside the C application (`server.c`), serving the luxury front-end (`index.html`) on `http://localhost:8080` and handling REST API endpoints. The web portal also features an offline browser fallback mode.

---

## 📋 Comprehensive Feature Checklist

| Requirement | Implementation Detail | Status |
| :--- | :--- | :---: |
| **Account Creation (Sign Up)** | Name, DOB, Phone, Account Type (Savings / Current), 4-Digit PIN. | ✅ **Complete** |
| **Phone Uniqueness** | Strictly checks if phone number is already registered; says `Already exists`. Max 1 account per phone number. | ✅ **Complete** |
| **Account Number Format** | Assigned as `TFBXXXX` (prefix `TFB` followed by 4 random digits, e.g., `TFB1024`). Checks against collisions. | ✅ **Complete** |
| **Downloadable Digital Card** | Shows Account Number, Name, DOB, Account Type, Status; downloadable as `.txt` / printable card. | ✅ **Complete** |
| **Sign In** | Authenticates via Account Number (or registered phone) AND 4-digit PIN. | ✅ **Complete** |
| **Forgot Account Number** | Recovers customer's `TFBXXXX` account number using registered Phone and DOB. | ✅ **Complete** |
| **Balance Enquiry** | Real-time balance check with 4-digit PIN authorization. | ✅ **Complete** |
| **Account Details** | Displays account number, holder name, account type, phone, email, DOB, creation timestamp. | ✅ **Complete** |
| **Cash Deposit** | PIN-authorized deposit, updates ledger balance, records transaction, generates receipt. | ✅ **Complete** |
| **Cash Withdrawal** | Validates balance & PIN, deducts funds, records transaction, generates receipt. | ✅ **Complete** |
| **Transfer to Another Account** | **Previews & displays receiver name and account number before confirmation**; updates both accounts. | ✅ **Complete** |
| **Transaction Receipts** | Generates detailed advice (Txn ID, Timestamp, Type, Amount, Balance, Counterparty); downloadable. | ✅ **Complete** |
| **Change 4-Digit PIN** | Verifies current PIN, enforces entering new PIN twice, validates 4 numeric digits. | ✅ **Complete** |
| **Update Personal Details** | Updates Name, Phone (with duplicate check), and DOB with PIN authorization. | ✅ **Complete** |
| **Transaction Analytics** | Calculates total deposited, withdrawn, transferred, net cash flow, and monthly summaries. | ✅ **Complete** |
| **Visual Chart** | ASCII bar chart in C terminal + luxury SVG bar chart in Web UI. | ✅ **Complete** |
| **Transaction Search** | Multi-criteria search by Transaction ID, Date (`YYYY-MM-DD`), or Type (`DEPOSIT`, `WITHDRAWAL`, `TRANSFER`). | ✅ **Complete** |
| **Account Deletion** | Enforces entering **registered phone number AND 4-digit PIN** with confirmation dialog. | ✅ **Complete** |

---

## 🗃️ C Structures & File Handling Architecture

### 1. Account Structure (`struct Account` in `bank.h`)
```c
typedef struct {
    char accountNumber[16]; /* Format: "TFBXXXX" (e.g. TFB1024) */
    char name[64];          /* Full Name */
    char dob[16];           /* Date of Birth: YYYY-MM-DD */
    char phone[24];         /* Unique Phone Number */
    char email[64];         /* Email Address */
    char accountType[20];   /* "Savings" or "Current" */
    char pin[8];            /* 4-Digit Security PIN */
    double balance;         /* Ledger Balance */
    int isActive;           /* 1 = Active, 0 = Closed/Deleted */
    char createdAt[32];     /* Timestamp */
} Account;
```

### 2. Transaction Ledger Structure (`struct Transaction` in `bank.h`)
```c
typedef struct {
    char transactionId[24];  /* Unique ID e.g. "TXN2611020101" */
    char accountNumber[16];  /* Source customer account */
    char targetAccount[16];  /* Counterparty account (transfers) */
    char targetName[64];     /* Counterparty name (transfers) */
    char type[20];           /* DEPOSIT, WITHDRAWAL, TRANSFER_SENT, TRANSFER_RECV */
    double amount;           /* Transaction amount ($) */
    double updatedBalance;   /* Resulting balance ($) */
    char timestamp[32];      /* Timestamp YYYY-MM-DD HH:MM:SS */
} Transaction;
```

### 3. Binary Files & Directories
- `accounts.dat`: Binary ledger storing customer `Account` records using `fread`, `fwrite`, and `fseek`.
- `transactions.dat`: Append-only binary ledger storing customer `Transaction` records.
- `cards/card_TFBXXXX.txt`: Generated ASCII digital account cards for customers.
- `receipts/receipt_TXNXXXX.txt`: Generated official transaction receipt slips.

---

## 🚀 How to Build and Run

### Prerequisites
- GCC Compiler (MinGW-w64 or MSYS2 GCC)
- Windows OS (uses `ws2_32` for networking and `direct.h` for directories)

### 1-Click Compilation
Run the provided batch file or execute GCC directly:
```bash
build.bat
```
*Or manual compilation:*
```bash
gcc -Wall -Wextra -O2 main.c bank_core.c server.c -o bank.exe -lws2_32
```

---

### Running Mode 1: Interactive C Console Application
Execute `bank.exe` without arguments:
```bash
.\bank.exe
```
This opens the terminal banking system where you can:
- Sign Up and watch your account card generated in `cards/card_TFBXXXX.txt`.
- Sign In with your PIN to deposit, withdraw, transfer money, view analytics charts, and search transactions.
- Choose Option 4 from the main menu to launch the Web Banking Server at any time!

---

### Running Mode 2: Web Banking Portal (Embedded C HTTP Server)
Run the provided `run.bat` or launch the server with the `--server` flag:
```bash
.\run.bat
```
*Or manual command:*
```bash
.\bank.exe --server
```
- The C program will start a native HTTP server on **port 8080** and automatically launch your default web browser to:
  `http://localhost:8080`
- All web operations (Sign up, sign in, deposits, transfers, receipts, profile updates) write directly to `accounts.dat` and `transactions.dat` in real time!

---

### Running Mode 3: Direct Browser Mode
If you double-click `index.html` directly in Google Chrome or Microsoft Edge, the page will detect that the C server is offline and automatically activate **Direct Browser Storage Mode** via `localStorage`, allowing offline demonstrations and testing!

---

### Running the Automated Unit Test Suite
To verify all banking logic and file handling against the 14 core project requirements:
```bash
gcc -Wall -Wextra -O2 test_bank.c bank_core.c -o test_bank.exe -lws2_32
.\test_bank.exe
```

---

## 📂 File Directory

```
F:\microproject\Logic using c\
├── bank.h                      # Core data structures, prototypes & constants
├── bank_core.c                 # Banking engine (accounts, transactions, file I/O)
├── server.c                    # Native Winsock HTTP server & REST API router
├── main.c                      # Terminal CLI UI & main program entry point
├── index.html                  # Full luxury web banking application (Tempest theme)
├── build.bat                   # 1-click GCC compiler script
├── run.bat                     # 1-click Web Server launcher
├── test_bank.c                 # Comprehensive 14-point automated test suite
├── accounts.dat                # Binary account records
├── transactions.dat            # Binary transaction ledger
├── cards/                      # Downloadable digital account cards (.txt)
└── receipts/                   # Downloadable official transaction receipts (.txt)
```

---

## 🛡️ Security & Validations Implemented
1. **Duplicate Phone Detection**: Rejects duplicate phone numbers during registration and profile update.
2. **PIN Protection**: 4-digit numeric verification required for balance checks, account details, deposits, withdrawals, transfers, and updates.
3. **Transfer Receiver Display**: Explicitly displays receiver name and account number for user verification before confirming funds transfer.
4. **Account Deletion Verification**: Requires entering registered phone number and 4-digit PIN before closing an account.
5. **Overdraw Prevention**: Disallows withdrawals and transfers exceeding available balance.

#   T E M P E S T - F E D E R A L - B A N K  
 