
#include <iostream>
#include <string>

using namespace std;


const int MAX_VARS = 5;
const int MAX_PREMISES = 3;
const int MAX_FORMULAS = MAX_PREMISES + 1;     
const int MAX_TOKENS = 32;                     
const int MAX_ROWS = 1 << MAX_VARS;            

struct Formula {
    string name;
    string tokens[MAX_TOKENS];
    int tokenCount;
};

// Convert a bool to "T"/"F" for compact table printing.
string boolToString(bool v) { return v ? "T" : "F"; }

// Lowercase a string (ASCII) to make variable and keyword handling case-insensitive.
string toLowerSimple(const string& s) {
    string out = s;
    // Walk each character to lowercase A-Z.
    for (size_t i = 0; i < out.size(); ++i) {
        char c = out[i];
        if (c >= 'A' && c <= 'Z') out[i] = char(c + ('a' - 'A'));
    }
    return out;
}

// Logical NOT.
bool opNot(bool p) { return !p; }
// Logical AND.
bool opAnd(bool p, bool q) { return p && q; }
// Logical OR.
bool opOr(bool p, bool q) { return p || q; }
// Logical implication p -> q.
bool opImplies(bool p, bool q) { return !p || q; }

// Split a symbol-mode expression into tokens (operators/parentheses/identifiers), lowercasing identifiers.
int splitTokens(const string& line, string outTokens[MAX_TOKENS]) {
    int count = 0;
    string current = "";
    auto flush = [&](void) {
        if (!current.empty() && count < MAX_TOKENS) {
            outTokens[count++] = current;
            current.clear();
        }
    };

    // Walk characters, emitting operators/parens immediately and grouping letters/digits.
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            flush();
            continue;
        }
        if (c == '(' || c == ')' || c == '&' || c == '|' || c == '>' || c == '!') {
            flush();
            if (count < MAX_TOKENS) {
                string t; t.push_back(c);
                outTokens[count++] = t;
            }
        } else {
            // case-sensitivity fix
            if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
            current.push_back(c);
        }
    }
    flush();
    return count;
}


// Normalize input to symbols if English mode is on; otherwise just lowercase.
string normalizeExpression(const string& in, bool englishMode) {
    if (!englishMode) return toLowerSimple(in);

    string out = "";
    string word = "";
    auto flushWord = [&](void) {
        if (word.empty()) return;
        string w = toLowerSimple(word);
        if (w == "not" || w == "no" || w == "~") {
            out += "! ";
        } else if (w == "and" || w == "&&") {
            out += "& ";
        } else if (w == "or" || w == "||") {
            out += "| ";
        } else if (w == "implies" || w == "then" || w == "if" || w == "=>" || w == ">") {
            out += "> ";
        } else {
            out += w + " ";
        }
        word.clear();
    };

    // Walk characters, mapping keywords/operators to symbols, leaving variables as words.
    for (size_t i = 0; i < in.size(); ++i) {
        char c = in[i];
        if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            flushWord();
        } else if (c == '(' || c == ')' || c == '&' || c == '|' || c == '>' || c == '!' || c == '~') {
            flushWord();
            if (c == '~') c = '!';
            out.push_back(c);
            out.push_back(' ');
        } else if (c == '=' && i + 1 < in.size() && in[i + 1] == '>') {
            flushWord();
            out += "> ";
            ++i;
        } else {
            word.push_back(c);
        }
    }
    flushWord();
    return out;
}

// Return operator precedence (higher number = tighter bind).
int precedence(const string& op) {
    if (op == "!") return 3;
    if (op == "&") return 2;
    if (op == "|") return 1;
    if (op == ">") return 0; 
    return -1;
}

// Identify right-associative operators (NOT, IMPLIES).
bool isRightAssociative(const string& op) {
    return op == "!" || op == ">";
}

// convert infix tokens to postfix to avoid recursion
// Convert infix tokens to postfix so evaluation can use a small stack.
void infixToPostfix(const string inTokens[MAX_TOKENS], int inCount,
                    string outTokens[MAX_TOKENS], int& outCount,
                    bool& ok) {
    string opStack[MAX_TOKENS];
    int opTop = 0;
    outCount = 0;
    ok = true;

    // Scan infix tokens, shunting operators to produce postfix order.
    for (int i = 0; i < inCount; ++i) {
        const string& t = inTokens[i];
        if (t == "(") {
            opStack[opTop++] = t;
        } else if (t == ")") {
            
            bool found = false;
            while (opTop > 0) {
                string top = opStack[--opTop];
                if (top == "(") { found = true; break; }
                outTokens[outCount++] = top;
            }
            if (!found) { ok = false; return; }
        } else if (t == "!" || t == "&" || t == "|" || t == ">") {
            int p = precedence(t);
            while (opTop > 0) {
                string top = opStack[opTop - 1];
                int pt = precedence(top);
                if (pt == -1 || top == "(") break;
                if ((pt > p) || (pt == p && !isRightAssociative(t))) {
                    outTokens[outCount++] = opStack[--opTop];
                } else {
                    break;
                }
            }
            opStack[opTop++] = t;
        } else {
            // variable
            outTokens[outCount++] = t;
        }
    }

    // Drain remaining operators to the output.
    while (opTop > 0) {
        string top = opStack[--opTop];
        if (top == "(" || top == ")") { ok = false; return; }
        outTokens[outCount++] = top;
    }
}


bool evaluateRPN(const Formula& f,
                 const string varNames[MAX_VARS],
                 const bool values[MAX_VARS],
                 int varCount,
                 bool& ok) {
    // Evaluate a postfix formula using the current row assignment.
    bool stack[MAX_TOKENS];
    int top = 0;
    ok = true;

    // Process each postfix token, pushing variables, applying ops.
    for (int i = 0; i < f.tokenCount; ++i) {
        const string& t = f.tokens[i];
        if (t == "!") {
            if (top < 1) { ok = false; break; }
            bool a = stack[--top];
            stack[top++] = opNot(a);
        } else if (t == "&") {
            if (top < 2) { ok = false; break; }
            bool b = stack[--top];
            bool a = stack[--top];
            stack[top++] = opAnd(a, b);
        } else if (t == "|") {
            if (top < 2) { ok = false; break; }
            bool b = stack[--top];
            bool a = stack[--top];
            stack[top++] = opOr(a, b);
        } else if (t == ">") {
            if (top < 2) { ok = false; break; }
            bool b = stack[--top];
            bool a = stack[--top];
            stack[top++] = opImplies(a, b);
        } else {
            // variable lookup
            int idx = -1;
            // Linear scan to find matching variable name.
            for (int k = 0; k < varCount; ++k) {
                if (varNames[k] == t) { idx = k; break; }
            }
            if (idx == -1) { ok = false; break; }
            stack[top++] = values[idx];
        }
    }
    if (!ok || top != 1) {
        ok = false;
        return false;
    }
    return stack[0];
}

// truth table builder
void buildTruthTable(const string varNames[MAX_VARS],
                     int varCount,
                     const Formula formulas[MAX_FORMULAS],
                     int formulaCount,
                     bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS],
                     int& rowCount,
                     bool& ok) {
    // Fill the 2D truth table: variables first, then each formula per row.
    ok = true;
    rowCount = 1 << varCount;
    // Iterate all 2^n assignments via bitmask.
    for (int mask = 0; mask < rowCount; ++mask) {
        bool assignment[MAX_VARS];
        // Map bits to variable truth values.
        for (int i = 0; i < varCount; ++i) {
            assignment[i] = (mask >> (varCount - 1 - i)) & 1;
            table[mask][i] = assignment[i];
        }

        // Evaluate each formula for this assignment.
        for (int f = 0; f < formulaCount; ++f) {
            bool good = true;
            bool val = evaluateRPN(formulas[f], varNames, assignment, varCount, good);
            if (!good) { ok = false; return; }
            table[mask][varCount + f] = val;
        }
    }
}

void printTruthTable(const string varNames[MAX_VARS],
                     int varCount,
                     const Formula formulas[MAX_FORMULAS],
                     int formulaCount,
                     bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS],
                     int rowCount) {
    // Pretty-print the truth table from the stored 2D array.
    auto pad = [](const string& s, int w) {
        string out = s;
        if ((int)out.size() < w) out.append(w - out.size(), ' ');
        return out;
    };

    cout << "\n                 TRUTH TABLE                 \n";
    const int width = 10;
    for (int i = 0; i < varCount; ++i) cout << pad(varNames[i], width);
    for (int f = 0; f < formulaCount; ++f) cout << pad(formulas[f].name, width);
    cout << "\n";

    // Print each row of vars + formulas.
    for (int r = 0; r < rowCount; ++r) {
        for (int c = 0; c < varCount + formulaCount; ++c) {
            cout << pad(boolToString(table[r][c]), width);
        }
        cout << "\n";
    }
}

void analyzeArgument(const string varNames[MAX_VARS],
                     bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS],
                     int rowCount,
                     int varCount,
                     int premiseCount,
                     int formulaCount) {
    // Scan the table to check satisfiability and find counterexamples to validity.
    bool valid = true;
    bool satisfiable = false;
    bool counterFound = false;
    int counterRow = -1;
    int conclusionIdx = varCount + formulaCount - 1;

    // Inspect every row for satisfiability and counterexamples.
    for (int r = 0; r < rowCount; ++r) {
        bool allPremises = true;
        // Check all premises in this row.
        for (int p = 0; p < premiseCount; ++p) {
            allPremises = allPremises && table[r][varCount + p];
        }
        bool conclusion = table[r][conclusionIdx];
        if (allPremises && !conclusion) {
            valid = false;
            if (!counterFound) {
                counterFound = true;
                counterRow = r;
            }
        }
        if (allPremises && conclusion) satisfiable = true;
    }

    cout << "\n                 ANALYSIS                     \n";
    cout << "Satisfiable: " << (satisfiable ? "Yes" : "No") << "\n";
    if (valid) {
        cout << "Valid: Yes (no counterexample)\n";
                    } else {
        cout << "Valid: Falsifiable (counterexample found)\n";
        if (counterFound) {
            cout << "Counterexample: \n";
    // Report the first counterexample
    for (int i = 0; i < varCount; ++i) {
                cout << "  " << varNames[i] << " = " << boolToString(table[counterRow][i]) << "\n";
            }
        }
    }
}

void interactiveMode() {
    // ask user for variables, mode (symbols vs English), premises, and conclusion; then analyze. 
    cout << "Operators: ! (NOT)  & (AND)  | (OR)  > (IMPLIES)\n";
    cout << "You can type symbols directly, or choose English keywords\n";
    cout << "(not/no, and, or, implies/then)\n";
    cout << "Example symbols: ( p | q ) > ! r\n";
    cout << "Example English: p and q then not r\n";

    int varCount;
    cout << "How many variables? (1-" << MAX_VARS << "): ";
    cin >> varCount;
    cin.ignore();
    if (varCount < 1 || varCount > MAX_VARS) {
        cout << "Invalid variable count.\n";
        return;
    }

    bool englishMode = false;
    cout << "Use English keywords instead of symbols? (yes or no): ";
    string modeChoice; cin >> modeChoice; cin.ignore();
    if (modeChoice == "yes" || modeChoice == "YES" || modeChoice == "y" || modeChoice == "Y") englishMode = true;

    string vars[MAX_VARS];
    // Read variable names (single letters in symbol mode, words allowed in English mode).
    for (int i = 0; i < varCount; ++i) {
        cout << "Name for variable " << (i + 1) << ": ";
        string tmp; getline(cin, tmp);
        tmp = toLowerSimple(tmp);
        if (!englishMode && tmp.size() != 1) {
            cout << "In symbol mode, use single-letter variable names.\n";
            return;
        }
        vars[i] = tmp;
    }

    int premiseCount;
    cout << "Number of premises (0-" << MAX_PREMISES << "): ";
    cin >> premiseCount;
    cin.ignore();
    if (premiseCount < 0 || premiseCount > MAX_PREMISES) {
        cout << "Invalid premise count.\n";
        return;
    }

    Formula formulas[MAX_FORMULAS];
    int formulaCount = premiseCount + 1;
    // Read each premise, normalize (if English mode), tokenize, convert to postfix.
    for (int i = 0; i < premiseCount; ++i) {
        cout << "Premise " << (i + 1) << ": ";
        string line; getline(cin, line);
        formulas[i].name = "P" + to_string(i + 1);
        string infix[MAX_TOKENS];
        string normalized = normalizeExpression(line, englishMode);
        int ic = splitTokens(normalized, infix);
        bool okConv = true;
        infixToPostfix(infix, ic, formulas[i].tokens, formulas[i].tokenCount, okConv);
        if (!okConv) { cout << "Malformed premise.\n"; return; }
    }

    cout << "Conclusion: ";
    string concl; getline(cin, concl);
    formulas[premiseCount].name = "Conclusion";
    string infixC[MAX_TOKENS];
    string normalizedC = normalizeExpression(concl, englishMode);
    int icc = splitTokens(normalizedC, infixC);
    bool okConvC = true;
    infixToPostfix(infixC, icc, formulas[premiseCount].tokens, formulas[premiseCount].tokenCount, okConvC);
    if (!okConvC) { cout << "Malformed conclusion.\n"; return; }

    bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS];
    int rowCount = 0;
    bool ok = true;
    buildTruthTable(vars, varCount, formulas, formulaCount, table, rowCount, ok);
    if (!ok) {
        cout << "Error: malformed expression or unknown variable.\n";
        return;
    }

    printTruthTable(vars, varCount, formulas, formulaCount, table, rowCount);
    analyzeArgument(vars, table, rowCount, varCount, premiseCount, formulaCount);
}

int main() {
    // Entry point: run interactive mode only.
    cout << "                 ARGUMENT VALIDATOR             \n";
    cout << "                 Hassan Radwan 25P0391                \n";
        interactiveMode();
    cout << "Thank You!\n";
    return 0;
}

