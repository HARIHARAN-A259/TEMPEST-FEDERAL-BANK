#include "bank.h"
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#define BUFFER_SIZE 65536

/* Simple JSON String Extractor */
static int extractJsonString(const char *json, const char *key, char *out, size_t maxLen) {
    if (!json || !key || !out) return 0;
    out[0] = '\0';
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = strstr(json, pattern);
    if (!pos) return 0;

    pos += strlen(pattern);
    while (*pos && (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '\r' || *pos == '\n')) pos++;
    if (*pos != '\"') return 0;
    pos++; /* Skip opening quote */

    size_t i = 0;
    while (*pos && *pos != '\"' && i < maxLen - 1) {
        if (*pos == '\\' && *(pos + 1)) pos++; /* Skip escape */
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return 1;
}

/* Simple JSON Double Extractor */
static int extractJsonDouble(const char *json, const char *key, double *valOut) {
    if (!json || !key || !valOut) return 0;
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = strstr(json, pattern);
    if (!pos) return 0;

    pos += strlen(pattern);
    while (*pos && (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '\r' || *pos == '\n' || *pos == '\"')) pos++;
    char *endPtr = NULL;
    double d = strtod(pos, &endPtr);
    if (endPtr == pos) return 0;
    *valOut = d;
    return 1;
}

/* Send HTTP Response */
static void sendResponse(SOCKET client, int statusCode, const char *statusText,
                         const char *contentType, const char *body) {
    size_t bodyLen = body ? strlen(body) : 0;
    char header[1024];
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s; charset=utf-8\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
             "Connection: close\r\n\r\n",
             statusCode, statusText, contentType, bodyLen);

    send(client, header, (int)strlen(header), 0);
    if (bodyLen > 0) {
        send(client, body, (int)bodyLen, 0);
    }
}

/* Send JSON Success */
static void sendJsonSuccess(SOCKET client, const char *jsonPayload) {
    sendResponse(client, 200, "OK", "application/json", jsonPayload);
}

/* Send JSON Error */
static void sendJsonError(SOCKET client, int code, const char *message) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"success\":false,\"error\":\"%s\"}", message);
    sendResponse(client, code, "Bad Request", "application/json", buf);
}

/* Serve Static File */
static void serveFile(SOCKET client, const char *path, const char *contentType) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        sendResponse(client, 404, "Not Found", "text/plain", "404 Not Found");
        return;
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    char *buf = (char *)malloc(fsize + 1);
    if (!buf) {
        fclose(fp);
        sendResponse(client, 500, "Internal Server Error", "text/plain", "Out of memory");
        return;
    }
    fread(buf, 1, fsize, fp);
    fclose(fp);
    buf[fsize] = '\0';

    char header[1024];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Connection: close\r\n\r\n",
             contentType, fsize);
    send(client, header, (int)strlen(header), 0);
    send(client, buf, (int)fsize, 0);
    free(buf);
}

/* Handle HTTP Request */
static void handleClient(SOCKET client) {
    char *buffer = (char *)malloc(BUFFER_SIZE);
    if (!buffer) {
        closesocket(client);
        return;
    }

    int bytesRead = recv(client, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead <= 0) {
        free(buffer);
        closesocket(client);
        return;
    }
    buffer[bytesRead] = '\0';

    char method[16] = {0};
    char path[256] = {0};
    sscanf(buffer, "%15s %255s", method, path);

    /* Handle CORS Preflight */
    if (strcmp(method, "OPTIONS") == 0) {
        sendResponse(client, 200, "OK", "text/plain", "");
        free(buffer);
        closesocket(client);
        return;
    }

    /* Locate Body */
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) body += 4;
    else body = "";

    /* Route: Serve UI */
    if (strcmp(method, "GET") == 0 && (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0)) {
        serveFile(client, "index.html", "text/html; charset=utf-8");
    }
    else if (strcmp(method, "GET") == 0 && (strcmp(path, "/logo.jpg") == 0 || strcmp(path, "/logo.jpeg") == 0 || strcmp(path, "/logo.png") == 0)) {
        serveFile(client, "logo.jpg", "image/jpeg");
    }
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/status") == 0) {
        sendJsonSuccess(client, "{\"success\":true,\"bank\":\"Tempest Federal Bank\",\"status\":\"online\"}");
    }
    /* Route: API Sign Up */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/signup") == 0) {
        char name[NAME_LEN], dob[DOB_LEN], phone[PHONE_LEN], email[EMAIL_LEN], accType[TYPE_LEN], pin[PIN_LEN];
        double initialDeposit = 0.0;
        extractJsonString(body, "name", name, sizeof(name));
        extractJsonString(body, "dob", dob, sizeof(dob));
        extractJsonString(body, "phone", phone, sizeof(phone));
        extractJsonString(body, "email", email, sizeof(email));
        extractJsonString(body, "accountType", accType, sizeof(accType));
        extractJsonString(body, "pin", pin, sizeof(pin));
        extractJsonDouble(body, "initialDeposit", &initialDeposit);

        Account newAcc;
        char errorMsg[256] = {0};
        if (createAccount(name, dob, phone, email, accType, pin, initialDeposit, &newAcc, errorMsg)) {
            char response[1024];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"message\":\"Account created successfully\","
                     "\"account\":{\"accountNumber\":\"%s\",\"name\":\"%s\",\"dob\":\"%s\","
                     "\"phone\":\"%s\",\"email\":\"%s\",\"accountType\":\"%s\",\"balance\":%.2f,"
                     "\"createdAt\":\"%s\"}}",
                     newAcc.accountNumber, newAcc.name, newAcc.dob, newAcc.phone,
                     newAcc.email, newAcc.accountType, newAcc.balance, newAcc.createdAt);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: API Sign In */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/signin") == 0) {
        char user[PHONE_LEN], pin[PIN_LEN];
        extractJsonString(body, "user", user, sizeof(user));
        extractJsonString(body, "pin", pin, sizeof(pin));

        Account acc;
        char errorMsg[256] = {0};
        if (verifyLogin(user, pin, &acc, errorMsg)) {
            char response[1024];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"account\":{\"accountNumber\":\"%s\",\"name\":\"%s\","
                     "\"dob\":\"%s\",\"phone\":\"%s\",\"email\":\"%s\",\"accountType\":\"%s\","
                     "\"balance\":%.2f,\"createdAt\":\"%s\",\"pin\":\"%s\"}}",
                     acc.accountNumber, acc.name, acc.dob, acc.phone, acc.email,
                     acc.accountType, acc.balance, acc.createdAt, acc.pin);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 401, errorMsg);
        }
    }
    /* Route: API Forgot Account Number */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/forgot") == 0) {
        char phone[PHONE_LEN], dob[DOB_LEN];
        extractJsonString(body, "phone", phone, sizeof(phone));
        extractJsonString(body, "dob", dob, sizeof(dob));

        Account acc;
        if (findAccountByPhoneAndDOB(phone, dob, &acc)) {
            char response[512];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"accountNumber\":\"%s\",\"name\":\"%s\"}",
                     acc.accountNumber, acc.name);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 404, "No active account found matching the provided phone number and date of birth.");
        }
    }
    /* Route: Check Balance */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/check-balance") == 0) {
        char accNum[ACC_NUM_LEN], pin[PIN_LEN];
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "pin", pin, sizeof(pin));

        double balance = 0.0;
        char errorMsg[256] = {0};
        if (checkBalance(accNum, pin, &balance, errorMsg)) {
            char response[256];
            snprintf(response, sizeof(response), "{\"success\":true,\"balance\":%.2f}", balance);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: Account Details */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/account-details") == 0) {
        char accNum[ACC_NUM_LEN], pin[PIN_LEN];
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "pin", pin, sizeof(pin));

        Account acc;
        if (!isAccountExists(accNum, &acc) || !acc.isActive) {
            sendJsonError(client, 404, "Account not found or closed.");
        } else if (strcmp(acc.pin, pin) != 0) {
            sendJsonError(client, 401, "Security verification failed: Incorrect PIN.");
        } else {
            char response[1024];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"account\":{\"accountNumber\":\"%s\",\"name\":\"%s\","
                     "\"dob\":\"%s\",\"phone\":\"%s\",\"email\":\"%s\",\"accountType\":\"%s\","
                     "\"balance\":%.2f,\"createdAt\":\"%s\"}}",
                     acc.accountNumber, acc.name, acc.dob, acc.phone, acc.email,
                     acc.accountType, acc.balance, acc.createdAt);
            sendJsonSuccess(client, response);
        }
    }
    /* Route: Deposit */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/deposit") == 0) {
        char accNum[ACC_NUM_LEN], pin[PIN_LEN];
        double amount = 0.0;
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "pin", pin, sizeof(pin));
        extractJsonDouble(body, "amount", &amount);

        Transaction txn;
        char errorMsg[256] = {0};
        if (depositMoney(accNum, pin, amount, &txn, errorMsg)) {
            double prevBalance = txn.updatedBalance - txn.amount;
            char response[1024];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"receipt\":{\"transactionId\":\"%s\",\"timestamp\":\"%s\","
                     "\"type\":\"%s\",\"amount\":%.2f,\"previousBalance\":%.2f,\"updatedBalance\":%.2f,\"accountNumber\":\"%s\"}}",
                     txn.transactionId, txn.timestamp, txn.type, txn.amount, prevBalance, txn.updatedBalance, txn.accountNumber);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: Withdraw */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/withdraw") == 0) {
        char accNum[ACC_NUM_LEN], pin[PIN_LEN];
        double amount = 0.0;
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "pin", pin, sizeof(pin));
        extractJsonDouble(body, "amount", &amount);

        Transaction txn;
        char errorMsg[256] = {0};
        if (withdrawMoney(accNum, pin, amount, &txn, errorMsg)) {
            double prevBalance = txn.updatedBalance + txn.amount;
            char response[1024];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"receipt\":{\"transactionId\":\"%s\",\"timestamp\":\"%s\","
                     "\"type\":\"%s\",\"amount\":%.2f,\"previousBalance\":%.2f,\"updatedBalance\":%.2f,\"accountNumber\":\"%s\"}}",
                     txn.transactionId, txn.timestamp, txn.type, txn.amount, prevBalance, txn.updatedBalance, txn.accountNumber);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: Transfer Preview (Display receiver name before confirming) */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/transfer-preview") == 0) {
        char src[ACC_NUM_LEN], pin[PIN_LEN], target[ACC_NUM_LEN];
        extractJsonString(body, "sourceAccount", src, sizeof(src));
        extractJsonString(body, "pin", pin, sizeof(pin));
        extractJsonString(body, "targetAccount", target, sizeof(target));

        char targetName[NAME_LEN] = {0};
        char errorMsg[256] = {0};
        if (transferPreview(src, pin, target, targetName, errorMsg)) {
            char response[512];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"targetAccount\":\"%s\",\"targetName\":\"%s\"}",
                     target, targetName);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: Transfer Execute */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/transfer-confirm") == 0) {
        char src[ACC_NUM_LEN], pin[PIN_LEN], target[ACC_NUM_LEN];
        double amount = 0.0;
        extractJsonString(body, "sourceAccount", src, sizeof(src));
        extractJsonString(body, "pin", pin, sizeof(pin));
        extractJsonString(body, "targetAccount", target, sizeof(target));
        extractJsonDouble(body, "amount", &amount);

        Transaction txn;
        char errorMsg[256] = {0};
        if (transferMoney(src, pin, target, amount, &txn, errorMsg)) {
            double prevBalance = txn.updatedBalance + txn.amount;
            char response[1024];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"receipt\":{\"transactionId\":\"%s\",\"timestamp\":\"%s\","
                     "\"type\":\"%s\",\"amount\":%.2f,\"previousBalance\":%.2f,\"updatedBalance\":%.2f,\"accountNumber\":\"%s\","
                     "\"targetAccount\":\"%s\",\"targetName\":\"%s\"}}",
                     txn.transactionId, txn.timestamp, txn.type, txn.amount, prevBalance, txn.updatedBalance,
                     txn.accountNumber, txn.targetAccount, txn.targetName);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: Transaction History */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/history") == 0) {
        char accNum[ACC_NUM_LEN], pin[PIN_LEN];
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "pin", pin, sizeof(pin));

        Account acc;
        if (!isAccountExists(accNum, &acc) || !acc.isActive) {
            sendJsonError(client, 404, "Account not found or closed.");
        } else if (strcmp(acc.pin, pin) != 0) {
            sendJsonError(client, 401, "Security verification failed: Incorrect PIN.");
        } else {
            Transaction *txns = NULL;
            int count = 0;
            getTransactionHistory(accNum, &txns, &count);

            /* Build JSON array */
            char *jsonArr = (char *)malloc(32768);
            if (!jsonArr) {
                if (txns) free(txns);
                sendJsonError(client, 500, "Memory allocation error.");
            } else {
                strcpy(jsonArr, "{\"success\":true,\"transactions\":[");
                for (int i = 0; i < count; i++) {
                    char item[512];
                    snprintf(item, sizeof(item),
                             "%s{\"transactionId\":\"%s\",\"timestamp\":\"%s\",\"type\":\"%s\","
                             "\"amount\":%.2f,\"updatedBalance\":%.2f,\"targetAccount\":\"%s\",\"targetName\":\"%s\"}",
                             (i > 0 ? "," : ""),
                             txns[i].transactionId, txns[i].timestamp, txns[i].type,
                             txns[i].amount, txns[i].updatedBalance,
                             txns[i].targetAccount, txns[i].targetName);
                    strcat(jsonArr, item);
                }
                strcat(jsonArr, "]}");
                sendJsonSuccess(client, jsonArr);
                free(jsonArr);
                if (txns) free(txns);
            }
        }
    }
    /* Route: Search Transactions */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/search") == 0) {
        char accNum[ACC_NUM_LEN], pin[PIN_LEN], qId[32], qDate[32], qType[32];
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "pin", pin, sizeof(pin));
        extractJsonString(body, "transactionId", qId, sizeof(qId));
        extractJsonString(body, "date", qDate, sizeof(qDate));
        extractJsonString(body, "type", qType, sizeof(qType));

        Account acc;
        if (!isAccountExists(accNum, &acc) || !acc.isActive) {
            sendJsonError(client, 404, "Account not found.");
        } else if (strcmp(acc.pin, pin) != 0) {
            sendJsonError(client, 401, "Security verification failed: Incorrect PIN.");
        } else {
            Transaction *txns = NULL;
            int count = 0;
            searchTransactions(accNum, qId, qDate, qType, &txns, &count);

            char *jsonArr = (char *)malloc(32768);
            if (!jsonArr) {
                if (txns) free(txns);
                sendJsonError(client, 500, "Memory allocation error.");
            } else {
                strcpy(jsonArr, "{\"success\":true,\"transactions\":[");
                for (int i = 0; i < count; i++) {
                    char item[512];
                    snprintf(item, sizeof(item),
                             "%s{\"transactionId\":\"%s\",\"timestamp\":\"%s\",\"type\":\"%s\","
                             "\"amount\":%.2f,\"updatedBalance\":%.2f,\"targetAccount\":\"%s\",\"targetName\":\"%s\"}",
                             (i > 0 ? "," : ""),
                             txns[i].transactionId, txns[i].timestamp, txns[i].type,
                             txns[i].amount, txns[i].updatedBalance,
                             txns[i].targetAccount, txns[i].targetName);
                    strcat(jsonArr, item);
                }
                strcat(jsonArr, "]}");
                sendJsonSuccess(client, jsonArr);
                free(jsonArr);
                if (txns) free(txns);
            }
        }
    }
    /* Route: Analytics */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/analytics") == 0) {
        char accNum[ACC_NUM_LEN], pin[PIN_LEN];
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "pin", pin, sizeof(pin));

        Account acc;
        if (!isAccountExists(accNum, &acc) || !acc.isActive) {
            sendJsonError(client, 404, "Account not found.");
        } else if (strcmp(acc.pin, pin) != 0) {
            sendJsonError(client, 401, "Security verification failed: Incorrect PIN.");
        } else {
            AnalyticsSummary summary;
            MonthlySummary *monthly = NULL;
            int mCount = 0;
            getTransactionAnalytics(accNum, &summary, &monthly, &mCount);

            char *jsonBuf = (char *)malloc(16384);
            if (!jsonBuf) {
                if (monthly) free(monthly);
                sendJsonError(client, 500, "Memory allocation error.");
            } else {
                snprintf(jsonBuf, 16384,
                         "{\"success\":true,\"summary\":{\"totalDeposited\":%.2f,\"totalWithdrawn\":%.2f,"
                         "\"totalTransferred\":%.2f,\"depositCount\":%d,\"withdrawCount\":%d,\"transferCount\":%d,"
                         "\"totalCount\":%d},\"monthly\":[",
                         summary.totalDeposited, summary.totalWithdrawn, summary.totalTransferred,
                         summary.depositCount, summary.withdrawCount, summary.transferCount, summary.totalCount);

                for (int m = 0; m < mCount; m++) {
                    char mItem[256];
                    snprintf(mItem, sizeof(mItem),
                             "%s{\"month\":\"%s\",\"deposited\":%.2f,\"withdrawn\":%.2f,\"transferred\":%.2f,\"count\":%d}",
                             (m > 0 ? "," : ""),
                             monthly[m].month, monthly[m].deposited, monthly[m].withdrawn,
                             monthly[m].transferred, monthly[m].count);
                    strcat(jsonBuf, mItem);
                }
                strcat(jsonBuf, "]}");
                sendJsonSuccess(client, jsonBuf);
                free(jsonBuf);
                if (monthly) free(monthly);
            }
        }
    }
    /* Route: Change PIN */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/change-pin") == 0) {
        char accNum[ACC_NUM_LEN], oldPin[PIN_LEN], newPin1[PIN_LEN], newPin2[PIN_LEN];
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "oldPin", oldPin, sizeof(oldPin));
        extractJsonString(body, "newPin1", newPin1, sizeof(newPin1));
        extractJsonString(body, "newPin2", newPin2, sizeof(newPin2));

        char errorMsg[256] = {0};
        if (changePin(accNum, oldPin, newPin1, newPin2, errorMsg)) {
            sendJsonSuccess(client, "{\"success\":true,\"message\":\"PIN updated successfully.\"}");
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: Update Personal Details */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/update-details") == 0) {
        char accNum[ACC_NUM_LEN], pin[PIN_LEN], name[NAME_LEN], phone[PHONE_LEN], dob[DOB_LEN];
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "pin", pin, sizeof(pin));
        extractJsonString(body, "name", name, sizeof(name));
        extractJsonString(body, "phone", phone, sizeof(phone));
        extractJsonString(body, "dob", dob, sizeof(dob));

        char errorMsg[256] = {0};
        if (updatePersonalDetails(accNum, pin, name, phone, dob, errorMsg)) {
            Account acc;
            isAccountExists(accNum, &acc);
            char response[1024];
            snprintf(response, sizeof(response),
                     "{\"success\":true,\"message\":\"Personal details updated successfully.\","
                     "\"account\":{\"accountNumber\":\"%s\",\"name\":\"%s\",\"dob\":\"%s\","
                     "\"phone\":\"%s\",\"email\":\"%s\",\"accountType\":\"%s\",\"balance\":%.2f}}",
                     acc.accountNumber, acc.name, acc.dob, acc.phone, acc.email,
                     acc.accountType, acc.balance);
            sendJsonSuccess(client, response);
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: Delete Account */
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/delete-account") == 0) {
        char accNum[ACC_NUM_LEN], phone[PHONE_LEN], pin[PIN_LEN];
        extractJsonString(body, "accountNumber", accNum, sizeof(accNum));
        extractJsonString(body, "phone", phone, sizeof(phone));
        extractJsonString(body, "pin", pin, sizeof(pin));

        char errorMsg[256] = {0};
        if (deleteAccount(accNum, phone, pin, errorMsg)) {
            sendJsonSuccess(client, "{\"success\":true,\"message\":\"Account deleted and closed successfully.\"}");
        } else {
            sendJsonError(client, 400, errorMsg);
        }
    }
    /* Route: Download Receipt */
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/receipts/", 10) == 0) {
        char localPath[256];
        snprintf(localPath, sizeof(localPath), ".%s", path);
        serveFile(client, localPath, "text/plain; charset=utf-8");
    }
    /* Route: Download Card */
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/cards/", 7) == 0) {
        char localPath[256];
        snprintf(localPath, sizeof(localPath), ".%s", path);
        serveFile(client, localPath, "text/plain; charset=utf-8");
    }
    else {
        sendResponse(client, 404, "Not Found", "application/json", "{\"error\":\"Resource not found\"}");
    }

    free(buffer);
    closesocket(client);
}

/* Start Winsock Web Server */
int startWebServer(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[-] WSAStartup failed.\n");
        return 0;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        printf("[-] Failed to create socket.\n");
        WSACleanup();
        return 0;
    }

    /* Allow address reuse */
    BOOL opt = TRUE;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((u_short)port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("[-] Bind failed on port %d. Error code: %d\n", port, WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        printf("[-] Listen failed.\n");
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

    printf("\n");
    printf("========================================================================\n");
    printf("           TEMPEST FEDERAL BANK - EMBEDDED HTTP SERVER ACTIVE           \n");
    printf("========================================================================\n");
    printf(" [i] Local Web URL  : http://localhost:%d\n", port);
    printf(" [i] REST API Base  : http://localhost:%d/api\n", port);
    printf(" [i] Binary Storage : accounts.dat & transactions.dat\n");
    printf(" [i] Press Ctrl+C in this terminal window to stop the server.\n");
    printf("========================================================================\n\n");
    fflush(stdout);

    /* Open default browser asynchronously */
    char openUrl[128];
    snprintf(openUrl, sizeof(openUrl), "http://localhost:%d", port);
    ShellExecuteA(NULL, "open", openUrl, NULL, NULL, SW_SHOWNORMAL);

    while (1) {
        struct sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 1;
}

