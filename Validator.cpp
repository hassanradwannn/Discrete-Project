// Minimal logical validator without STL helpers (only basic arrays/math).
#include <iostream>
#include <string>

using namespace std;

// Limits to keep everything in fixed-size arrays (no std::vector, no find, no iomanip).
const int MAX_VARS = 5;
const int MAX_PREMISES = 5;
const int MAX_FORMULAS = MAX_PREMISES + 1;     // premises + conclusion
const int MAX_TOKENS = 32;                     // per formula in postfix
const int MAX_ROWS = 1 << MAX_VARS;            // 2^5 = 32

struct Formula {
    string name;
    string tokens[MAX_TOKENS];
    int tokenCount;
};

string boolToString(bool v) { return v ? "T" : "F"; }
string toLowerSimple(const string& s) {
    string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        char c = out[i];
        if (c >= 'A' && c <= 'Z') out[i] = char(c + ('a' - 'A'));
    }
    return out;
}

// Basic logic ops (still just math/boolean).
bool opNot(bool p) { return !p; }
bool opAnd(bool p, bool q) { return p && q; }
bool opOr(bool p, bool q) { return p || q; }
bool opImplies(bool p, bool q) { return !p || q; }

// Manual tokenizer: splits operators (),!,&,|,> as separate tokens and
// separates words; ignores spaces. Handles "!A" by emitting "!" then "A".
int splitTokens(const string& line, string outTokens[MAX_TOKENS]) {
    int count = 0;
    string current = "";
    auto flush = [&](void) {
        if (!current.empty() && count < MAX_TOKENS) {
            outTokens[count++] = current;
            current.clear();
        }
    };

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
            // normalize to lowercase for case-insensitive matching
            if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
            current.push_back(c);
        }
    }
    flush();
    return count;
}

// Precedence helper for infix parsing.
int precedence(const string& op) {
    if (op == "!") return 3;
    if (op == "&") return 2;
    if (op == "|") return 1;
    if (op == ">") return 0; // lowest (implies)
    return -1;
}

bool isRightAssociative(const string& op) {
    return op == "!" || op == ">";
}

// Convert infix tokens to postfix (shunting-yard, minimal, space-delimited).
void infixToPostfix(const string inTokens[MAX_TOKENS], int inCount,
                    string outTokens[MAX_TOKENS], int& outCount,
                    bool& ok) {
    string opStack[MAX_TOKENS];
    int opTop = 0;
    outCount = 0;
    ok = true;

    for (int i = 0; i < inCount; ++i) {
        const string& t = inTokens[i];
        if (t == "(") {
            opStack[opTop++] = t;
        } else if (t == ")") {
            // pop until "("
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

    while (opTop > 0) {
        string top = opStack[--opTop];
        if (top == "(" || top == ")") { ok = false; return; }
        outTokens[outCount++] = top;
    }
}

// Evaluate postfix (RPN) tokens using a small bool stack.
bool evaluateRPN(const Formula& f,
                 const string varNames[MAX_VARS],
                 const bool values[MAX_VARS],
                 int varCount,
                 bool& ok) {
    bool stack[MAX_TOKENS];
    int top = 0;
    ok = true;

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
            // variable lookup (linear scan, no std::find)
            int idx = -1;
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

// Build truth table into a fixed 2D bool array.
void buildTruthTable(const string varNames[MAX_VARS],
                     int varCount,
                     const Formula formulas[MAX_FORMULAS],
                     int formulaCount,
                     bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS],
                     int& rowCount,
                     bool& ok) {
    ok = true;
    rowCount = 1 << varCount;
    for (int mask = 0; mask < rowCount; ++mask) {
        bool assignment[MAX_VARS];
        for (int i = 0; i < varCount; ++i) {
            assignment[i] = (mask >> (varCount - 1 - i)) & 1;
            table[mask][i] = assignment[i];
        }

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
    auto pad = [](const string& s, int w) {
        string out = s;
        if ((int)out.size() < w) out.append(w - out.size(), ' ');
        return out;
    };

    cout << "\n================ TRUTH TABLE ================\n";
    const int width = 10;
    for (int i = 0; i < varCount; ++i) cout << pad(varNames[i], width);
    for (int f = 0; f < formulaCount; ++f) cout << pad(formulas[f].name, width);
    cout << "\n";

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
    bool valid = true;
    bool satisfiable = false;
    bool counterFound = false;
    int counterRow = -1;
    int conclusionIdx = varCount + formulaCount - 1;

    for (int r = 0; r < rowCount; ++r) {
        bool allPremises = true;
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

    cout << "\n================ ANALYSIS ===================\n";
    cout << "Satisfiable: " << (satisfiable ? "Yes" : "No") << "\n";
    if (valid) {
        cout << "Valid:       Yes (no counterexample)\n";
    } else {
        cout << "Valid:       Falsifiable (counterexample found)\n";
        if (counterFound) {
            cout << "Counterexample assignment:\n";
            for (int i = 0; i < varCount; ++i) {
                cout << "  " << varNames[i] << " = " << boolToString(table[counterRow][i]) << "\n";
            }
        }
    }
}

void runPreset() {
    string vars[MAX_VARS] = {"k", "m", "a"};
    int varCount = 3;

    Formula formulas[MAX_FORMULAS];
    int formulaCount = 3;

    // Infix strings (space separated), then converted to postfix.
    string infix[MAX_TOKENS];
    int infixCount;
    bool ok = true;

    formulas[0].name = "Premise1";
    infixCount = splitTokens("K | M > ! A", infix);
    infixToPostfix(infix, infixCount, formulas[0].tokens, formulas[0].tokenCount, ok);

    formulas[1].name = "Premise2";
    infixCount = splitTokens("A | M", infix);
    infixToPostfix(infix, infixCount, formulas[1].tokens, formulas[1].tokenCount, ok);

    formulas[2].name = "Conclusion";
    infixCount = splitTokens("A | ! K", infix);
    infixToPostfix(infix, infixCount, formulas[2].tokens, formulas[2].tokenCount, ok);

    bool table[MAX_ROWS][MAX_VARS + MAX_FORMULAS];
    int rowCount = 0;
    buildTruthTable(vars, varCount, formulas, formulaCount, table, rowCount, ok);
    if (!ok) {
        cout << "Error building preset truth table.\n";
        return;
    }
    printTruthTable(vars, varCount, formulas, formulaCount, table, rowCount);
    analyzeArgument(vars, table, rowCount, varCount, formulaCount - 1, formulaCount);
}

void interactiveMode() {
    cout << "\n================ INTERACTIVE (INFIX) ================\n";
    cout << "Operators: ! (NOT)  & (AND)  | (OR)  > (IMPLIES)\n";
    cout << "Enter expressions space-separated in INFIX form.\n";
    cout << "Example: ( P | Q ) > ! R   or   P | Q > ! R\n";

    int varCount;
    cout << "How many variables (1-" << MAX_VARS << ")? ";
    cin >> varCount;
    cin.ignore();
    if (varCount < 1 || varCount > MAX_VARS) {
        cout << "Invalid variable count.\n";
        return;
    }

    string vars[MAX_VARS];
    for (int i = 0; i < varCount; ++i) {
        cout << "Name for variable " << (i + 1) << ": ";
        string tmp; getline(cin, tmp);
        vars[i] = toLowerSimple(tmp);
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
    for (int i = 0; i < premiseCount; ++i) {
        cout << "Premise " << (i + 1) << " (infix, space-separated): ";
        string line; getline(cin, line);
        formulas[i].name = "P" + to_string(i + 1);
        string infix[MAX_TOKENS];
        int ic = splitTokens(line, infix);
        bool okConv = true;
        infixToPostfix(infix, ic, formulas[i].tokens, formulas[i].tokenCount, okConv);
        if (!okConv) { cout << "Malformed premise.\n"; return; }
    }

    cout << "Conclusion (infix, space-separated): ";
    string concl; getline(cin, concl);
    formulas[premiseCount].name = "Conclusion";
    string infixC[MAX_TOKENS];
    int icc = splitTokens(concl, infixC);
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
    cout << "Argument Validator (no STL helpers)\n";
    cout << "Preset argument (K, M, A):\n";
    runPreset();

    cout << "\nRun interactive mode? (y/n): ";
    char c; cin >> c; cin.ignore();
    if (c == 'y' || c == 'Y') {
        interactiveMode();
    }
    
    cout << "Done.\n";
    return 0;
}

