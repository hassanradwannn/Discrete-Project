#include <iostream>  // Include iostream for input/output
#include <string>  // Include string for string handling

using namespace std;  // Use standard namespace

const int MAX_VARS = 5;  // Maximum number of variables
const int MAX_PREMISES = 3;  // Maximum number of premises
const int MAX_FORMULAS = MAX_PREMISES + 1;  // Maximum number of formulas (premises + conclusion)
const int MAX_TOKENS = 32;  // Maximum number of tokens
const int MAX_ROWS = 1 << MAX_VARS;  // Maximum number of rows in truth table (2^MAX_VARS)

struct Formula {  // Define struct for formula
    string name;  // Name of the formula
    string tokens[MAX_TOKENS];  // Array of tokens
    int tokenCount;  // Number of tokens
};

// Convert a bool to "T"/"F" for compact table printing.
string boolToString(bool v) { return v ? "T" : "F"; }  // Function to convert bool to string

// Lowercase a string (ASCII) to make variable and keyword handling case-insensitive.
string toLowerSimple(const string& s) {  // Function to lowercase string
    string out = s;  // Copy input string
    // Walk each character to lowercase A-Z.
    for (size_t i = 0; i < out.size(); ++i) {  // Loop through each character
        char c = out[i];  // Get current character
        if (c >= 'A' && c <= 'Z') out[i] = char(c + ('a' - 'A'));  // If uppercase, make lowercase
    }
    return out;  // Return the lowercased string
}

// Logical NOT.
bool opNot(bool p) { return !p; }  // Function for logical NOT
// Logical AND.
bool opAnd(bool p, bool q) { return p && q; }  // Function for logical AND
// Logical OR.
bool opOr(bool p, bool q) { return p || q; }  // Function for logical OR
// Logical implication p -> q.
bool opImplies(bool p, bool q) { return !p || q; }  // Function for implication

// Split a symbol-mode expression into tokens (operators/parentheses/identifiers), lowercasing identifiers.
int splitTokens(const string& line, string outTokens[MAX_TOKENS]) {  // Function to split expression into tokens
    int count = 0;  // Token count
    string current = "";  // Current token being built
    auto flush = [&](void) {  // Lambda to add current token to output
        if (!current.empty() && count < MAX_TOKENS) {  // If current is not empty and not exceeding max
            outTokens[count++] = current;  // Add to tokens
            current.clear();  // Clear current
        }
    };

    // Walk characters, emitting operators/parens immediately and grouping letters/digits.
    for (size_t i = 0; i < line.size(); ++i) {  // Loop through each character in line
        char c = line[i];  // Get current character
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {  // If whitespace
            flush();  // Flush current token
            continue;  // Skip
        }
        if (c == '(' || c == ')' || c == '&' || c == '|' || c == '>' || c == '!') {  // If operator or paren
            flush();  // Flush current
            if (count < MAX_TOKENS) {  // If not exceeding max
                string t; t.push_back(c);  // Create string with char
                outTokens[count++] = t;  // Add to tokens
            }
        } else {  // Else, part of identifier
            // case-sensitivity fix
            if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));  // Lowercase if uppercase
            current.push_back(c);  // Add to current token
        }
    }
    flush();  // Flush any remaining
    return count;  // Return token count
}


// Normalize input to symbols if English mode is on; otherwise just lowercase.
string normalizeExpression(const string& in, bool englishMode) {  // Function to normalize expression
    if (!englishMode) return toLowerSimple(in);  // If not english mode, just lowercase

    string out = "";  // Output string
    string word = "";  // Current word
    auto flushWord = [&](void) {  // Lambda to process word
        if (word.empty()) return;  // If empty, return
        string w = toLowerSimple(word);  // Lowercase word
        if (w == "not" || w == "no" || w == "~") {  // If not
            out += "! ";  // Add !
        } else if (w == "and" || w == "&&") {  // If and
            out += "& ";  // Add &
        } else if (w == "or" || w == "||") {  // If or
            out += "| ";  // Add |
        } else if (w == "implies" || w == "then" || w == "if" || w == "=>" || w == ">") {  // If implies
            out += "> ";  // Add >
        } else {  // Else, variable
            out += w + " ";  // Add word
        }
        word.clear();  // Clear word
    };

    // Walk characters, mapping keywords/operators to symbols, leaving variables as words.
    for (size_t i = 0; i < in.size(); ++i) {  // Loop through input
        char c = in[i];  // Get char
        if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));  // Lowercase
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {  // If space
            flushWord();  // Flush word
        } else if (c == '(' || c == ')' || c == '&' || c == '|' || c == '>' || c == '!' || c == '~') {  // If operator
            flushWord();  // Flush
            if (c == '~') c = '!';  // Convert ~ to !
            out.push_back(c);  // Add char
            out.push_back(' ');  // Add space
        } else if (c == '=' && i + 1 < in.size() && in[i + 1] == '>') {  // If =>
            flushWord();  // Flush
            out += "> ";  // Add >
            ++i;  // Skip next
        } else {  // Else
            word.push_back(c);  // Add to word
        }
    }
    flushWord();  // Flush last
    return out;  // Return normalized
}

// Return operator precedence (higher number = tighter bind).
int precedence(const string& op) {  // Function for precedence
    if (op == "!") return 3;  // NOT highest
    if (op == "&") return 2;  // AND
    if (op == "|") return 1;  // OR
    if (op == ">") return 0;  // IMPLIES lowest
    return -1;  // Invalid
}

// Identify right-associative operators (NOT, IMPLIES).
bool isRightAssociative(const string& op) {  // Function for associativity
    return op == "!" || op == ">";  // NOT and IMPLIES are right assoc
}

// convert infix tokens to postfix to avoid recursion
// Convert infix tokens to postfix so evaluation can use a small stack.
void infixToPostfix(const string inTokens[MAX_TOKENS], int inCount,  // Function to convert infix to postfix
                    string outTokens[MAX_TOKENS], int& outCount,  // Output tokens and count
                    bool& ok) {  // Success flag
    string opStack[MAX_TOKENS];  // Operator stack
    int opTop = 0;  // Stack top
    outCount = 0;  // Output count
    ok = true;  // Assume ok

    // Scan infix tokens, shunting operators to produce postfix order.
    for (int i = 0; i < inCount; ++i) {  // Loop through input tokens
        const string& t = inTokens[i];  // Current token
        if (t == "(") {  // If left paren
            opStack[opTop++] = t;  // Push to stack
        } else if (t == ")") {  // If right paren

            bool found = false;  // Flag for matching paren
            while (opTop > 0) {  // While stack not empty
                string top = opStack[--opTop];  // Pop
                if (top == "(") { found = true; break; }  // If left paren, done
                outTokens[outCount++] = top;  // Else, output
            }
            if (!found) { ok = false; return; }  // If no matching, error
        } else if (t == "!" || t == "&" || t == "|" || t == ">") {  // If operator
            int p = precedence(t);  // Get precedence
            while (opTop > 0) {  // While stack has ops
                string top = opStack[opTop - 1];  // Peek top
                int pt = precedence(top);  // Precedence of top
                if (pt == -1 || top == "(") break;  // If not op or paren, stop
                if ((pt > p) || (pt == p && !isRightAssociative(t))) {  // If higher prec or same and left assoc
                    outTokens[outCount++] = opStack[--opTop];  // Pop to output
                } else {  // Else
                    break;  // Stop
                }
            }
            opStack[opTop++] = t;  // Push current op
        } else {  // Else, operand
            // variable
            outTokens[outCount++] = t;  // Output variable
        }
    }

    // Drain remaining operators to the output.
    while (opTop > 0) {  // While stack not empty
        string top = opStack[--opTop];  // Pop
        if (top == "(" || top == ")") { ok = false; return; }  // If paren, error
        outTokens[outCount++] = top;  // Output
    }
}


bool evaluateRPN(const Formula& f,  // Function to evaluate RPN
                  const string varNames[MAX_VARS],  // Variable names
                  const bool values[MAX_VARS],  // Variable values
                  int varCount,  // Number of variables
                  bool& ok) {  // Success flag
    // Evaluate a postfix formula using the current row assignment.
    bool stack[MAX_TOKENS];  // Evaluation stack
    int top = 0;  // Stack top
    ok = true;  // Assume ok

    // Process each postfix token, pushing variables, applying ops.
    for (int i = 0; i < f.tokenCount; ++i) {  // Loop through tokens
        const string& t = f.tokens[i];  // Current token
        if (t == "!") {  // If NOT
            if (top < 1) { ok = false; break; }  // If not enough operands, error
            bool a = stack[--top];  // Pop operand
            stack[top++] = opNot(a);  // Apply NOT and push
        } else if (t == "&") {  // If AND
            if (top < 2) { ok = false; break; }  // Need 2 operands
            bool b = stack[--top];  // Pop second
            bool a = stack[--top];  // Pop first
            stack[top++] = opAnd(a, b);  // Apply AND and push
        } else if (t == "|") {  // If OR
            if (top < 2) { ok = false; break; }  // Need 2
            bool b = stack[--top];  // Pop b
            bool a = stack[--top];  // Pop a
            stack[top++] = opOr(a, b);  // Apply OR
        } else if (t == ">") {  // If IMPLIES
            if (top < 2) { ok = false; break; }  // Need 2
            bool b = stack[--top];  // Pop b
            bool a = stack[--top];  // Pop a
            stack[top++] = opImplies(a, b);  // Apply IMPLIES
        } else {  // Else, variable
            // variable lookup
            int idx = -1;  // Index
            // Linear scan to find matching variable name.
            for (int k = 0; k < varCount; ++k) {  // Loop through variables
                if (varNames[k] == t) { idx = k; break; }  // If match, set index
            }
            if (idx == -1) { ok = false; break; }  // If not found, error
            stack[top++] = values[idx];  // Push value
        }
    }
    if (!ok || top != 1) {  // If error or not one result
        ok = false;  // Set error
        return false;  // Return false
    }
    return stack[0];  // Return result
}

// truth table builder
void buildTruthTable(const string varNames[MAX_VARS],  // Function to build truth table
                      int varCount,  // Number of variables
                      const Formula formulas[MAX_FORMULAS],  // Formulas
                      int formulaCount,  // Number of formulas
                      bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS],  // Table array
                      int& rowCount,  // Row count
                      bool& ok) {  // Success
    // Fill the 2D truth table: variables first, then each formula per row.
    ok = true;  // Assume ok
    rowCount = 1 << varCount;  // 2^varCount rows
    // Iterate all 2^n assignments via bitmask.
    for (int mask = 0; mask < rowCount; ++mask) {  // For each mask
        bool assignment[MAX_VARS];  // Assignment array
        // Map bits to variable truth values.
        for (int i = 0; i < varCount; ++i) {  // For each variable
            assignment[i] = (mask >> (varCount - 1 - i)) & 1;  // Set value from bit
            table[mask][i] = assignment[i];  // Store in table
        }

        // Evaluate each formula for this assignment.
        for (int f = 0; f < formulaCount; ++f) {  // For each formula
            bool good = true;  // Good flag
            bool val = evaluateRPN(formulas[f], varNames, assignment, varCount, good);  // Evaluate
            if (!good) { ok = false; return; }  // If error, fail
            table[mask][varCount + f] = val;  // Store result
        }
    }
}

void printTruthTable(const string varNames[MAX_VARS],  // Function to print truth table
                      int varCount,  // Var count
                      const Formula formulas[MAX_FORMULAS],  // Formulas
                      int formulaCount,  // Formula count
                      bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS],  // Table
                      int rowCount) {  // Row count
    // Pretty-print the truth table from the stored 2D array.
    auto pad = [](const string& s, int w) {  // Lambda to pad string
        string out = s;  // Copy
        if ((int)out.size() < w) out.append(w - out.size(), ' ');  // Pad with spaces
        return out;  // Return
    };

    cout << "\n                 TRUTH TABLE                 \n";  // Print header
    const int width = 10;  // Column width
    for (int i = 0; i < varCount; ++i) cout << pad(varNames[i], width);  // Print var names
    for (int f = 0; f < formulaCount; ++f) cout << pad(formulas[f].name, width);  // Print formula names
    cout << "\n";  // New line

    // Print each row of vars + formulas.
    for (int r = 0; r < rowCount; ++r) {  // For each row
        for (int c = 0; c < varCount + formulaCount; ++c) {  // For each column
            cout << pad(boolToString(table[r][c]), width);  // Print value
        }
        cout << "\n";  // New line
    }
}

void analyzeArgument(const string varNames[MAX_VARS],  // Function to analyze argument
                      bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS],  // Table
                      int rowCount,  // Row count
                      int varCount,  // Var count
                      int premiseCount,  // Premise count
                      int formulaCount) {  // Formula count
    // Scan the table to check satisfiability and find counterexamples to validity.
    bool valid = true;  // Assume valid
    bool satisfiable = false;  // Assume not satisfiable
    bool counterFound = false;  // No counter yet
    int counterRow = -1;  // Counter row
    int conclusionIdx = varCount + formulaCount - 1;  // Index of conclusion

    // Inspect every row for satisfiability and counterexamples.
    for (int r = 0; r < rowCount; ++r) {  // For each row
        bool allPremises = true;  // Assume all premises true
        // Check all premises in this row.
        for (int p = 0; p < premiseCount; ++p) {  // For each premise
            allPremises = allPremises && table[r][varCount + p];  // AND with premise value
        }
        bool conclusion = table[r][conclusionIdx];  // Get conclusion
        if (allPremises && !conclusion) {  // If premises true but conclusion false
            valid = false;  // Not valid
            if (!counterFound) {  // If first counter
                counterFound = true;  // Found
                counterRow = r;  // Set row
            }
        }
        if (allPremises && conclusion) satisfiable = true;  // If satisfiable
    }

    cout << "\n                 ANALYSIS                     \n";  // Print analysis header
    cout << "Satisfiable: " << (satisfiable ? "Yes" : "No") << "\n";  // Print satisfiable
    if (valid) {  // If valid
        cout << "Valid: Yes (no counterexample)\n";  // Valid
                    } else {  // Else
        cout << "Valid: Falsifiable (counterexample found)\n";  // Not valid
        if (counterFound) {  // If counter found
            cout << "Counterexample: \n";  // Print counter
    // Report the first counterexample
    for (int i = 0; i < varCount; ++i) {  // For each var
                cout << "  " << varNames[i] << " = " << boolToString(table[counterRow][i]) << "\n";  // Print assignment
            }
        }
    }
}

void interactiveMode() {  // Function for interactive mode
    // ask user for variables, mode (symbols vs English), premises, and conclusion; then analyze.
    cout << "Operators: ! (NOT)  & (AND)  | (OR)  > (IMPLIES)\n";  // Print operators
    cout << "You can type symbols directly, or choose English keywords\n";  // Print options
    cout << "(not/no, and, or, implies/then)\n";  // Keywords
    cout << "Example symbols: ( p | q ) > ! r\n";  // Example
    cout << "Example English: p and q then not r\n";  // Example

    int varCount;  // Variable count
    cout << "How many variables? (1-" << MAX_VARS << "): ";  // Ask for var count
    cin >> varCount;  // Read
    cin.ignore();  // Ignore newline
    if (varCount < 1 || varCount > MAX_VARS) {  // If invalid
        cout << "Invalid variable count.\n";  // Error
        return;  // Return
    }

    bool englishMode = false;  // English mode flag
    cout << "Use English keywords instead of symbols? (yes or no): ";  // Ask for mode
    string modeChoice; cin >> modeChoice; cin.ignore();  // Read choice
    if (modeChoice == "yes" || modeChoice == "YES" || modeChoice == "y" || modeChoice == "Y") englishMode = true;  // Set if yes

    string vars[MAX_VARS];  // Variable names
    // Read variable names (single letters in symbol mode, words allowed in English mode).
    for (int i = 0; i < varCount; ++i) {  // For each var
        cout << "Name for variable " << (i + 1) << ": ";  // Ask name
        string tmp; getline(cin, tmp);  // Read line
        tmp = toLowerSimple(tmp);  // Lowercase
        if (!englishMode && tmp.size() != 1) {  // If symbol mode and not single letter
            cout << "In symbol mode, use single-letter variable names.\n";  // Error
            return;  // Return
        }
        vars[i] = tmp;  // Set name
    }

    int premiseCount;  // Premise count
    cout << "Number of premises (0-" << MAX_PREMISES << "): ";  // Ask premise count
    cin >> premiseCount;  // Read
    cin.ignore();  // Ignore
    if (premiseCount < 0 || premiseCount > MAX_PREMISES) {  // If invalid
        cout << "Invalid premise count.\n";  // Error
        return;  // Return
    }

    Formula formulas[MAX_FORMULAS];  // Formulas array
    int formulaCount = premiseCount + 1;  // Total formulas
    // Read each premise, normalize (if English mode), tokenize, convert to postfix.
    for (int i = 0; i < premiseCount; ++i) {  // For each premise
        cout << "Premise " << (i + 1) << ": ";  // Ask premise
        string line; getline(cin, line);  // Read
        formulas[i].name = "P" + to_string(i + 1);  // Set name
        string infix[MAX_TOKENS];  // Infix tokens
        string normalized = normalizeExpression(line, englishMode);  // Normalize
        int ic = splitTokens(normalized, infix);  // Split
        bool okConv = true;  // Ok flag
        infixToPostfix(infix, ic, formulas[i].tokens, formulas[i].tokenCount, okConv);  // Convert
        if (!okConv) { cout << "Malformed premise.\n"; return; }  // If error
    }

    cout << "Conclusion: ";  // Ask conclusion
    string concl; getline(cin, concl);  // Read
    formulas[premiseCount].name = "Conclusion";  // Set name
    string infixC[MAX_TOKENS];  // Infix for conclusion
    string normalizedC = normalizeExpression(concl, englishMode);  // Normalize
    int icc = splitTokens(normalizedC, infixC);  // Split
    bool okConvC = true;  // Ok
    infixToPostfix(infixC, icc, formulas[premiseCount].tokens, formulas[premiseCount].tokenCount, okConvC);  // Convert
    if (!okConvC) { cout << "Malformed conclusion.\n"; return; }  // If error

    bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS];  // Table
    int rowCount = 0;  // Row count
    bool ok = true;  // Ok
    buildTruthTable(vars, varCount, formulas, formulaCount, table, rowCount, ok);  // Build table
    if (!ok) {  // If error
        cout << "Error: malformed expression or unknown variable.\n";  // Error
        return;  // Return
    }

    printTruthTable(vars, varCount, formulas, formulaCount, table, rowCount);  // Print table
    analyzeArgument(vars, table, rowCount, varCount, premiseCount, formulaCount);  // Analyze
}

int main() {  // Main function
    // Entry point: run interactive mode only.
    cout << "                 ARGUMENT VALIDATOR             \n";  // Print title
    cout << "                 Hassan Radwan 25P0391                \n";  // Print name
        interactiveMode();  // Run interactive
    cout << "Thank You!\n";  // Thank you
    return 0;  // Return 0
}
