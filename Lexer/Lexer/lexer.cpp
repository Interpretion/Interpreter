#include <iostream>
using namespace std;

//===----------------------------------------------------------------------===//
// Lexer for VSL
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
// ȫ��ȡ������Ϊ����ASCII������
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

	//control
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

	//assignment
	tok_equal = -18

};

static std::string IdentifierStr; // Filled in if tok_identifier ������ʶ���͹ؼ���
static double NumVal;             // Filled in if tok_number

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

	// Otherwise, just return the character as its ascii value. eg. + - * / and so on.
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar; // �˴�ascii�루�����������պ���ö�ٳ�����ʾ����ֵ�����Ǹ�������Ӧ
}

int main()
{
	//���ÿһ��token��ö�ٳ���ֵ��-1 - -18������ascii��ֵ��0 - 255��
	int token = gettok();
	cout << token;
	return 0;
}
