#include "dummyscheme.h"

#define CASE_NUM case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define CASE_SYMBOL case '+':case '-':case '*':case '/'

#define TokenError(fmt, ...) Tokenize::error(fmt" File: %s Line: %d, Function: %s", ##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)

DummyValue::DummyValue(int num)
	:type(DummyType::DUMMY_INT_NUM)
{
	basic.intnum = num;
}

DummyValue::DummyValue(DummyType type, std::string val)
	:type(type)
{	
	if (type == DummyType::DUMMY_STRING ||
			type == DummyType::DUMMY_SYMBOL) {
		strAndSymbol = val;
	}	else {
		TokenError("wrong dummytype %d", type);
	}
}

DummyValue::DummyValue(std::vector<DummyValue*> list)
	:type(DummyType::DUMMY_LIST),
	 list(list)
{
}

DummyValue::DummyValue(DummyValue* val)
	:type(val->type),
	basic(val->basic),
	list(val->list)
{
}

DummyValue::~DummyValue()
{
	// only check list type
	if (type == DummyType::DUMMY_LIST) {
		for (std::vector<DummyValue*>::iterator itr = list.begin();
				 itr != list.end(); ++itr) {
			delete *itr;
		}
	}
}

/*
	(+ 1 2 3)
	TODO: check parameter length the first pass	
*/
DummyValue* OpFuncPlus(DummyValue* value)
{
	std::vector<DummyValue*> list = value->getList(); 
	
	int num = 0;
	std::vector<DummyValue*>::iterator itr = list.begin();	
	// first is the symbol
	for (itr++; itr != list.end(); ++itr) {
		DummyValue* item = *itr;	
		if (item->getType() != DummyType::DUMMY_LIST) {
			num += item->getInt();
		} else {
			DummyValue* realItem = Tokenize::eval(item);
			// TODO: check type
			num += realItem->getInt();
			delete realItem;
		}
	}
	return new DummyValue(num);
}

/*
	(- 1 2 3)
*/
DummyValue* OpFuncMinus(DummyValue* value)
{
	std::vector<DummyValue*> list = value->getList(); 
	
	// first is the symbol	
	std::vector<DummyValue*>::iterator itr = list.begin();	
	// *itr = "-"
	// second is the eeeee
	++itr;
	int num = (*itr)->getInt();
	
	for (++itr; itr != list.end(); ++itr) {
		DummyValue* item = *itr;	
		if (item->getType() != DummyType::DUMMY_LIST) {
			num -= item->getInt();
		} else {
			DummyValue* realItem = Tokenize::eval(item);
			// TODO: check type
			num -= realItem->getInt();	
			delete realItem;
		}
	}
	return new DummyValue(num);
}

/*
	(* 1 2 3)
*/
DummyValue* OpFuncMul(DummyValue* value)
{
	std::vector<DummyValue*> list = value->getList(); 

	int num = 1;
	// first is the symbol	
	std::vector<DummyValue*>::iterator itr = list.begin();	
	
	for (itr++; itr != list.end(); ++itr) {
		DummyValue* item = *itr;	
		if (item->getType() != DummyType::DUMMY_LIST) {
			num *= item->getInt();
		} else {
			DummyValue* realItem = Tokenize::eval(item);
			// TODO: check type
			num *= realItem->getInt();	
			delete realItem;	
		}
	}
	return new DummyValue(num);
}

/*
	(/ 1 2 3)
*/
DummyValue* OpFuncDivide(DummyValue* value)
{
	std::vector<DummyValue*> list = value->getList(); 
	
	// first is the symbol	
	std::vector<DummyValue*>::iterator itr = list.begin();	
	// second is the eeeee
	int num = (*(++itr))->getInt();
	
	for (++itr; itr != list.end(); ++itr) {
		DummyValue* item = *itr;	
		if (item->getType() != DummyType::DUMMY_LIST) {
			num /= item->getInt();
		} else {
			DummyValue* realItem = Tokenize::eval(item);
			// TODO: check type
			num /= realItem->getInt();	
			delete realItem;	
		}
	}
	return new DummyValue(num);
}

std::map<std::string, OpFunc> Tokenize::opMap;
//Tokenize::opMap["+"] = OpFuncPlus;
void Tokenize::init()
{
	opMap["+"] = OpFuncPlus;
	opMap["-"] = OpFuncMinus;
	opMap["*"] = OpFuncMul;
	opMap["/"] = OpFuncDivide;
}

/*
	Just for first pass to check
*/
void Tokenize::addOpForCheck(std::string symbol)
{
	std::map<std::string, OpFunc>::iterator it = opMap.find(symbol);
	if (it == opMap.end()) {
		opMap[symbol] = NULL;
	} else {
		// do nothing
	}
}

/*
	for define function
*/
void Tokenize::addOp(std::string symbol, OpFunc func)
{
	std::map<std::string, OpFunc>::iterator it = opMap.find(symbol);
	if (it != opMap.end() && it->second != NULL) {
		error("duplicate for symbol %s", symbol);
	}
	opMap[symbol] = func;
}

void Tokenize::unexpectedToken()
{
		std::stringstream ss;
		ss << "unexpected token "
			 << input[index]
			 << " at " << index;
		throw ss.str().c_str();
}

void Tokenize::error(const char *fmt, ...)
{
	char buffer[1024] = {0};
	va_list args;
	va_start(args, fmt);	
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	throw buffer;
}


void Tokenize::toString(std::stringstream& out, DummyValue * val)
{
	switch(val->type) {
	case DUMMY_INT_NUM:
		out << val->basic.intnum;
		break;
	case DUMMY_FLOAT_NUM:
		out << val->basic.floatnum;
		break;
	case DUMMY_SYMBOL:
		out << val->strAndSymbol;	
		break;
	case DUMMY_STRING:
		out << "\"" << val->strAndSymbol << "\"";
		break;
	case DUMMY_LIST:{
		out << "(";
		for (std::vector<DummyValue*>::iterator itr = val->list.begin();
				 itr != val->list.end(); itr++) {
			toString(out, *itr);
			if (itr + 1 != val->list.end()) {
				out << " ";	
			}
		}
		out << ")";
		break;
	}
	default:
		TokenError("unknown type %d", val->type);
		break;
	}
}

Tokenize::Tokenize(std::string &input)
{
	this->input = input;
	this->index = 0;

	try {
		DummyValue* val = readP();
		std::stringstream out;
		toString(out, val);
		out << "\n";
		
		DummyValue* evalVal = eval(val);
		toString(out, evalVal);
		printf("%s\n", out.str().c_str());
		
		delete val;
		// TODO: check val is same with evalVal
		delete evalVal;
	}	catch(const char* exception) {
		printf("%s\n", exception);
		// TODO: how to release val
	}
}

DummyValue* Tokenize::eval(DummyValue* value)
{
	if (value->getType() != DummyType::DUMMY_LIST) {
		return new DummyValue(value);
	} else {
		std::vector<DummyValue*> list = value->getList();
		if (list.empty()) {
			return NULL;
		}
		DummyValue* symbol = list.front();
		std::map<std::string, OpFunc>::iterator it = opMap.find(symbol->getSymbol());
		if (it == opMap.end()) {
			error("symbol error %s", symbol->getSymbol().c_str());
			return NULL;
		}

		OpFunc op = it->second;
		// this list contains symbol value as the first
		return op(value);
	}
}

TokenType Tokenize::readToken()
{
	skipBlank();
	switch(input[index])
	{
	CASE_NUM:
		return TokenType::TOKEN_NUM;
	case '\"':
		return TokenType::TOKEN_DOUBLE_QUOT;
	case '(':
		return TokenType::TOKEN_LEFT_PAREN;
	case ')':
		return TokenType::TOKEN_RIGHT_PAREN;	
	CASE_SYMBOL:
		return TokenType::TOKEN_SYMBOL;
	default:
		if ('A' <= input[index] && input[index] <= 'Z') {
			return TokenType::TOKEN_SYMBOL;
		}	else if ('a' <= input[index] && input[index] <= 'z') {
			return TokenType::TOKEN_SYMBOL;
		} else {
			return TokenType::TOKEN_UNKNOWN;
		}
	}

	return TokenType::TOKEN_UNKNOWN;
}

/*
	P = NUM | STRING | LEFT_PAREN LIST RIGHT_PAREN
*/
DummyValue* Tokenize::readP()
{
	DummyValue* curValue;
	headType = readToken();
	TokenType token;	
	switch(headType)
	{
	case TokenType::TOKEN_NUM:
		curValue = readNum();
		break;
	case TokenType::TOKEN_DOUBLE_QUOT:
		curValue = readStr();
		break;
	case TokenType::TOKEN_LEFT_PAREN:{	
		index++;
		std::vector<DummyValue*> list = readList();
		curValue = new DummyValue(list);
		token = readToken();
		if (token != TokenType::TOKEN_RIGHT_PAREN)
			unexpectedToken();
		else
			index++;
		break;
	}
	default:
		unexpectedToken();
		break;
	}
	
	return curValue;
}

/*
	LIST = SYMBOL LISTP	
*/
std::vector<DummyValue*> Tokenize::readList()
{
	std::vector<DummyValue*> list;
	headType = readToken();
	switch(headType)
	{
	case TokenType::TOKEN_SYMBOL:{
		DummyValue* symbol = readSymbol();
		addOpForCheck(symbol->getSymbol());
		
		list.push_back(symbol);
		
		std::vector<DummyValue*> listP = readListP();
		if (listP.size() > 0) {
			list.insert(list.end(), listP.begin(), listP.end());
		}
		break;
	}
	default:
		unexpectedToken();
		break;
	}

	return list;
}

/*
	LISTP = P LISTP		
*/
std::vector<DummyValue*> Tokenize::readListP()
{
	std::vector<DummyValue*> list;	
	headType = readToken();
	switch(headType)
	{
	case TokenType::TOKEN_NUM:
	case TokenType::TOKEN_LEFT_PAREN: {
		list.push_back(readP());
		std::vector<DummyValue*> listP = readListP();	
		if (listP.size() > 0) {
			list.insert(list.end(), listP.begin(), listP.end());
		}
		break;
	}
	case TokenType::TOKEN_RIGHT_PAREN:
		// DO nothing
		break;
	default:
		unexpectedToken();
		break;
	}

	return list;
}

bool Tokenize::isNum()
{
	return input[index] >= '0' && input[index] <= '9';
}

DummyValue* Tokenize::readStr()
{
	if (input[index] != '\"') {
		TokenError("begin expect token \" actually %c index %d", input[index], index);
	}
	
	index++;
	
	std::stringstream str;
	while(index < input.length() && input[index] != '\"') {
		str << input[index++];
	}
	
	if (input[index] != '\"') {
		TokenError("end expect token \" actually %c index %d", input[index], index);
	}
	
	index++;
	
	return new DummyValue(DummyType::DUMMY_STRING, str.str());	
}

DummyValue* Tokenize::readNum()
{
	char num[20] = {0};
	int temp = 0;
	
	bool breakFlag = false;
	while(index < input.length() && !breakFlag)
	{
		switch(input[index])
		{
		CASE_NUM:
			num[temp++] = input[index++];
			break;
		default:
			breakFlag = true;
			break;
		}
	}
	int val = 0;
	sscanf(num, "%d", &val);

	DummyValue* dValue = new DummyValue(val);
	return dValue;
}

bool Tokenize::isBlank()
{
	switch(input[index])
	{
	case ' ':
	case '\t':
	case '\n':
	case '\r':
		return true;
	default:
		return false;
	}

	return false;
}

void Tokenize::skipBlank()
{
	while(index < input.size() && isBlank())
		index++;
}

DummyValue* Tokenize::readSymbol()
{
	std::stringstream symbol;
	while(index < input.length() && !isBlank())
		symbol << input[index++];	

	return new DummyValue(DummyType::DUMMY_SYMBOL, symbol.str());
}

