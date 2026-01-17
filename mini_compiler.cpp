#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cctype>
#include <stdexcept>
#include <fstream>   // FIXED: Added missing header

using namespace std;

// =======================================
// 1. TOKENS & LEXER
// =======================================
struct Token {
    string type;
    string value;
    int line;
    Token(string t, string v, int l=0) : type(t), value(v), line(l) {}
};

class Lexer {
private:
    string source;
    size_t pos;
    int line;
public:
    Lexer(string s) : source(s), pos(0), line(1) {}

    vector<Token> tokenize() {
        vector<Token> tokens;
        while (pos < source.length()) {
            char c = source[pos];
            if (isspace(c)) {
                if (c == '\n') line++;
                pos++;
                continue;
            }
            if (isdigit(c)) {
                string num;
                while (pos < source.length() && isdigit(source[pos])) {
                    num += source[pos++];
                }
                tokens.emplace_back("NUMBER", num, line);
                continue;
            }
            if (isalpha(c)) {
                string id;
                while (pos < source.length() && isalnum(source[pos])) {
                    id += source[pos++];
                }
                if (id == "print") tokens.emplace_back("PRINT", id, line);
                else tokens.emplace_back("ID", id, line);
                continue;
            }
            if (string("+-*/()=;").find(c) != string::npos) {
                tokens.emplace_back(string(1, c), string(1, c), line);
                pos++;
                continue;
            }
            throw runtime_error("Lex error: " + string(1, c) + " at line " + to_string(line));
        }
        return tokens;
    }
};

// =======================================
// 2. AST NODES
// =======================================
struct Node {
    string type;
    vector<Node*> children;
    string value;
    Node(string t, string v="") : type(t), value(v) {}
    ~Node() { for (auto child : children) delete child; }
};



class Parser {
private:
    vector<Token> tokens;
    size_t pos;

    const Token& current() { return tokens[pos]; }
    void advance() { if (pos < tokens.size()) pos++; }
    bool match(string type) {
        if (pos < tokens.size() && tokens[pos].type == type) {
            advance();
            return true;
        }
        return false;
    }

    Token expect(string type) {
        if (pos < tokens.size() && tokens[pos].type == type) {
            return tokens[pos++];
        }
        throw runtime_error("Expected " + type + " at line " + (pos < tokens.size() ? to_string(tokens[pos].line) : "end"));
    }

public:
    Parser(vector<Token> t) : tokens(t), pos(0) {}

    Node* parse() {
        Node* program = new Node("PROGRAM");
        while (pos < tokens.size()) {
            program->children.push_back(parse_stmt());
        }
        return program;
    }

    Node* parse_stmt() {
        if (match("PRINT")) {
            expect("(");
            Node* expr = parse_expr();
            expect(")");
            expect(";");
            Node* print = new Node("PRINT");
            print->children.push_back(expr);
            return print;
        } else {
            Token id = expect("ID");
            expect("=");
            Node* expr = parse_expr();
            expect(";");
            Node* assign = new Node("ASSIGN", id.value);
            assign->children.push_back(expr);
            return assign;
        }
    }

    Node* parse_expr() {
        Node* node = parse_term();
        while (match("+") || match("-")) {
            string op = tokens[pos-1].value;
            Node* right = parse_term();
            Node* binop = new Node(op);
            binop->children.push_back(node);
            binop->children.push_back(right);
            node = binop;
        }
        return node;
    }

    Node* parse_term() {
        Node* node = parse_factor();
        while (match("*") || match("/")) {
            string op = tokens[pos-1].value;
            Node* right = parse_factor();
            Node* binop = new Node(op);
            binop->children.push_back(node);
            binop->children.push_back(right);
            node = binop;
        }
        return node;
    }

    Node* parse_factor() {
        if (match("NUMBER")) return new Node("NUMBER", tokens[pos-1].value);
        if (match("ID")) return new Node("ID", tokens[pos-1].value);
        if (match("(")) {
            Node* expr = parse_expr();
            expect(")");
            return expr;
        }
        throw runtime_error("Syntax error at line " + to_string(tokens[pos].line));
    }
};

// =======================================
// 3. SYMBOL TABLE & INTERPRETER
// =======================================
class SymbolTable {
public:
    map<string, int> vars; // FIXED: Made public for simpler interpreter access

    int lookup(string name) {
        if (vars.count(name) == 0) throw runtime_error("Undeclared variable: " + name);
        return vars[name];
    }

    int eval_node(Node* node) {
        if (node->type == "NUMBER") return stoi(node->value);
        if (node->type == "ID") return lookup(node->value);

        int left = eval_node(node->children[0]);
        int right = eval_node(node->children[1]);

        if (node->type == "+") return left + right;
        if (node->type == "-") return left - right;
        if (node->type == "*") return left * right;
        if (node->type == "/") {
            if (right == 0) throw runtime_error("Division by zero");
            return left / right;
        }
        throw runtime_error("Eval error: " + node->type);
    }
};

// =======================================
// 4. CODE GENERATION
// =======================================
string codegen(Node* node, map<string, bool>& declared) {
    if (node->type == "PROGRAM") {
        string code = "#include <stdio.h>\nint main() {\n";
        for (auto child : node->children) {
            code += "  " + codegen(child, declared) + ";\n";
        }
        return code + "  return 0;\n}";
    }
    if (node->type == "ASSIGN") {
        string prefix = declared[node->value] ? "" : "int ";
        declared[node->value] = true;
        return prefix + node->value + " = " + codegen(node->children[0], declared);
    }
    if (node->type == "PRINT") {
        return "printf(\"%d\\n\", " + codegen(node->children[0], declared) + ")";
    }
    if (node->type == "NUMBER") return node->value;
    if (node->type == "ID") return node->value;

    return "(" + codegen(node->children[0], declared) + " " + node->type + " " + codegen(node->children[1], declared) + ")";
}

// =======================================
// 5. MAIN
// =======================================
int main(int argc, char* argv[]) {
    // ১. কমান্ড লাইন আর্গুমেন্ট চেক
    if (argc < 2) {
        cout << "Usage: ./compiler <source_file>\n";
        return 1;
    }

    // ২. সোর্স ফাইল ওপেন এবং রিড করা
    ifstream file(argv[1]);
    if (!file) {
        cout << "File not found: " << argv[1] << endl;
        return 1;
    }

    string source, line;
    while (getline(file, line)) {
        source += line + "\n";
    }
    file.close();

    try {
        // ৩. ফেজ ১: লেক্সিকাল অ্যানালাইসিস (Lexical Analysis)
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // --- SCANNER REPORT (টোকেন লিস্ট দেখানো) ---
        cout << "=== SCANNER REPORT ===" << endl;
        cout << "Line\tType\t\tValue" << endl;
        cout << "-------------------------------" << endl;
        for (const auto& t : tokens) {
            // সুন্দরভাবে সাজিয়ে দেখানোর জন্য ট্যাব ব্যবহার করা হয়েছে
            cout << t.line << "\t" << t.type << (t.type.length() < 8 ? "\t\t" : "\t") << t.value << endl;
        }
        cout << "-------------------------------\n" << endl;

        // ৪. ফেজ ২: সিনট্যাক্স অ্যানালাইসিস (Parsing)
        Parser parser(tokens);
        Node* ast = parser.parse();

        // ৫. ফেজ ৩: ইন্টারপ্রিটার (Execution)
        cout << "=== EXECUTION ===" << endl;
        SymbolTable symtab;
        for (auto stmt : ast->children) {
            if (stmt->type == "ASSIGN") {
                // ভ্যারিয়েবল ডিক্লেয়ার বা আপডেট করা
                symtab.vars[stmt->value] = symtab.eval_node(stmt->children[0]);
            } else if (stmt->type == "PRINT") {
                // সরাসরি আউটপুট দেখানো
                cout << symtab.eval_node(stmt->children[0]) << endl;
            }
        }

        // ৬. ফেজ ৪: কোড জেনারেশন (C Code Generation)
        map<string, bool> declared;
        string c_code = codegen(ast, declared);
        cout << "\n=== C CODE ===\n" << c_code << endl;

        // ৭. আউটপুট ফাইল হিসেবে সেভ করা
        ofstream out("output.c");
        out << c_code;
        out.close();

        // মেমরি পরিষ্কার করা
        delete ast;

    } catch (const exception& e) {
        // এরর মেসেজ সুন্দরভাবে দেখানো
        cerr << "\n------------------------------------" << endl;
        cerr << "COMPILE ERROR: " << e.what() << endl;
        cerr << "------------------------------------" << endl;
        return 1;
    }

    return 0;
}