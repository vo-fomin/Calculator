//Created by Volodymyr Fomin on 07/02/2018

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include "Evaluator.h"

#ifndef NDEBUG
#include <iostream>
#endif

unsigned int Evaluator::_freeId(0);
const double Evaluator::_eps(1E-9);
const Evaluator::UnaryOperator Evaluator::_unaryOperators[]={&fac, &percent};
const Evaluator::BinaryOperator Evaluator::_binaryOperators[]={&power, &modulo};

Evaluator::Evaluator(const HashMap<string, Function>& functions, const string& exp, const bool& rad)
	: _id(++_freeId), _pos(-1), _ch(0), _expression(exp), _functions(functions), _rad(rad)
{
#ifndef NDEBUG
	cout << "Evaluator ID-" << _id << " created" << endl;
#endif
}
Evaluator::Evaluator(const Evaluator& ev)
	: _id(++_freeId), _pos(-1), _ch(0), _expression(ev._expression), _functions(ev._functions), _rad(ev._rad)
{
#ifndef NDEBUG
	cout << "Evaluator ID-" << _id << " copied" << endl;
#endif
}
Evaluator::~Evaluator()
{
#ifndef NDEBUG
	cout << "Evaluator ID-" << _id << " destroyed" << endl;
#endif
}

Evaluator& Evaluator::operator=(const Evaluator& ev)
{
	if(this!=&ev)
	{
		_expression = ev._expression;
		_rad = ev._rad;
		_functions = ev._functions;
		_pos = -1;
		_ch = 0;
	}
	return *this;
}

Evaluator::operator Element()
{
	return parse();
}

const double Evaluator::toRadians(const double deg)
{
	return (deg*M_PI)/180.f;
}
const double Evaluator::toDegrees(const double rad)
{
	return (rad*180.f)/M_PI;
}

const unsigned long long int Evaluator::factorial(const unsigned int n)
{
	if(n==0 || n==1) return 1;
    return n * factorial(n-1);
}

void Evaluator::nextChar() const
{
	_ch = (++_pos < static_cast<int>(_expression.length())) ? _expression[_pos] : -1;
}
const bool Evaluator::eat(const char& c) const
{
	while(_ch == ' ') 
		nextChar();
	if (_ch == c)
	{
		nextChar();
		return true;
	}
	return false;
}

const Evaluator::Element Evaluator::parse() const
{
	if(!_expression.compare(""))
		throw EvaluatorException("Expression undefined");
	nextChar();
	//Expression evaluation
	double x(parseExpression());
	if (_pos < static_cast<int>(_expression.length()) && _ch!='P' && _ch!='I' && _ch!='e') 
		throw EvaluatorException(string("Unexpected character: ")+=_ch);
	_pos = -1;
	_ch = 0;
	if(abs(x)<_eps)
		x=0;
	return x;
}
const Evaluator::Element Evaluator::parseExpression() const
{
    //Calculating operations with greater priority first
    double x(parseTerm());
    //Afterwards move to addition/subtraction
    while(true)
    {
        if (eat('+')) 
			x += parseTerm(); //Addition
        else {
            if (eat('-')) 
				x -= parseTerm(); //Subtraction
			//If there are no operators of addition/subtraction -> return the result
            else 
				return x;
        }
    }
}
const Evaluator::Element Evaluator::parseTerm() const
{
    //Calculate operations with greater priority first
    double x(parseFactor());
    //Afterwards move to multiplication/division
    while(true)
    {
		if(eat('*')) 
			x *= parseFactor(); //Multiplication
        else 
		{
            if(eat('/')) 
				x /= parseFactor(); //Division
			//If there are no operators of multiplication/division -> return the result
            else 
				return x;
        }
    }
}
const Evaluator::Element Evaluator::parseFactor() const
{
    //Firstly process sign
	//Return later evaluated result with appropriate sign
    if (eat('+')) 
		return parseFactor();
    if (eat('-')) 
		return -parseFactor();
	//The result
    double x(0);
	//Remember position from which processing started to get a substring from the whole expression
    int startPos(_pos);
    if (eat('('))
    {
        //Read braces
        //Start evaluating sub expression as a new expression
        x = parseExpression();
        eat(')');
    }
    else 
	{
        //If currently read char is a number move through expression until come across a non-digit, not dot or not letters responsible for constants
        //If constant is read -> replace with number and process it again
		if ((_ch >= '0' && _ch <= '9') || _ch == '.' || (_ch == 'p' && _expression[_pos+1] == 'i') || (_ch == 'P' && _expression[_pos+1] == 'I') || _ch == 'e' || _ch == 'E') 
		{
            x = parseNumber(startPos);
        } 
		else 
		{
            //If read letter not responsible for constant -> process as a function
            //Move through expression until come across a non-letter character. Then store read substring and find appropriate function for it
            //If appropriate function hadn't been found -> throw an exception
            if (_ch >= 'a' && _ch <= 'z') 
			{
                x = parseFunction(startPos);
			}
			else 
				throw EvaluatorException(string("Unexpected character: ")+=_ch);
		} 
	}
    //After processing numbers/functions process operators
    x = parseOperator(x);
    return x;
}
const Evaluator::Element Evaluator::parseNumber(const int startPos) const
{
	double res(0);
	if ((_ch == 'P' && _expression[_pos+1] == 'I') || (_ch == 'p' && _expression[_pos+1] == 'i')) 
	{
		res = M_PI;
		nextChar();
		nextChar();
	}
    else if (_ch == 'e' || _ch == 'E') 
	{
		res = M_E;
		nextChar();
	}
	else
	{
		_ch = _expression[_pos];
		while ((_ch >= '0' && _ch <= '9') || _ch == '.') 
			nextChar();
		//Parsing read number
		res = atof((_expression.substr(startPos, _pos-startPos)).c_str());
	}
	return res;
}
const Evaluator::Element Evaluator::parseFunction(const int startPos) const
{
	double res(0);
	while (_ch >= 'a' && _ch <= 'z') 
		nextChar();
    string func(_expression.substr(startPos, _pos-startPos));
    res = parseFactor();
	try
	{
		res = _functions[func](res, _rad);
	}
	catch(...)
	{
		throw EvaluatorException("Unexpected function: " + func);
	}
	return res;
}
const Evaluator::Element Evaluator::parseOperator(const double calculated) const
{
	double res(calculated);
	if (eat('^')) 
		res = _binaryOperators[0](res, parseExpression()); //Raising to power
    else if (eat('!')) //Factorial
		res = _unaryOperators[0](res);
	//Percents
	else if (eat('%'))
		res = _unaryOperators[1](res);
	//Modulo
    else if (eat('&')) 
		res = _binaryOperators[1](res, parseExpression());
	return res;
}

const string& Evaluator::getExpression() const
{
	return _expression;
}
void Evaluator::setExpression(const string& exp)
{
	_expression=exp;
	_ch=0;
	_pos=-1;
}

bool& Evaluator::rad()
{
	return _rad;
}