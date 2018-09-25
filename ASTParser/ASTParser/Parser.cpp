#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <iostream>
using namespace std;


//===----------------------------------------------------------------------===//
// Lexer for VSL
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
// 全部取负数是为了与ASCII码区分
enum Token {
	tok_eof = -1,

	// commands
	tok_FUNC = -2,
	tok_PRINT = -3,
	tok_RETURN = -4,
	tok_CONTINUE = -5,

	// primary
	tok_identifier = -6,
	tok_number = -7,

	// control
	tok_IF = -8,
	tok_THEN = -9,
	tok_ELSE = -10,
	tok_FI = -11,
	tok_WHILE = -12,
	tok_DO = -13,
	tok_DONE = -14,

	// operators
	tok_binary = -15,
	tok_unary = -16,

	// var definition
	tok_VAR = -17,

	// assignment
	tok_equal = -18

};

static std::string
IdentifierStr;    // Filled in if tok_identifier 包括标识符和关键字
static double NumVal; // Filled in if tok_number

					  /// gettok - Return the next token from standard input.
static int gettok() {
	static int LastChar = ' ';

	// Skip any whitespace.
	while (isspace(LastChar))
		LastChar = getchar();

	if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
		IdentifierStr = LastChar;
		while (isalnum((LastChar = getchar())))
			IdentifierStr += LastChar;

		if (IdentifierStr == "FUNC")
			return tok_FUNC;
		if (IdentifierStr == "IF")
			return tok_IF;
		if (IdentifierStr == "THEN")
			return tok_THEN;
		if (IdentifierStr == "ELSE")
			return tok_ELSE;
		if (IdentifierStr == "FI")
			return tok_FI;
		if (IdentifierStr == "WHILE")
			return tok_WHILE;
		if (IdentifierStr == "DO")
			return tok_DO;
		if (IdentifierStr == "DONE")
			return tok_DONE;
		if (IdentifierStr == "binary")
			return tok_binary;
		if (IdentifierStr == "unary")
			return tok_unary;
		if (IdentifierStr == "VAR")
			return tok_VAR;
		if (IdentifierStr == ":=")
			return tok_equal;

		return tok_identifier;
	}

	if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr.c_str(), nullptr);
		return tok_number;
	}

	if (LastChar == '/') {
		// Comment until end of line.
		do
			LastChar = getchar();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF)
			return gettok();
	}

	// Check for end of file.  Don't eat the EOF.
	if (LastChar == EOF)
		return tok_eof;

	// Otherwise, just return the character as its ascii value. eg. + - * / and so
	// on.
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar; // 此处ascii码（都是正数）刚好与枚举常量表示的数值（都是负数）相应
}

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

class ExprAST {
public:
	virtual ~ExprAST() {}
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
public:
	double Val;
public:
	NumberExprAST(double val) : Val(val) {}
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
public:
	std::string Name;
public:
	VariableExprAST(const std::string &name) : Name(name) {}
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
public:
	char Op;
	ExprAST *LHS = NULL, *RHS = NULL;
public:
	BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
		: Op(op), LHS(lhs), RHS(rhs) {}
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
public:
	std::string Callee;
	std::vector<ExprAST*> Args;
public:
	CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
		: Callee(callee), Args(args) {}
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
public:
	std::string Name;
	std::vector<std::string> Args;
public:
	PrototypeAST(const std::string &name, const std::vector<std::string> &args)
		: Name(name), Args(args) {}

};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
public:
	PrototypeAST * Proto;
	ExprAST *Body;
public:
	FunctionAST(PrototypeAST *proto, ExprAST *body)
		: Proto(proto), Body(body) {}

};

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() {
	return CurTok = gettok();
}

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
	if (!isascii(CurTok))
		return -1;

	// Make sure it's a declared binop.
	int TokPrec = BinopPrecedence[CurTok];
	if (TokPrec <= 0) return -1;
	return TokPrec;
}

/// Error* - These are little helper functions for error handling.
ExprAST *Error(const char *Str) { fprintf(stderr, "Error: %s\n", Str); return nullptr; }
PrototypeAST *ErrorP(const char *Str) { Error(Str); return nullptr; }
FunctionAST *ErrorF(const char *Str) { Error(Str); return nullptr; }

static ExprAST *ParseExpression();

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static ExprAST *ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;

	getNextToken();  // eat identifier.

	if (CurTok != '(') // Simple variable ref.
		return new VariableExprAST(IdName);

	// Call.
	getNextToken();  // eat (
	std::vector<ExprAST*> Args;
	if (CurTok != ')') {
		while (1) {
			auto *Arg = ParseExpression();
			if (!Arg) return nullptr;
			Args.push_back(Arg);

			if (CurTok == ')') break;

			if (CurTok != ',')
				return Error("Expected ')' or ',' in argument list");
			getNextToken();
		}
	}

	// Eat the ')'.
	getNextToken();

	return new CallExprAST(IdName, Args);
}

/// numberexpr ::= number
static ExprAST *ParseNumberExpr() {
	auto *Result = new NumberExprAST(NumVal);
	getNextToken(); // consume the number
	return Result;
}

/// parenexpr ::= '(' expression ')'
static ExprAST *ParseParenExpr() {
	getNextToken();  // eat (.
	auto *V = ParseExpression();
	if (!V) return nullptr;

	if (CurTok != ')')
		return Error("expected ')'");
	getNextToken();  // eat ).
	return V;
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static ExprAST *ParsePrimary() {
	switch (CurTok) {
	default: return Error("unknown token when expecting an expression");
	case tok_identifier: return ParseIdentifierExpr();
	case tok_number:     return ParseNumberExpr();
	case '(':            return ParseParenExpr();
	}
}

/// binoprhs
///   ::= ('+' primary)*
static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
	// If this is a binop, find its precedence.
	while (1) {
		int TokPrec = GetTokPrecedence();

		// If this is a binop that binds at least as tightly as the current binop,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return LHS;
		//return nullptr;

		// Okay, we know this is a binop.
		int BinOp = CurTok;
		getNextToken();  // eat binop

						 // Parse the primary expression after the binary operator.
		auto *RHS = ParsePrimary();
		if (!RHS) return nullptr;

		// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, RHS);
			if (!RHS) return nullptr;
		}

		// Merge LHS/RHS.
		LHS = new BinaryExprAST(BinOp, LHS, RHS);
	}
}

/// expression
///   ::= primary binoprhs
///
static ExprAST *ParseExpression() {
	auto *LHS = ParsePrimary();
	if (!LHS) return nullptr;

	return ParseBinOpRHS(0, LHS);
}

/// prototype
///   ::= id '(' id* ')'
static PrototypeAST *ParsePrototype() {
	if (CurTok != tok_identifier)
		return ErrorP("Expected function name in prototype");

	std::string FnName = IdentifierStr;
	getNextToken();

	if (CurTok != '(')
		return ErrorP("Expected '(' in prototype");

	std::vector<std::string> ArgNames;

	//参数列表中参数之间可用逗号分隔
	getNextToken();
	while (CurTok == tok_identifier || CurTok == 44)
	{
		if(CurTok == tok_identifier) ArgNames.push_back(IdentifierStr);
		CurTok = getNextToken();
	}
		
	if (CurTok != ')')
		return ErrorP("Expected ')' in prototype");

	// success.
	getNextToken();  // eat ')'.

	return new PrototypeAST(FnName, ArgNames);
}

/// definition ::= 'FUNC' prototype expression
static FunctionAST *ParseDefinition() {

	getNextToken();  // eat FUNC.
	PrototypeAST *Proto = ParsePrototype();
	if (Proto == 0) return nullptr;

	
	if(CurTok != '{') return ErrorF("Expected '{' in function body"); // eat { for VSL
	else getNextToken();
	if (auto *E = ParseExpression())
		if (CurTok != '}') return ErrorF("Expected '}' in function body"); // eat } for VSL
		else
		{
			getNextToken();
			return new FunctionAST(Proto, E);
		}
	return nullptr;
}

/// toplevelexpr ::= expression
static FunctionAST *ParseTopLevelExpr() {
	if (auto *E = ParseExpression()) {
		// Make an anonymous proto.
		PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
		return new FunctionAST(Proto, E);
	}
	return nullptr;
}

/// external ::= 'extern' prototype
static PrototypeAST *ParseExtern() {
	getNextToken();  // eat extern.
	return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//
static void Print(ExprAST* T, string printMoudle = "EXPR") {
	queue<ExprAST*> myqueue;
	myqueue.push(T);
	int count = 1;

	while (!myqueue.empty()) {

		//根据打印内容判断如何打印（函数，单个表达式（默认））
		string s;
		if(printMoudle == "FUNC") s = "。。。。";
		else s = "";

		for (int i = 0; i < count / 2; i++)
		{
			s += "。。";
		}
		BinaryExprAST* tmp = (BinaryExprAST*)myqueue.front();
		VariableExprAST* var = (VariableExprAST*)tmp;
		if (tmp->Op != '+'&&tmp->Op != '-'&&tmp->Op != '*'&&tmp->Op != '/')cout << s << var->Name << "\n";
		else {
			if (tmp->LHS != NULL)
				myqueue.push(tmp->LHS);
			if (tmp->RHS != NULL)
				myqueue.push(tmp->RHS);
			cout << s << tmp->Op << "\n";
		}

		myqueue.pop();
		count++;
	}
}

static void HandleDefinition() {
	auto def = ParseDefinition();
	if (def) {
		fprintf(stderr, "Parsed a function definition.\n");
		PrototypeAST* proto = def->Proto;
		string name = proto->Name;
		vector<std::string> args = proto->Args;
		ExprAST* Exp = def->Body;

		cout << "FUNC\n";
		cout << "。。Prototype\n";
		cout << "。。。。" << name << "\n";
		for(string s: args)
		{
			cout << "。。。。" << s << "\n";
		}
		cout << "。。Body\n";
		Print(Exp, "FUNC");//打印内容


	}
	else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleTopLevelExpression() {
	// Evaluate a top-level expression into an anonymous function.
	auto ptle = ParseTopLevelExpr();
	if (ptle) {
		fprintf(stderr, "Parsed a top-level expr\n");
		PrototypeAST* proto = ptle->Proto;
		string name = proto->Name;
		ExprAST* Exp = ptle->Body;
		Print(Exp);//打印内容

	}
	else {
		// Skip token for error recovery.
		getNextToken();
	}
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
	while (true) {
		fprintf(stderr, "ready> ");
		switch (CurTok) {
		case tok_eof:
			return;
		case ';': // ignore top-level semicolons.
			getNextToken();
			break;
		case tok_FUNC:
			HandleDefinition();
			break;
		default:
			HandleTopLevelExpression();
			break;
		}
	}
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main() {
	// Install standard binary operators.
	// 1 is lowest precedence.
	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40; // highest.

							   // Prime the first token.
	fprintf(stderr, "ready> ");
	getNextToken();

	// Run the main "interpreter loop" now.
	MainLoop();

	return 0;
}