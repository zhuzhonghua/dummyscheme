// Calculator.cpp : Defines the entry point for the console application.
//


///////////////////////////////////////////////////////////////////////////////
//	���ߣ�����
//	���ڣ�2004��9��11��
//	���ƣ��ɱ�̼�����
//	����������
//		������������ʵ�ֶ����� 3+(1+7)*6+4*((5+4))�����ı��ʽ�ļ�
//		�㣬Ŀǰ�����ڴ����ŵ� +, * ����
//	ʵ��������
//		�������������ڽ̲� pp.49, �ķ� (3.8)�������Լ��޸ĺ��������϶��µĵݹ�
//		����ʵ�֡�ÿ�����ս����Ӧһ������������
//		�޸ĺ���ķ�������ʾ������ e ����մ�
//
//		E-->TE'
//		E'-->+TE'|e
//		T-->FT'
//		T'-->*FT'|e
//		F-->(E)|num
//
//		������ʹ����һ���ʷ��������� yylex������yylval��������������Ǻŵ���ֵ��
//		'\0'��Ӧ��$���ţ�Ԥʾ�����봮����.
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stdlib.h"
#include "conio.h"
#include <string.h>


// ��һ���ǼǺŵĶ���
#define ADD 0
#define MUL 1
#define LBRACE 2
#define RBRACE 3
#define NUM 4
#define END 5
#define OTHER 6
#define MINUS 7
#define DIV 8

char input[200];	// ���봮��
int lookahead;
int pCur;
int yylval;

// ��������
int yylex();
void Match(int t);
long T();
long E_();
long E();
float T_();
long F();

// �ʷ�������������һ���Ǻ�
int yylex()
{
	char num[20];
	int temp = 0;

	// ���˵��հ�
	while (input[pCur] == ' ') pCur++;

	// ��������֣���ô������Ǻŵ���ֵ����������� yylval ��
	while (input[pCur] >= '0' && input[pCur] <= '9') {
		num[temp++] = input[pCur++];
	}
	if (temp > 0)
	{
		sscanf(num, "%d", &yylval);
		return NUM;
	}

	// �����ǺŵĴ���
	switch (input[pCur++])	// ע�⣺����ָ����ǰ����һλ
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

// ƥ�亯��������ǰ�Ǻ��������ͬ���������һ���Ǻ�
void Match(int t)
{
	if (lookahead == t) lookahead = yylex();
	else
	{
		printf("\n Error\n");

		exit(0);
	}
}

// ���� T-->FT'
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

// ���� E'-->+TE'|e
long E_()
{
	switch (lookahead)
	{
	case ADD: {	// E'-->+TE' ������� FIRST(E')={+,e}
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
	case RBRACE:// E'-->e ����������ʱ����Ҫ���� FOLLOW���ϣ� FOLLOW(E')={), $}
	case END:
		return 0;
	default:
		printf("\n Error\n");

		exit(0);
	}
}

// ���� E-->TE'
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

// ���� T'-->*FT'|e
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
	case ADD:	// T'-->e ����������ʱ����Ҫ���� FOLLOW���ϣ� FOLLOW(T')={+,),$}
	case MINUS:
	case RBRACE:
	case END:
		return 1;
	default:
		printf("\n Error\n");

		exit(0);
	}
}

// ���� F-->(E)|num
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

	// �������봮
	//printf("Please input the string:");
	//scanf("%s", input);
	strcpy(input, "(1+2)/3+1");
	// lookahead ����ֵ
	lookahead = yylex();

	// ���� ��ʼ����E ��Ӧ�Ĵ���������������봮
	printf("The answer is %d\n", E());


	scanf("%s", input);

	return 0;
}



