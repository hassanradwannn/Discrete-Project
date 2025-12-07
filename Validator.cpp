#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;

bool notGate(bool p) {
    return !p;
}

bool andGate(bool p, bool q) {
    return p && q;
}


bool orGate(bool p, bool q) {
    return p || q;
}


bool impliedBy(bool p, bool q) {
    return !p || q;
}

bool evaluatePremise1(bool K, bool M, bool A) {
    bool leftSide = orGate(K, M);      // (K OR M)
    bool rightSide = notGate(A);        // (NOT A)
    return impliedBy(leftSide, rightSide);  // (K OR M) IMPLIES (NOT A)
}


bool evaluatePremise2(bool A, bool M) {
    return orGate(A, M);
}


bool evaluateConclusion(bool A, bool K) {
    bool notK = notGate(K);
    return orGate(A, notK);
}


bool evaluateAllPremises(bool K, bool M, bool A) {
    bool p1 = evaluatePremise1(K, M, A);
    bool p2 = evaluatePremise2(A, M);
    return andGate(p1, p2);
}

string boolToString(bool value) {
    return value ? "T" : "F";
}

bool getTruthValue(bool& result) {
    string input;
    cin >> input;
    
    // Check if input is "1" or "0"
    if (input == "1") {
        result = true;
        return true;
    } else if (input == "0") {
        result = false;
        return true;
    }
    
    
    string lowerInput = "";
    for (int i = 0; i < input.length(); i++) {
        char c = input[i];
        if (c >= 'A' && c <= 'Z') {
            lowerInput += (c + 32);  // Convert to lowercase using ASCII values due to case sensitivity
        } else {
            lowerInput += c;
        }
    }
    

    if (lowerInput == "true") {
        result = true;
        return true;
    } else if (lowerInput == "false") {
        result = false;
        return true;
    }
    
    
    return false;
}


void printTableHeader() {
    cout << "\n" << string(80, '=') << "\n";
    cout << "TRUTH TABLE\n";   
    cout << left << setw(8) << "K" 
         << setw(8) << "M" 
         << setw(8) << "A" 
         << setw(15) << "Premise 1" 
         << setw(15) << "Premise 2" 
         << setw(15) << "Conclusion" 
         << setw(15) << "All Premises" << "\n";
}


void generateTruthTable(bool& isValid, bool& isSatisfiable) {
    printTableHeader();
    
    isValid = true;           
    isSatisfiable = false;    
    
    for (int i = 0; i < 8; i++) {
        bool K = (i & 4) != 0;  
        bool M = (i & 2) != 0;  
        bool A = (i & 1) != 0;  
        
        bool premise1 = evaluatePremise1(K, M, A);
        bool premise2 = evaluatePremise2(A, M);
        bool conclusion = evaluateConclusion(A, K);
        bool allPremises = evaluateAllPremises(K, M, A);
        
        cout << left << setw(8) << boolToString(K)
             << setw(8) << boolToString(M)
             << setw(8) << boolToString(A)
             << setw(15) << boolToString(premise1)
             << setw(15) << boolToString(premise2)
             << setw(15) << boolToString(conclusion)
             << setw(15) << boolToString(allPremises) << "\n";
        
        if (allPremises && !conclusion) {
            isValid = false;
        }
        
        if (allPremises && conclusion) {
            isSatisfiable = true;
        }
    }
    
    
}

void printResults(bool isValid, bool isSatisfiable) {
    cout << "\n" << string(80, '=') << "\n";
    cout << "ANALYSIS RESULTS\n";
    
    cout << "\nSATISFIABILITY CHECK:\n";
    
    if (isSatisfiable) {
        cout << "Result: The set of formulas is SATISFIABLE\n";
        cout << "Explanation: There exists at least one row where all premises and\n";
        cout << "the conclusion are simultaneously true.\n";
    } else {
        cout << "Result: The set of formulas is NOT SATISFIABLE\n";
        cout << "Explanation: There is no row where all premises and the conclusion\n";
        cout << "are simultaneously true.\n";
    }
    
    cout << "\nVALIDITY CHECK:\n";
    
    if (isValid) {
        cout << "Result: The argument is VALID\n";
        cout << "Explanation: In all rows where all premises are true, the conclusion is also true.\n";
    } else {
        cout << "Result: The argument is FALSIFIABLE\n";
        cout << "Explanation: There exists at least one row where all premises are true\n";
        cout << "but the conclusion is false (counterexample found).\n";
    }
    
    cout << "\n" << string(80, '=') << "\n";
}

void evaluateSingleCase(bool K, bool M, bool A) {
    cout << "\n" << string(80, '=') << "\n";
    cout << "EVALUATION FOR SPECIFIC VALUES\n";
    cout << string(80, '=') << "\n";
    
    bool premise1 = evaluatePremise1(K, M, A);
    bool premise2 = evaluatePremise2(A, M);
    bool conclusion = evaluateConclusion(A, K);
    bool allPremises = evaluateAllPremises(K, M, A);
    
    cout << "\nVariable Assignments:\n";
    cout << "K (Khalid) = " << boolToString(K) << "\n";
    cout << "M (Mariam) = " << boolToString(M) << "\n";
    cout << "A (Aly) = " << boolToString(A) << "\n";
    
    cout << "\n" << string(80, '-') << "\n";
    cout << "Evaluation Results:\n";
    cout << string(80, '-') << "\n";
    cout << "Premise 1: (K OR M) IMPLIES (NOT A) = " << boolToString(premise1) << "\n";
    cout << "Premise 2: A OR M = " << boolToString(premise2) << "\n";
    cout << "Conclusion: A OR (NOT K) = " << boolToString(conclusion) << "\n";
    cout << "All Premises (P1 AND P2) = " << boolToString(allPremises) << "\n";
    
    
    if (allPremises && !conclusion) {
        cout << "\n*** This case shows the argument is FALSIFIABLE ***\n";
        cout << "All premises are true, but the conclusion is false.\n";
    } else if (allPremises && conclusion) {
        cout << "\n*** This case satisfies all formulas ***\n";
        cout << "All premises and the conclusion are true.\n";
    } else {
        cout << "\nThis case does not affect validity (premises are not all true).\n";
    }
    
    cout << string(80, '=') << "\n";
}


void interactiveMode() {
    cout << "\n" << string(80, '=') << "\n";
    cout << "INTERACTIVE MODE\n";
    cout << string(80, '=') << "\n";
    cout << "\nThis mode allows you to evaluate custom logical expressions.\n";
    
    char newVariablesChoice = 'y';
    
    while (newVariablesChoice == 'y' || newVariablesChoice == 'Y') {
        int numVariables;
        cout << "Enter the number of variables (1-5): ";
        cin >> numVariables;
        
        if (numVariables < 1) {
            cout << "Invalid number of variables. The number must be at least 1.\n";
            char retryChoice;
            cout << "Would you like to reenter? (y/n): ";
            cin >> retryChoice;
            if (retryChoice == 'y' || retryChoice == 'Y') {
                continue;  
            } else {
                newVariablesChoice = 'n';   
                break;
            }
        } else if (numVariables > 5) {
            cout << "Invalid number of variables. Please enter a number between 1 and 5.\n";
            cout << "Would you like to try again? (y/n): ";
            cin >> newVariablesChoice;
            continue;
        }
        
        vector<string> varNames(numVariables);
        cout << "\nEnter the names for your variables:\n";
        for (int i = 0; i < numVariables; i++) {
            cout << "Enter name for variable " << (i + 1) << " (e.g., P, Q, R): ";
            cin >> varNames[i];
        }
        
        cout << "\nVariables defined: ";
        for (int i = 0; i < numVariables; i++) {
            cout << varNames[i];
            if (i < numVariables - 1) cout << ", ";
        }
        cout << "\n";
        
        
        char continueChoice;
        do {
            cout << "\n" << string(80, '-') << "\n";
            cout << "Enter truth values for the variables (1/0 or true/false):\n";
            vector<bool> variables(numVariables);
            
            
            for (int i = 0; i < numVariables; i++) {
                bool validInput = false;
                while (!validInput) {
                    cout << "Enter truth value for " << varNames[i] << " (1/0 or true/false): ";
                    bool value;
                    if (getTruthValue(value)) {
                        variables[i] = value;
                        validInput = true;
                    } else {
                        cout << "Invalid input! Please enter 1, 0, true, or false.\n";
                    }
                }
            }
            
            
            cout << "\nCurrent variable assignments:\n";
            for (int i = 0; i < numVariables; i++) {
                cout << varNames[i] << " = " << boolToString(variables[i]) << "\n";
            }
            
            cout << "\nAvailable operations:\n";
            cout << "1. NOT (negation)\n";
            cout << "2. AND (conjunction)\n";
            cout << "3. OR (disjunction)\n";
            cout << "4. IMPLIES (implication)\n";
            cout << "\nExample: To evaluate (P OR Q) -> (NOT R):\n";
            cout << "  - First compute: P OR Q\n";
            cout << "  - Then compute: NOT R\n";
            cout << "  - Finally compute: (P OR Q) IMPLIES (NOT R)\n";
            
            cout << "\nEnter operation number (1-4): ";
            int op;
            cin >> op;
            
            if (op >= 1 && op <= 4) {
                bool result;
                if (op == 1) {
                    
                    if (numVariables < 1) {
                        cout << "Insufficient variables for NOT operation.\n";
                    } else {
                        cout << "\nSelect a variable:\n";
                        for (int i = 0; i < numVariables; i++) {
                            cout << "  " << (i + 1) << ". " << varNames[i] << " (currently " << boolToString(variables[i]) << ")\n";
                        }
                        cout << "Enter variable number (1 to " << numVariables << "): ";
                        int varNum;
                        cin >> varNum;
                        int idx1 = varNum - 1;  
                        if (idx1 >= 0 && idx1 < numVariables) {
                            result = notGate(variables[idx1]);
                            cout << "\nResult: NOT " << varNames[idx1] << " = " << boolToString(result) << "\n";
                        } else {
                            cout << "Invalid variable number.\n";
                        }
                    }
                } else {
                    
                    if (numVariables < 2) {
                        cout << "Insufficient variables for this operation (need at least 2).\n";
                    } else {
                        cout << "\nSelect first variable:\n";
                        for (int i = 0; i < numVariables; i++) {
                            cout << "  " << (i + 1) << ". " << varNames[i] << " (currently " << boolToString(variables[i]) << ")\n";
                        }
                        cout << "Enter first variable number (1 to " << numVariables << "): ";
                        int varNum1;
                        cin >> varNum1;
                        int idx1 = varNum1 - 1; 
                        
                        cout << "\nSelect second variable:\n";
                        for (int i = 0; i < numVariables; i++) {
                            cout << "  " << (i + 1) << ". " << varNames[i] << " (currently " << boolToString(variables[i]) << ")\n";
                        }
                        cout << "Enter second variable number (1 to " << numVariables << "): ";
                        int varNum2;
                        cin >> varNum2;
                        int idx2 = varNum2 - 1; 
                        
                        if (idx1 >= 0 && idx1 < numVariables && idx2 >= 0 && idx2 < numVariables) {
                            bool val1 = variables[idx1];
                            bool val2 = variables[idx2];
                            
                            switch (op) {
                                case 2:
                                    result = andGate(val1, val2);
                                    cout << "\nResult: " << varNames[idx1] << " AND " << varNames[idx2] 
                                         << " = " << boolToString(result) << "\n";
                                    break;
                                case 3:
                                    result = orGate(val1, val2);
                                    cout << "\nResult: " << varNames[idx1] << " OR " << varNames[idx2] 
                                         << " = " << boolToString(result) << "\n";
                                    break;
                                case 4:
                                    result = impliedBy(val1, val2);
                                    cout << "\nResult: " << varNames[idx1] << " IMPLIES " << varNames[idx2] 
                                         << " = " << boolToString(result) << "\n";
                                    break;
                            }
                        } else {
                            cout << "Invalid variable number.\n";
                        }
                    }
                }
            } else {
                cout << "Invalid operation number.\n";
            }
            
            cout << "\nOptions:\n";
            cout << "  y - Evaluate another set of values for these variables\n";
            cout << "  n - Choose new variables\n";
            cout << "  e - Exit interactive mode\n";
            cout << "Enter your choice (y/n/e): ";
            cin >> continueChoice;
            
            if (continueChoice == 'n' || continueChoice == 'N') {
                    
                break;
            } else if (continueChoice == 'e' || continueChoice == 'E') {
                newVariablesChoice = 'n';
                break;
            }
            
        } while (true);  
        
        if (continueChoice == 'e' || continueChoice == 'E') {
            break;
        }
    }
}

int main() {
    
    cout << "Argument Validator\n";
    cout << "Hassan Radwan 25P0391\n";
    
    cout << "\nAssigned Argument:\n";
    cout << "Variables: K (Khalid), M (Mariam), A (Aly)\n";
    cout << "Premise 1: (K OR M) IMPLIES (NOT A)\n";
    cout << "Premise 2: A OR M\n";
    cout << "Conclusion: A OR (NOT K)\n";
    
    
    bool isValid = true;
    bool isSatisfiable = false;
   
    generateTruthTable(isValid, isSatisfiable);
    
   
    printResults(isValid, isSatisfiable);
    

    cout << "\nWould you like to use interactive mode? (y/n): ";
    char choice;
    cin >> choice;
    
    if (choice == 'y' || choice == 'Y') {
        interactiveMode();
    }
    
    cout << "\nAll Done! Thank you!\n";
    
    return 0;
}

