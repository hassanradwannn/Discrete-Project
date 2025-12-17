/* g++ discrete_task3.cpp -o main */
#include <iostream>
#include <string>
#include <algorithm> // For transform (toupper)
#include <cctype>    // For toupper

using namespace std;

string alphabet = "ABCDEFGHILMNOPRSTU"; // 18 letters

// ---------------------- MATHEMATICAL HELPERS ----------------------

// Calculate GCD to check for coprimality, to generate valid a inputs
int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Compute modular inverse of a mod m
int modInverse(int a, int m) {
    a = a % m;
    for (int x = 1; x < m; x++) {
        if ((a * x) % m == 1)
            return x;
    }
    return -1;
}

// Display all valid 'a' keys (coprime to 18)
void showValidKeys() {
    cout << "Valid values for key 'a': ";
    for (int i = 1; i < 18; i++) {
        if (gcd(i, 18) == 1) {
            cout << i << " ";
        }
    }
    cout << endl;
}

// ---------------------- ENCRYPT / DECRYPT ----------------------

string encryptText(int a, int b, const string &text) {
    string result = "";
    for (char ch : text) {
        int pos = alphabet.find(ch);
        if (pos == string::npos) {
            result += ch; 
        } else {
            int x = pos;
            int y = (a * x + b) % 18;
            result += alphabet[y];
        }
    }
    return result;
}

string decryptText(int a, int b, const string &text) {
    int a_inv = modInverse(a, 18);
    // Note: We validate 'a' before calling this, but safety check remains
    if (a_inv == -1) return "Error: Key 'a' is invalid.";

    string result = "";
    for (char ch : text) {
        int pos = alphabet.find(ch);
        if (pos == string::npos) {
            result += ch;
        } else {
            int y = pos;
            // Add 18 before modulo to handle negative results
            int x = (a_inv * (y - b + 18)) % 18; 
            result += alphabet[x];
        }
    }
    return result;
}


// Safely gets an integer from user loop until valid
int getIntegerInput(string prompt) {
    string input;
    int value;
    while (true) {
        cout << prompt;
        getline(cin, input);
        try {
            value = stoi(input);
            return value;
        } catch (...) {
            cout << "Invalid input. Please enter a number." << endl;
        }
    }
}

// ---------------------- MAIN PROGRAM ----------------------
int main() {
    string inputChoice;

    while (true) {
        cout << "\n======================================\n";
        cout << "       AFFINE CIPHER (MOD 18)         \n";
        cout << "======================================\n";
        cout << "Type '1' to Encrypt\n";
        cout << "Type '2' to Decrypt\n";
        cout << "Type 'exit' to quit program\n";
        cout << "--------------------------------------\n";
        cout << "Choice: ";
        
        getline(cin, inputChoice);

        // Convert choice to lowercase to handle "EXIT", "Exit", etc.
        string lowerChoice = inputChoice;
        transform(lowerChoice.begin(), lowerChoice.end(), lowerChoice.begin(), ::tolower);

        if (lowerChoice == "exit") {
            cout << "Exiting program. Goodbye!" << endl;
            break;
        }

        if (inputChoice != "1" && inputChoice != "2") {
            cout << "Invalid command. Please try again." << endl;
            continue; // Restarts the loop
        }

        
        showValidKeys(); 
        int a;
        while (true) {
            a = getIntegerInput("Enter key a (must be from list above): ");
            if (gcd(a, 18) == 1) {
                break;
            }
            cout << "Error: " << a << " is not coprime with 18. Please choose from the valid list." << endl;
        }

        
        int b = getIntegerInput("Enter key b : ");
        // Normalize b to be positive 0-17
        b = (b % 18 + 18) % 18; 

        
        string text;
        cout << "Enter text to process: ";
        getline(cin, text);

        // Convert text to uppercase automatically
        for (char &c : text) {
            c = toupper(c);
        }

        
        if (inputChoice == "1") {
            cout << "\n>>> Result: " << encryptText(a, b, text) << endl;
        } else {
            cout << "\n>>> Result: " << decryptText(a, b, text) << endl;
        }
        
        cout << "\n(Press Enter to continue...)";
        cin.ignore(); 
    }

    return 0;
}