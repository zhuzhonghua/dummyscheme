// Calculator.cpp : Defines the entry point for the console application.
//


///////////////////////////////////////////////////////////////////////////////
//	作者：江贺
//	日期：2004年9月11日
//	名称：可编程计算器
//	功能描述：
//		本计算器可以实现对诸如 3+(1+7)*6+4*((5+4))这样的表达式的计
//		算，目前仅限于带括号的 +, * 运算
//	实现描述：
//		本计算器的是在教材 pp.49, 文法 (3.8)基础上稍加修改后，利用自上而下的递归
//		程序实现。每个非终结符对应一个分析函数。
//		修改后的文法如下所示，其中 e 代表空串
//
//		E-->TE'
//		E'-->+TE'|e
//		T-->FT'
//		T'-->*FT'|e
//		F-->(E)|num
//
//		程序中使用了一个词法分析函数 yylex，变量yylval代表最近的整数记号的数值。
//		'\0'对应着$符号，预示着输入串结束.
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stdlib.h"
#include "conio.h"
#include <string.h>


// 这一段是记号的定义
#define ADD 0
#define MUL 1
#define LBRACE 2
#define RBRACE 3
#define NUM 4
#define END 5
#define OTHER 6
#define MINUS 7
#define DIV 8

char input[200];	// 输入串。
int lookahead;
int pCur;
int yylval;

// 函数声明
int yylex();
void Match(int t);
long T();
long E_();
long E();
float T_();
long F();

// 词法分析器，读入一个记号
int yylex()
{
	char num[20];
	int temp = 0;

	// 过滤掉空白
	while (input[pCur] == ' ') pCur++;

	// 如果是数字，那么把这个记号的数值计算出来放在 yylval 中
	while (input[pCur] >= '0' && input[pCur] <= '9') {
		num[temp++] = input[pCur++];
	}
	if (temp > 0)
	{
		sscanf(num, "%d", &yylval);
		return NUM;
	}

	// 其他记号的处理
	switch (input[pCur++])	// 注意：这里指针往前移了一位
	{
	case '+': return ADD;
	case '-': return MINUS;
	case '*':return MUL;
	case '/':return DIV;
	case '(':return LBRACE;
	case ')':return RBRACE;
	case '\0': return END;
	default: return OTHER;
	}
}

// 匹配函数，若当前记号与参数相同，则读入下一个记号
void Match(int t)
{
	if (lookahead == t) lookahead = yylex();
	else
	{
		printf("\n Error\n");

		exit(0);
	}
}

// 处理 T-->FT'
long T()
{
	switch (lookahead)
	{
	case LBRACE:	// FIRST(FT')={(,num}
	case NUM: {
		long f = F();
		float t_ = T_();
		return f * t_;
		//return F()*T_();
	}
	default:
		printf("\n Error\n");

		exit(0);
	}
}

// 处理 E'-->+TE'|e
long E_()
{
	switch (lookahead)
	{
	case ADD: {	// E'-->+TE' 的情况， FIRST(E')={+,e}
		Match(ADD);
		long t = T();
		long e_ = E_();
		//return T() + E_();
		return t + e_;
	}
	case MINUS: {
		Match(MINUS);
		long t = T();
		long e_ = E_();
		return -1 * t + e_;
		//return -1 * T() + E_();
	}
	case RBRACE:// E'-->e 的情况，这个时候需要处理 FOLLOW集合， FOLLOW(E')={), $}
	case END:
		return 0;
	default:
		printf("\n Error\n");

		exit(0);
	}
}

// 处理 E-->TE'
long E()
{
	switch (lookahead)
	{
	case LBRACE:	// FIRST(TE')={(,num}
	case NUM: {
		long t = T();
		long e_ = E_();
		//return T() + E_();
		return t + e_;
		//		case END:		// FOLLOW(E)={),$}
		//			return 0;
	}
	default:
		printf("\n Error\n");

		exit(0);
	}
}

// 处理 T'-->*FT'|e
float T_()
{
	switch (lookahead)
	{
	case MUL: {	// FIRST(*FT')={*}
		Match(MUL);
		long f = F();
		float t_ = T_();
		return f * t_;
		//return F() * T_();
	}
	case DIV: {
		Match(DIV);
		long f = F();
		float t_ = T_();
		return 1.0f / f*t_;
		//return 1 / F()*T_();
	}
	case ADD:	// T'-->e 的情况，这个时候需要处理 FOLLOW集合， FOLLOW(T')={+,),$}
	case MINUS:
	case RBRACE:
	case END:
		return 1;
	default:
		printf("\n Error\n");

		exit(0);
	}
}

// 处理 F-->(E)|num
long F()
{
	int temp;

	switch (lookahead)
	{
	case LBRACE:	// FIRST((E))={(}
		Match(LBRACE);
		temp = E();
		Match(RBRACE);
		return temp;
	case NUM:		// FIRST(num) = {num}
		temp = yylval;
		Match(NUM);
		return temp;
	default:
		printf("\n Error\n");

		exit(0);
	}
}


int main(int argc, char* argv[])
{


	pCur = 0;

	// 读入输入串
	//printf("Please input the string:");
	//scanf("%s", input);
	strcpy(input, "(1+2)/3+1");
	// lookahead 赋初值
	lookahead = yylex();

	// 调用 开始符号E 对应的处理过程来处理输入串
	printf("The answer is %d\n", E());


	scanf("%s", input);

	return 0;
}



