#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

/* ================= CONSTANTS ================= */
#define MAX_ACCOUNTS 1000
#define MAX_TRANSACTIONS 500
#define MAX_NAME 100
#define MAX_EMAIL 100
#define ADMIN_PASSWORD 1234

/* ================= STRUCTURES ================= */

typedef struct {
    int account_no;
    char name[MAX_NAME];
    char email[MAX_EMAIL];
    int age;
    int encrypted_pin;
    float balance;
    float loan_amount;
    float loan_total;
    float mutual_fund_investment;
} Account;

typedef struct {
    int account_no;
    char message[200];
    time_t timestamp;
} Transaction;

/* ================= GLOBAL VARIABLES ================= */

Account accounts[MAX_ACCOUNTS];
int total_accounts = 0;
Transaction transactions[MAX_TRANSACTIONS];
int total_transactions = 0;
sqlite3 *db = NULL;

/* ================= ENCRYPTION FUNCTIONS ================= */

int encrypt_decrypt(int pin) {
    int key = 5678;
    return pin ^ key;
}

/* ================= DATABASE FUNCTIONS ================= */

int init_database() {
    int rc = sqlite3_open("bank_system.db", &db);
    
    if (rc) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    const char *sql = "CREATE TABLE IF NOT EXISTS accounts("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "account_no INTEGER UNIQUE, "
                      "name TEXT NOT NULL, "
                      "email TEXT, "
                      "age INTEGER, "
                      "encrypted_pin INTEGER, "
                      "balance REAL DEFAULT 0, "
                      "loan_amount REAL DEFAULT 0, "
                      "loan_total REAL DEFAULT 0, "
                      "mutual_fund REAL DEFAULT 0, "
                      "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                      "); "
                      
                      "CREATE TABLE IF NOT EXISTS transactions("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "account_no INTEGER, "
                      "message TEXT, "
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                      "FOREIGN KEY(account_no) REFERENCES accounts(account_no)"
                      ");";
    
    char *err_msg = 0;
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    
    return 1;
}

int create_account_db(Account *account) {
    const char *sql = "INSERT INTO accounts(account_no, name, email, age, encrypted_pin, balance) "
                      "VALUES(?, ?, ?, ?, ?, ?)";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, account->account_no);
        sqlite3_bind_text(stmt, 2, account->name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, account->email, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, account->age);
        sqlite3_bind_int(stmt, 5, account->encrypted_pin);
        sqlite3_bind_double(stmt, 6, account->balance);
        
        rc = sqlite3_step(stmt);
    }
    
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 1 : 0;
}

int find_account_db(int account_no, Account *account) {
    const char *sql = "SELECT * FROM accounts WHERE account_no = ?";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, account_no);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            account->account_no = sqlite3_column_int(stmt, 1);
            strcpy(account->name, (const char *)sqlite3_column_text(stmt, 2));
            strcpy(account->email, (const char *)sqlite3_column_text(stmt, 3));
            account->age = sqlite3_column_int(stmt, 4);
            account->encrypted_pin = sqlite3_column_int(stmt, 5);
            account->balance = sqlite3_column_double(stmt, 6);
            account->loan_amount = sqlite3_column_double(stmt, 7);
            account->loan_total = sqlite3_column_double(stmt, 8);
            account->mutual_fund_investment = sqlite3_column_double(stmt, 9);
            
            sqlite3_finalize(stmt);
            return 1;
        }
    }
    
    sqlite3_finalize(stmt);
    return 0;
}

int update_account_db(Account *account) {
    const char *sql = "UPDATE accounts SET balance = ?, loan_amount = ?, "
                      "loan_total = ?, mutual_fund = ? WHERE account_no = ?";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    if (rc == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, account->balance);
        sqlite3_bind_double(stmt, 2, account->loan_amount);
        sqlite3_bind_double(stmt, 3, account->loan_total);
        sqlite3_bind_double(stmt, 4, account->mutual_fund_investment);
        sqlite3_bind_int(stmt, 5, account->account_no);
        
        rc = sqlite3_step(stmt);
    }
    
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 1 : 0;
}

void add_transaction_db(int account_no, const char *message) {
    const char *sql = "INSERT INTO transactions(account_no, message) VALUES(?, ?)";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, account_no);
        sqlite3_bind_text(stmt, 2, message, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
    }
    
    sqlite3_finalize(stmt);
}

int get_all_accounts_db(Account **accounts_arr) {
    const char *sql = "SELECT * FROM accounts ORDER BY created_at DESC";
    
    sqlite3_stmt *stmt;
    int count = 0;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW && count < MAX_ACCOUNTS) {
            Account *acc = (Account *)malloc(sizeof(Account));
            acc->account_no = sqlite3_column_int(stmt, 1);
            strcpy(acc->name, (const char *)sqlite3_column_text(stmt, 2));
            strcpy(acc->email, (const char *)sqlite3_column_text(stmt, 3));
            acc->age = sqlite3_column_int(stmt, 4);
            acc->encrypted_pin = sqlite3_column_int(stmt, 5);
            acc->balance = sqlite3_column_double(stmt, 6);
            acc->loan_amount = sqlite3_column_double(stmt, 7);
            acc->loan_total = sqlite3_column_double(stmt, 8);
            acc->mutual_fund_investment = sqlite3_column_double(stmt, 9);
            
            accounts_arr[count++] = acc;
        }
    }
    
    sqlite3_finalize(stmt);
    return count;
}

/* ================= UI FUNCTIONS ================= */

void print_header() {
    printf("\n");
    printf("=================================================\n");
    printf("        SECURE FUTURE BANK - C BACKEND\n");
    printf("=================================================\n\n");
}

void print_separator() {
    printf("-------------------------------------------------\n");
}

void pause_screen() {
    printf("\nPress Enter to continue...");
    getchar();
}

/* ================= ACCOUNT FUNCTIONS ================= */

void create_account() {
    system("clear || cls");
    print_header();
    
    Account new_account = {0};
    
    printf("Enter Account Number: ");
    scanf("%d", &new_account.account_no);
    
    printf("Enter Name: ");
    scanf(" %[^\n]", new_account.name);
    
    printf("Enter Email: ");
    scanf("%s", new_account.email);
    
    printf("Enter Age: ");
    scanf("%d", &new_account.age);
    
    int pin;
    printf("Set 4-digit PIN: ");
    scanf("%d", &pin);
    
    new_account.encrypted_pin = encrypt_decrypt(pin);
    new_account.balance = 0;
    new_account.loan_amount = 0;
    new_account.loan_total = 0;
    new_account.mutual_fund_investment = 0;
    
    if (create_account_db(&new_account)) {
        printf("\n✅ Account Created Successfully!\n");
        printf("Account Number: %d\n", new_account.account_no);
    } else {
        printf("\n❌ Error creating account\n");
    }
    
    pause_screen();
}

/* ================= TRANSACTION FUNCTIONS ================= */

void deposit(Account *account) {
    float amount;
    printf("\nEnter amount to deposit: ");
    scanf("%f", &amount);
    
    if (amount > 0) {
        account->balance += amount;
        update_account_db(account);
        
        char msg[200];
        snprintf(msg, sizeof(msg), "Deposited: %.2f", amount);
        add_transaction_db(account->account_no, msg);
        
        printf("✅ Deposit Successful! Balance: %.2f\n", account->balance);
    } else {
        printf("❌ Invalid amount\n");
    }
    
    pause_screen();
}

void withdraw(Account *account) {
    float amount;
    printf("\nEnter amount to withdraw: ");
    scanf("%f", &amount);
    
    if (amount > account->balance) {
        printf("❌ Insufficient Balance! Available: %.2f\n", account->balance);
    } else if (amount > 0) {
        account->balance -= amount;
        update_account_db(account);
        
        char msg[200];
        snprintf(msg, sizeof(msg), "Withdrawn: %.2f", amount);
        add_transaction_db(account->account_no, msg);
        
        printf("✅ Withdrawal Successful! Balance: %.2f\n", account->balance);
    } else {
        printf("❌ Invalid amount\n");
    }
    
    pause_screen();
}

void transfer_money(Account *from_account) {
    int to_account_no;
    float amount;
    
    printf("\nEnter Receiver Account Number: ");
    scanf("%d", &to_account_no);
    
    Account to_account;
    if (!find_account_db(to_account_no, &to_account)) {
        printf("❌ Receiver Account Not Found!\n");
        pause_screen();
        return;
    }
    
    printf("Enter Amount to Transfer: ");
    scanf("%f", &amount);
    
    if (amount > from_account->balance) {
        printf("❌ Insufficient Balance!\n");
    } else if (amount > 0) {
        from_account->balance -= amount;
        to_account.balance += amount;
        
        update_account_db(from_account);
        update_account_db(&to_account);
        
        char msg1[200], msg2[200];
        snprintf(msg1, sizeof(msg1), "Transferred: %.2f to AccNo %d", amount, to_account_no);
        snprintf(msg2, sizeof(msg2), "Received: %.2f from AccNo %d", amount, from_account->account_no);
        
        add_transaction_db(from_account->account_no, msg1);
        add_transaction_db(to_account_no, msg2);
        
        printf("✅ Transfer Successful!\n");
    } else {
        printf("❌ Invalid amount\n");
    }
    
    pause_screen();
}

/* ================= LOAN FUNCTIONS ================= */

void apply_loan(Account *account) {
    float principal, rate, years;
    
    printf("\nLoan Amount: ");
    scanf("%f", &principal);
    
    printf("Interest Rate (%%): ");
    scanf("%f", &rate);
    
    printf("Time (years): ");
    scanf("%f", &years);
    
    // Calculate compound interest
    float total = principal;
    for (int i = 0; i < (int)years; i++) {
        total = total * (1 + rate / 100);
    }
    
    account->loan_amount = principal;
    account->loan_total = total;
    account->balance += principal;
    
    update_account_db(account);
    
    char msg[200];
    snprintf(msg, sizeof(msg), "Loan Taken: %.2f | Total Payable: %.2f", principal, total);
    add_transaction_db(account->account_no, msg);
    
    printf("✅ Loan Approved!\n");
    printf("Loan Amount: %.2f\n", principal);
    printf("Total Payable: %.2f\n", total);
    
    pause_screen();
}

void repay_loan(Account *account) {
    if (account->loan_total == 0) {
        printf("\n✅ No Active Loan!\n");
        pause_screen();
        return;
    }
    
    printf("\nLoan Due: %.2f\n", account->loan_total);
    
    if (account->balance >= account->loan_total) {
        account->balance -= account->loan_total;
        
        char msg[200];
        snprintf(msg, sizeof(msg), "Loan Repaid: %.2f", account->loan_total);
        add_transaction_db(account->account_no, msg);
        
        account->loan_total = 0;
        account->loan_amount = 0;
        
        update_account_db(account);
        
        printf("✅ Loan Fully Repaid!\n");
    } else {
        printf("❌ Insufficient Balance!\n");
    }
    
    pause_screen();
}

/* ================= MUTUAL FUND FUNCTIONS ================= */

void mutual_fund_menu(Account *account) {
    int choice;
    float amount;
    float rate;
    int probability;
    
    system("clear || cls");
    print_header();
    
    printf("Mutual Fund Investment Options:\n");
    printf("1. Low Risk (6%%) - 80%% Success Rate\n");
    printf("2. Medium Risk (10%%) - 70%% Success Rate\n");
    printf("3. High Risk (15%%) - 60%% Success Rate\n");
    printf("4. Withdraw Investment\n");
    printf("5. Back\n");
    
    printf("\nEnter choice: ");
    scanf("%d", &choice);
    
    if (choice >= 1 && choice <= 3) {
        printf("Enter investment amount: ");
        scanf("%f", &amount);
        
        if (amount > account->balance) {
            printf("❌ Insufficient Balance!\n");
        } else if (amount > 0) {
            if (choice == 1) { rate = 6; probability = 80; }
            else if (choice == 2) { rate = 10; probability = 70; }
            else { rate = 15; probability = 60; }
            
            account->balance -= amount;
            
            int random = rand() % 100;
            
            if (random < probability) {
                float profit = amount * (rate / 100);
                account->mutual_fund_investment += amount + profit;
                printf("✅ Investment Successful! You earned profit.\n");
                printf("Profit: %.2f\n", profit);
            } else {
                account->mutual_fund_investment += amount * 0.7;
                printf("⚠️ Market Loss! Retained 70%% of amount.\n");
            }
            
            update_account_db(account);
            
            char msg[200];
            snprintf(msg, sizeof(msg), "Mutual Fund Invested: %.2f", amount);
            add_transaction_db(account->account_no, msg);
        }
    } else if (choice == 4) {
        if (account->mutual_fund_investment > 0) {
            account->balance += account->mutual_fund_investment;
            update_account_db(account);
            
            char msg[200];
            snprintf(msg, sizeof(msg), "Mutual Fund Withdrawn: %.2f", account->mutual_fund_investment);
            add_transaction_db(account->account_no, msg);
            
            printf("✅ Investment Withdrawn! Amount: %.2f\n", account->mutual_fund_investment);
            account->mutual_fund_investment = 0;
            update_account_db(account);
        } else {
            printf("✅ No investment to withdraw!\n");
        }
    }
    
    pause_screen();
}

/* ================= MAIN MENU FUNCTIONS ================= */

void user_dashboard(Account *account) {
    int choice;
    
    do {
        system("clear || cls");
        print_header();
        
        printf("Welcome, %s\n", account->name);
        printf("Account Number: %d\n", account->account_no);
        print_separator();
        printf("Current Balance: %.2f\n", account->balance);
        printf("Loan Due: %.2f\n", account->loan_total);
        printf("Mutual Fund: %.2f\n", account->mutual_fund_investment);
        print_separator();
        
        printf("\n1. Deposit\n");
        printf("2. Withdraw\n");
        printf("3. Transfer Money\n");
        printf("4. Apply Loan\n");
        printf("5. Repay Loan\n");
        printf("6. Mutual Fund\n");
        printf("7. Logout\n");
        
        printf("\nEnter choice: ");
        scanf("%d", &choice);
        
        switch (choice) {
            case 1: deposit(account); break;
            case 2: withdraw(account); break;
            case 3: transfer_money(account); break;
            case 4: apply_loan(account); break;
            case 5: repay_loan(account); break;
            case 6: mutual_fund_menu(account); break;
        }
        
    } while (choice != 7);
}

void user_login() {
    int account_no, pin;
    Account account;
    
    system("clear || cls");
    print_header();
    
    printf("Enter Account Number: ");
    scanf("%d", &account_no);
    
    printf("Enter PIN: ");
    scanf("%d", &pin);
    
    if (find_account_db(account_no, &account) && 
        account.encrypted_pin == encrypt_decrypt(pin)) {
        user_dashboard(&account);
    } else {
        printf("\n❌ Invalid Credentials!\n");
        pause_screen();
    }
}

void admin_dashboard() {
    Account **accounts_arr = (Account **)malloc(MAX_ACCOUNTS * sizeof(Account *));
    int count = get_all_accounts_db(accounts_arr);
    
    system("clear || cls");
    print_header();
    
    printf("Total Accounts: %d\n\n", count);
    
    for (int i = 0; i < count; i++) {
        printf("AccNo: %d | Name: %-20s | Balance: %.2f | Loan: %.2f\n",
               accounts_arr[i]->account_no,
               accounts_arr[i]->name,
               accounts_arr[i]->balance,
               accounts_arr[i]->loan_total);
    }
    
    for (int i = 0; i < count; i++) {
        free(accounts_arr[i]);
    }
    free(accounts_arr);
    
    pause_screen();
}

void admin_login() {
    int password;
    
    printf("Enter Admin Password: ");
    scanf("%d", &password);
    
    if (password == ADMIN_PASSWORD) {
        admin_dashboard();
    } else {
        printf("❌ Wrong Password!\n");
        pause_screen();
    }
}

void main_menu() {
    int choice;
    
    while (1) {
        system("clear || cls");
        print_header();
        
        printf("1. Create Account\n");
        printf("2. User Login\n");
        printf("3. Admin Login\n");
        printf("4. Exit\n");
        
        printf("\nEnter choice: ");
        scanf("%d", &choice);
        
        switch (choice) {
            case 1: create_account(); break;
            case 2: user_login(); break;
            case 3: admin_login(); break;
            case 4:
                printf("\nThank you for using Secure Future Bank!\n");
                exit(0);
            default:
                printf("❌ Invalid Choice!\n");
                pause_screen();
        }
    }
}

/* ================= MAIN FUNCTION ================= */

int main() {
    srand(time(0));
    
    if (!init_database()) {
        printf("Failed to initialize database\n");
        return 1;
    }
    
    main_menu();
    
    sqlite3_close(db);
    return 0;
}
