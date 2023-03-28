#include "scheme.h"

using namespace Dummy;

#define CASE_DIGIT case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define CASE_BLANK case ' ':case '\t':case '\n':case '\r':case '\0'

#define AssertInputEnd(idx) Assert(idx < input.size(), "reached end of input")

//void Reader::parse(const String &input)
//{
//  this->input = input;
//	this->index = 0;
//
//  aheadToken = dLex();
//
//  VarValue val;
//  while (aheadToken != TOKEN_END)
//  {
//    val = readValue();
//  }
//}

Reader::Reader(const String& in)
{
  init(in);
}

void Reader::init(const String& in)
{
  this->input = in;
	this->index = 0;

  aheadToken = dLex();
}

VarValue Reader::readOne()
{
  if (aheadToken != TOKEN_END)
    return readValue();
  else
    return NULL;
}

void Reader::match(int type)
{
	if (type == aheadToken)
		aheadToken = dLex();
	else
		Error("unrecognized token type %d with current %d", type, aheadToken);
}

void Reader::skipBlankComment()
{
 loop:
  if(isEnd())
    return;

  switch(curChar()){
  case ' ':
  case '\t':
  case '\n':
  case '\r':
  case '\0':
    index++;
    goto loop;
  case ';':
    while(index < input.size() && input[index] != '\n')
      index++;
    index++;
    goto loop;
  default:
    break;
  }
}

bool Reader::isEnd()
{
	return index >= input.size();
}

void Reader::advInput()
{
  ++index;
}

char Reader::curCharAdv()
{
  return input[index++];
}

char Reader::curChar()
{
  return input[index];
}

int Reader::dLex()
{
  skipBlankComment();

	if (isEnd())
		return TOKEN_END;

  int token = TOKEN_UNKNOWN;

  switch(curChar()){
  CASE_DIGIT:
    token = readNum();
    break;
  case '\"':
    token = readString();
    break;
  case '(':
		token = TOKEN_LEFT_PAREN;
    advInput();
    break;
	case ')':
		token = TOKEN_RIGHT_PAREN;
    advInput();
    break;
	case '\'':
		token = TOKEN_QUOTE;
    advInput();
    break;
	case '`':
		token = TOKEN_QUASIQUOTE;
    advInput();
		break;
  case ',':
    token = TOKEN_UNQUOTE;
    advInput();
    if (!isEnd() && '@' == curChar())
    {
      token = TOKEN_UNQUOTE_SPLICING;
      advInput();
    }
    break;
  default:
    token = readSymbol();
    break;
  }

  return token;
}

/*
	read a str
	TODO: read doc
 */
int Reader::readString()
{
  advInput();

	StringStream str;
  while(!isEnd())
  {
    char c = curCharAdv();
    if (c == '\"')
      break;
    str << c;
  }

  if (isEnd())
    return TOKEN_UNKNOWN;

	strLexVal = str.str();

	return TOKEN_STRING;
}

/*
	TODO:	currently only support int
 */
int Reader::readNum()
{
	// TODO: read float
	char num[20] = {0};
	int temp = 0;

 loop:
  if(!isEnd())
  {
    char c = curChar();
    switch(c){
    CASE_BLANK:
    case ')':
      break;
    default:
      // TODO: illegal char
      num[temp++] = c;
      advInput();
      goto loop;
    }
  }

	sscanf(num, "%d", &numLexVal);

	return TOKEN_NUM;
}

int Reader::readSymbol()
{
  StringStream symbol;

 loop:
  if(!isEnd())
  {
		char c = curChar();
    switch(c){
    CASE_BLANK:
    case ')':
      break;
		default:
      symbol << c;
      advInput();
      goto loop;
		}
	}

	strLexVal = symbol.str();

	return TOKEN_SYMBOL;
}

VarValue Reader::readValue()
{
  VarValue val;
	switch(aheadToken){
	case TOKEN_NUM:
		val = new NumValue(numLexVal);
		match(TOKEN_NUM);
    break;
	case TOKEN_STRING:
		val = new StringValue(strLexVal);
		match(TOKEN_STRING);
    break;
	case TOKEN_SYMBOL:
    val = Scheme::intern(strLexVal);
		match(TOKEN_SYMBOL);
    break;
	case TOKEN_QUOTE:
    match(TOKEN_QUOTE);
    val = Squote(readValue());
    break;
	case TOKEN_UNQUOTE:
    match(TOKEN_UNQUOTE);
    val = Sunquote(readValue());
    break;
	case TOKEN_QUASIQUOTE:
    match(TOKEN_QUASIQUOTE);
    val = Squasiquote(readValue());
    break;
	case TOKEN_UNQUOTE_SPLICING:
    match(TOKEN_UNQUOTE_SPLICING);
    val = Sunquotesplicing(readValue());
    break;
	case TOKEN_LEFT_PAREN:
		match(TOKEN_LEFT_PAREN);
		val = readList();
		match(TOKEN_RIGHT_PAREN);
    break;
	default:
		Error("somethign error happened");
    break;
	}

  return val;
}

VarValue Reader::readList()
{
  VarValue res = Snull;
  VarValue parent;

  while (aheadToken != TOKEN_RIGHT_PAREN)
  {
    VarValue val = readValue();

    VarValue tmp(new PairValue(val, Scheme::Null));
    if (Snullp(res))
    {
      res = tmp;
    }
    if (parent)
    {
      parent->cdr(tmp);
    }
    parent = tmp;
  }

  return res;
}
