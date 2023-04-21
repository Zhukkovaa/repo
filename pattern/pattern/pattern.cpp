#include <iostream>
#include <cassert>    
#include <string> 

using namespace std;
struct Transformer;
struct Number;
struct BinaryOperation;
struct FunctionCall;
struct Variable;
struct Expression //базовая абстрактная структура
{
	virtual ~Expression() { } //виртуальный деструктор
	virtual double evaluate() const = 0; //абстрактный метод «вычислить»
	virtual Expression* transform(Transformer* tr) const = 0;
	virtual std::string print() const = 0;
};
struct Transformer
{
	virtual ~Transformer() {}
	virtual Expression* transformNumber(Number const*) = 0;
	virtual Expression* transformBinaryOperation(BinaryOperation const*) = 0;
	virtual Expression* transformFunctionCall(FunctionCall const*) = 0;
	virtual Expression* transformVariable(Variable const*) = 0;
};
struct Number : Expression // стуктура «Число»
{
	Number(double value) : value_(value) {} //конструктор
	double value() const { return value_; } // метод чтения значения числа
	double evaluate() const { return value_; } // реализация виртуального метода «вычислить»
	~Number() {}//деструктор, тоже виртуальный
	Expression* transform(Transformer* tr) const
	{
		return tr->transformNumber(this);
	}
	std::string print() const {
		return std::to_string(this->value_);
	}
private:
	double value_; // само вещественное число
};
struct BinaryOperation : Expression // «Бинарная операция»
{
	enum { // перечислим константы, которыми зашифруем символы операций
		PLUS = '+',
		MINUS = '-',
		DIV = '/',
		MUL = '*'
	};
	// в конструкторе надо указать 2 операнда — левый и правый, а также сам символ операции
	BinaryOperation(Expression const* left, int op, Expression const* right) :
		left_(left), op_(op), right_(right)
	{
		assert(left_ && right_);
	}
	~BinaryOperation() //в деструкторе освободим занятую память
	{
		delete left_;
		delete right_;
	}
	Expression const* left() const { return left_; } // чтение левого операнда
	Expression const* right() const { return right_; } // чтение правого операнда
	int operation() const { return op_; } // чтение символа операции
	double evaluate() const // реализация виртуального метода «вычислить»
	{
		double left = left_->evaluate(); // вычисляем левую часть
		double right = right_->evaluate(); // вычисляем правую часть
		switch (op_) // в зависимости от вида операции, складываем, вычитаем, умножаем  или делим левую и правую части
		{
		case PLUS: return left + right;
		case MINUS: return left - right;
		case DIV: return left / right;
		case MUL: return left * right;
		}
	}
	Expression* transform(Transformer* tr) const
	{
		return tr->transformBinaryOperation(this);
	}
	std::string print() const {
		return this->left_->print() + std::string(1, this->op_) + this->right_->print();
	}
private:
	Expression const* left_; // указатель на левый операнд
	Expression const* right_; // указатель на правый операнд
	int op_; // символ операции
};
struct FunctionCall : Expression // структура «Вызов функции»
{
	// в конструкторе надо учесть имя функции и ее аогумент
	FunctionCall(std::string const& name, Expression const* arg) : name_(name),
		arg_(arg)
	{
		assert(arg_);
		assert(name_ == "sqrt" || name_ == "abs");
	}
	// разрешены только вызов sqrt и abs
	std::string const& name() const
	{
		return name_;
	}
	Expression const* arg() const // чтение аргумента функции
	{
		return arg_;
	}
	~FunctionCall() { delete arg_; } // освобождаем память в деструкторе
	virtual double evaluate() const { // реализация виртуального метода
		//«вычислить»
		if (name_ == "sqrt")
			return sqrt(arg_->evaluate()); // либо вычисляем корень квадратный
		else return fabs(arg_->evaluate());
	} // либо модуль — остальные функции запрещены
	Expression* transform(Transformer* tr) const
	{
		return tr->transformFunctionCall(this);
	}
	std::string print() const {
		return this->name_ + "(" + this->arg_->print() + ")";
	}
private:
	std::string const name_; // имя функции
	Expression const* arg_; // указатель на ее аргумент
};
struct Variable : Expression // структура «Переменная»
{
	Variable(std::string const& name) : name_(name) { } //в конструкторе надо
	//указать ее имя
	std::string const& name() const { return name_; } // чтение имени переменной
	double evaluate() const // реализация виртуального метода «вычислить»
	{
		return 0.0;
	}
	Expression* transform(Transformer* tr) const
	{
		return tr->transformVariable(this);
	}
	std::string print() const {
		return this->name_;
	}
private:
	std::string const name_; // имя переменной
};
struct CopySyntaxTree : Transformer
{
	Expression* transformNumber(Number const* number)
	{
		Expression* exp = new Number(number->value());
		return exp;
	}
	Expression* transformBinaryOperation(BinaryOperation const* binop)
	{
		Expression* exp = new BinaryOperation((binop->left())->transform(this),
			binop->operation(),
			(binop->right())->transform(this));
		return exp;
	}
	Expression* transformFunctionCall(FunctionCall const* fcall)
	{
		Expression* exp = new FunctionCall(fcall->name(),
			(fcall->arg())->transform(this));
		return exp;
	}
	Expression* transformVariable(Variable const* var)
	{
		Expression* exp = new Variable(var->name());
		return exp;
	}
	~CopySyntaxTree() { };
};
struct FoldConstants : Transformer
{
	Expression* transformNumber(Number const* number)
	{
		Expression* exp = new Number(number->value());
		return exp;
	}
	Expression* transformBinaryOperation(BinaryOperation const* binop)
	{
		// 1. Создаем указатели на левое и правое выражение
		Expression* nleft = (binop->left())->transform(this);
		Expression* nright = (binop->right())->transform(this);
		int noperation = binop->operation();

		// 2. Создаем новый объект типа BinaryOperation с новыми указателями
		BinaryOperation* nbinop = new BinaryOperation(nleft,
			noperation,
			// binop->operation(),
			nright);
		// 3. Проверяем на приводимость указателей к типу Number
		Number* nleft_is_number = dynamic_cast<Number*>(nleft);
		Number* nright_is_number = dynamic_cast<Number*>(nright);
		if (nleft_is_number && nright_is_number) {
			// 3.1 Вычисляем значение выражения
			Expression* result = new Number(binop->evaluate());

			// 3.2 Освобождаем память
			delete nbinop;
			// 3.3 Возвращаем результат
			return result;
		}
		else {
			return nbinop;
		}
	}
	Expression* transformFunctionCall(FunctionCall const* fcall)
	{
		// 1. Создаем указатель на аргумент
		Expression* narg = (fcall->arg())->transform(this);
		std::string const& nname = fcall->name();

		// 2. Создаем новый объект типа FunctionCall с новым указателем
		FunctionCall* nfcall = new FunctionCall(nname,narg);

		// 3. Проверяем на приводимость указателя к типу Number
		Number* narg_is_number = dynamic_cast<Number*>(narg);
		if (narg_is_number) {
			// 3.1 Вычисляем значение выражения
			Expression* result = new Number(fcall->evaluate());

			// 3.2 Освобождаем память
			delete nfcall;
			// 3.3 Возвращаем результат
			return result;
		}
		else {
			return nfcall;
		}
	}
	Expression* transformVariable(Variable const* var)
	{
		Expression* exp = new Variable(var->name());
		return exp;
	}
	~FoldConstants() { };
};


int main()
{   /*
	//------------------------------------------------------------------------------
	Expression * e1 = new Number(1.234);
	Expression * e2 = new Number(-1.234);
	Expression * e3 = new BinaryOperation(e1, BinaryOperation::DIV, e2);
	cout << e3->evaluate() << endl;
	//------------------------------------------------------------------------------
	Expression* n32 = new Number(32.0);
	Expression* n16 = new Number(16.0);
	Expression* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
	Expression* callSqrt = new FunctionCall("sqrt", minus);
	Expression* n2 = new Number(2.0);
	Expression* mult = new BinaryOperation(n2, BinaryOperation::MUL, callSqrt);
	Expression* callAbs = new FunctionCall("abs", mult);
	cout << callAbs->evaluate() << endl;
	//------------------------------------------------------------------------------
	*/


	/*
	Number* n32 = new Number(32.0);
	Number* n16 = new Number(16.0);
	BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
	FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
	Variable* var = new Variable("var");
	BinaryOperation* mult = new BinaryOperation(var, BinaryOperation::MUL,
		callSqrt);
	FunctionCall* callAbs = new FunctionCall("abs", mult);
	CopySyntaxTree CST;
	Expression* newExpr = callAbs->transform(&CST);
	std::cout << newExpr->print() << std::endl;
	*/
	
	Number* n32 = new Number(32.0);
	Number* n16 = new Number(16.0);
	BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS,
		n16);
	FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
	Variable* var = new Variable("var");
	BinaryOperation* mult = new BinaryOperation(var, BinaryOperation::MUL,
		callSqrt);
	FunctionCall* callAbs = new FunctionCall("abs", mult);
	FoldConstants FC;
	Expression* newExpr2 = callAbs->transform(&FC);
	//FoldConstants FC;
	std::cout << newExpr2->print() << std::endl;

	/*
	Number* n32 = new Number(32.0);
    Number* n16 = new Number(16.0);
    BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
    FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
    Variable* var = new Variable("var");
    BinaryOperation* mult = new BinaryOperation(var, BinaryOperation::MUL, callSqrt);
    FunctionCall* callAbs = new FunctionCall("abs", mult);
    CopySyntaxTree CST;
    std::cout << callAbs->transform(&CST)->print() << std::endl;
    FoldConstants FC;
    std::cout << callAbs->transform(&FC)->print() << std::endl;
	*/
}