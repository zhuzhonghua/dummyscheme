#pragma once

#define TOKEN_LEFT_PAREN '('
#define TOKEN_RIGHT_PAREN ')'

// space and tab
// TODO: comment
int skipBlank(std::string &input, int startIndex);
// read a word as symbol
int readSymbol(std::string &input, int startIndex);
// a lisp form
int readForm(std::string &input, int startIndex);
