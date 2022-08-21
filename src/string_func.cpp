/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_func.cpp Safe string functions. */

#include "stdafx.h"
#include "string_func.h"

/**
 * Safe string copy (as in, the destination is always terminated).
 * @param dest Destination of the string.
 * @param src  Source of the string.
 * @param size Number of bytes (including terminating 0) from \a src.
 * @return Copied string.
 * @note The ::SafeStrncpy(uint8 *dest, const uint8 *src, int size) function is preferred.
 */
char *SafeStrncpy(char *dest, const char *src, int size)
{
	assert(size >= 1);

	strncpy(dest, src, size);
	dest[size - 1] = '\0';
	return dest;
}

/**
 * Safe string copy (as in, the destination is always terminated).
 * @param dest Destination of the string.
 * @param src  Source of the string.
 * @param size Number of bytes (including terminating 0) from \a src.
 * @return Copied string.
 */
uint8 *SafeStrncpy(uint8 *dest, const uint8 *src, int size)
{
	assert(size >= 1);

	strncpy((char *)dest, (const char *)src, size);
	dest[size - 1] = '\0';
	return dest;
}

/**
 * Duplicate a string.
 * @param src Source string.
 * @return Copy of the string in its own memory.
 * @note String must be deleted later!
 */
char *StrDup(const char *src)
{
	size_t n = strlen(src);
	char *mem = new char[n + 1];
	assert(mem != nullptr);

	return SafeStrncpy(mem, src, n + 1);
}

/**
 * String copy up to an end-point of the destination.
 * @param dest Start of destination buffer.
 * @param end End of destination buffer (\c *end is never written).
 * @param src Source string.
 * @return \a dest.
 */
uint8 *StrECpy(uint8 *dest, uint8 *end, const uint8 *src)
{
	uint8 *dst = dest;
	end--;
	while (*src && dest < end) {
		*dest = *src;
		dest++;
		src++;
	}
	*dest = '\0';
	return dst;
}

/**
 * Get the length of an utf-8 string in bytes.
 * @param str String to inspect.
 * @return Length of the given string in bytes.
 */
size_t StrBytesLength(const uint8 *str)
{
	return strlen((const char *)str);
}

/**
 * Decode an UTF-8 character.
 * @param data Pointer to the start of the data.
 * @param length Length of the \a data buffer.
 * @param[out] codepoint If decoding was successful, the value of the decoded character.
 * @return Number of bytes read to decode the character, or \c 0 if reading failed.
 */
int DecodeUtf8Char(const char *data, size_t length, uint32 *codepoint)
{
	if (length < 1) return 0;
	uint32 value = *data;
	data++;
	if ((value & 0x80) == 0) {
		*codepoint = value;
		return 1;
	}
	int size;
	uint32 min_value;
	if ((value & 0xE0) == 0xC0) {
		size = 2;
		min_value = 0x80;
		value &= 0x1F;
	} else if ((value & 0xF0) == 0xE0) {
		size = 3;
		min_value = 0x800;
		value &= 0x0F;
	} else if ((value & 0xF8) == 0xF0) {
		size = 4;
		min_value = 0x10000;
		value &= 0x07;
	} else {
		return 0;
	}

	if (length < static_cast<size_t>(size)) return 0;
	for (int n = 1; n < size; n++) {
		char val = *data;
		data++;
		if ((val & 0xC0) != 0x80) return 0;
		value = (value << 6) | (val & 0x3F);
	}
	if (value < min_value || (value >= 0xD800 && value <= 0xDFFF) || value > 0x10FFFF) return 0;
	*codepoint = value;
	return size;
}

/**
 * Encode a code point into UTF-8.
 * @param codepoint Code point to encode.
 * @param dest [out] If supplied and not \c nullptr, the encoded character is written to this position. String is \em not terminated.
 * @return Length of the encoded character in bytes.
 * @note It is recommended to use this function for measuring required output size (by making \a dest a \c nullptr), before writing the encoded string.
 */
int EncodeUtf8Char(uint32 codepoint, char *dest)
{
	if (codepoint < 0x7F + 1) {
		/* 7 bits, U+0000 .. U+007F, 1 byte: 0xxx.xxxx */
		if (dest != nullptr) *dest = codepoint;
		return 1;
	}
	if (codepoint < 0x7FF + 1) {
		/* 11 bits, U+0080 .. U+07FF, 2 bytes: 110x.xxxx, 10xx.xxxx */
		if (dest != nullptr) {
			dest[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
			dest[1] = 0x80 | (codepoint & 0x3F);
		}
		return 2;
	}
	if (codepoint < 0xFFFF + 1) {
		/* 16 bits, U+0800 .. U+FFFF, 3 bytes: 1110.xxxx, 10xx.xxxx, 10xx.xxxx */
		if (dest != nullptr) {
			dest[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
			dest[1] = 0x80 | ((codepoint >> 6) & 0x3F);
			dest[2] = 0x80 | (codepoint & 0x3F);
		}
		return 3;
	}
	assert(codepoint <= 0x10FFFF); // Max Unicode code point, RFC 3629.

	/* 21 bits, U+10000 .. U+1FFFFF, 4 bytes: 1111.0xxx, 10xx.xxxx, 10xx.xxxx, 10xx.xxxx */
	if (dest != nullptr) {
		dest[0] = 0xF0 | ((codepoint >> 18) & 0x07);
		dest[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		dest[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		dest[3] = 0x80 | (codepoint & 0x3F);
	}
	return 4;
}

/**
 * Find the previous character in the given string, skipping over multi-byte characters.
 * @param data String to work with.
 * @param pos Index in the string to start searching from.
 * @return The previous character's index.
 */
size_t GetPrevChar(const std::string &data, size_t pos)
{
	if (pos == 0) return 0;
	do {
		pos--;
	} while (pos > 0 && (data[pos] & 0xc0) == 0x80);
	return pos;
}

/**
 * Find the next character in the given string, skipping over multi-byte characters.
 * @param data String to work with.
 * @param pos Index in the string to start searching from.
 * @return The next character's index.
 */
size_t GetNextChar(const std::string &data, size_t pos)
{
	const size_t length = data.size();
	if (pos >= length) return length;
	do {
		pos++;
	} while (pos < length && (data[pos] & 0xc0) == 0x80);
	return pos;
}

/**
 * Are the two strings equal?
 * @param s1 First string to compare.
 * @param s2 Second string to compare.
 * @return strings are exactly equal.
 */
bool StrEqual(const uint8 *s1, const uint8 *s2)
{
	assert(s1 != nullptr && s2 != nullptr);
	while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
		s1++;
		s2++;
	}
	return *s1 == '\0' && *s2 == '\0';
}

/**
 * Test whether \a str ends with \a end.
 * @param str String to check.
 * @param end Expected end text.
 * @param case_sensitive Compare case sensitively.
 * @return \a str ends with the given \a end possibly after changing case.
 */
bool StrEndsWith(const char *str, const char *end, bool case_sensitive)
{
	size_t str_length = strlen(str);
	size_t end_length = strlen(end);
	if (end_length > str_length) return false;
	if (end_length == 0) return true;

	str += str_length - end_length;
	while (*str != '\0' && *end != '\0') {
		char s = *str;
		char e = *end;
		if (!case_sensitive && s >= 'A' && s <= 'Z') s += 'a' - 'A';
		if (!case_sensitive && e >= 'A' && e <= 'Z') e += 'a' - 'A';
		if (s != e) return false;
		str++; end++;
	}
	return true;
}

/** Implementation details for parsing expression strings. */
namespace EvaluateableExpressionImpl {

struct AbstractExpression;
using Stack = std::vector<std::unique_ptr<AbstractExpression>>;

#define ERROR(msg, ...) do { printf("Impossible operation: " msg "\n", ##__VA_ARGS__); exit(1); } while (false)

/** A piece of an expression. */
struct AbstractExpression : public EvaluateableExpression {
	/**
	 * Whether this expression and its left and/or right neighbour can be collapsed into a single expression.
	 * @return The expression is reducible.
	 */
	virtual bool Reducible() const = 0;

	/**
	 * Ordering for determining which types of expressions should be reduced first.
	 * Expressions with higher precedence are reduced earlier.
	 * Expressions with equal precedence are reduced left to right.
	 * @return The precedence rating.
	 */
	virtual int Precedence() const = 0;

	/**
	 * Collapse this expression and its left and/or right neighbour into a single expression, if possible.
	 * @param stack The stack of all expressions.
	 * @param own_index This expression's index in the %stack.
	 * @note If reduction succeeds, left and/or right neighbour may be \c nullptr afterwards.
	 */
	virtual void Reduce(Stack &stack, int own_index) = 0;
};

/** Represents the expression placeholder. */
struct VariableExpression : public AbstractExpression {
	bool Reducible() const override { return false; }
	int Precedence() const override { return 1; }
	int Eval(int n) const override
	{
		return n;
	}
	void Reduce(Stack&, int) override { ERROR("Variables are irreducible"); }
};

/** An integer number. */
struct LiteralExpression : public AbstractExpression {
	explicit LiteralExpression(int v) : value(v) {}
	bool Reducible() const override { return false; }
	int Precedence() const override { return 1; }
	int Eval(int /* n */) const override
	{
		return this->value;
	}
	int value;
	void Reduce(Stack&, int) override { ERROR("Literals are irreducible"); }
};

/** An expression wrapped in parentheses. */
struct ParenthesisedExpression : public AbstractExpression {
	bool Reducible() const override { return false; }
	int Precedence() const override { return 1; }
	int Eval(int n) const override
	{
		return this->expr->Eval(n);
	}
	std::unique_ptr<AbstractExpression> expr = nullptr;
	void Reduce(Stack&, int) override { ERROR("Parentheses are irreducible"); }
};

/** Inverts the boolean value of an expression. */
struct UnaryNot : public AbstractExpression {
	bool Reducible() const override { return this->expr == nullptr; }
	int Precedence() const override { return 100; }
	int Eval(int n) const override
	{
		return this->expr->Eval(n) == 0 ? 1 : 0;
	}
	void Reduce(Stack &stack, int own_index) override
	{
		if (this->expr != nullptr) return;
		if (own_index + 1 >= static_cast<int>(stack.size())) ERROR("Unary NOT without expression");
		this->expr = std::move(stack.at(own_index + 1));
	}
	std::unique_ptr<AbstractExpression> expr = nullptr;
};

/** Performs an operation on two expressions. */
struct BinaryOperator : public AbstractExpression {
	bool Reducible() const override { return this->left == nullptr || this->right == nullptr; }
	void Reduce(Stack &stack, int own_index) override
	{
		if (this->right == nullptr) {
			if (own_index + 1 >= static_cast<int>(stack.size())) ERROR("Binary operator without right-hand expression");
			this->right = std::move(stack.at(own_index + 1));
		}
		if (this->left == nullptr) {
			if (own_index <= 0) ERROR("Binary operator without left-hand expression");
			this->left = std::move(stack.at(own_index - 1));
		}
	}
	std::unique_ptr<AbstractExpression> left = nullptr;
	std::unique_ptr<AbstractExpression> right = nullptr;
};

/** Groups the on-true and on-false cases of a ternary operator. */
struct TernaryEvaluate : public BinaryOperator {
	int Precedence() const override { return 60; }
	int Eval(int) const override { ERROR("Ternary evaluate construct must never be called directly!"); }
};

/** The ternary operator. */
struct TernaryCondition : public AbstractExpression {
	bool Reducible() const override { return this->condition == nullptr || this->evaluate == nullptr; }
	int Precedence() const override { return 50; }
	int Eval(int n) const override
	{
		return this->condition->Eval(n) != 0 ? this->evaluate->left->Eval(n) : this->evaluate->right->Eval(n);
	}
	void Reduce(Stack &stack, int own_index) override
	{
		if (this->evaluate == nullptr) {
			if (own_index + 1 >= static_cast<int>(stack.size())) ERROR("Ternary operator without expression set");
			TernaryEvaluate *e = dynamic_cast<TernaryEvaluate*>(stack.at(own_index + 1).release());
			if (e == nullptr) ERROR("Ternary operator with single expression");
			this->evaluate.reset(e);
		}
		if (this->condition == nullptr) {
			if (own_index <= 0) ERROR("Ternary operator without condition");
			this->condition = std::move(stack.at(own_index - 1));
		}
	}
	std::unique_ptr<AbstractExpression> condition = nullptr;
	std::unique_ptr<TernaryEvaluate> evaluate = nullptr;
};

/** The addition operator. */
struct BinaryPlus : public BinaryOperator {
	int Precedence() const override { return 20; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) + this->right->Eval(n);
	}
};

/** The subtraction operator. */
struct BinaryMinus : public BinaryOperator {
	int Precedence() const override { return 20; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) - this->right->Eval(n);
	}
};

/** The multiplication operator. */
struct BinaryMult : public BinaryOperator {
	int Precedence() const override { return 30; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) * this->right->Eval(n);
	}
};

/** The division operator. */
struct BinaryDiv : public BinaryOperator {
	int Precedence() const override { return 30; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) / this->right->Eval(n);
	}
};

/** The modulo operator. */
struct BinaryMod : public BinaryOperator {
	int Precedence() const override { return 30; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) % this->right->Eval(n);
	}
};

/** The equality operator. */
struct BinaryEq : public BinaryOperator {
	int Precedence() const override { return 10; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) == this->right->Eval(n);
	}
};

/** The inequality operator. */
struct BinaryNeq : public BinaryOperator {
	int Precedence() const override { return 10; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) != this->right->Eval(n);
	}
};

/** The strictly-less-than operator. */
struct BinaryLt : public BinaryOperator {
	int Precedence() const override { return 10; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) < this->right->Eval(n);
	}
};

/** The strictly-greater-than operator. */
struct BinaryGt : public BinaryOperator {
	int Precedence() const override { return 10; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) > this->right->Eval(n);
	}
};

/** The less-or-equal operator. */
struct BinaryLeq : public BinaryOperator {
	int Precedence() const override { return 10; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) <= this->right->Eval(n);
	}
};

/** The greater-or-equal operator. */
struct BinaryGeq : public BinaryOperator {
	int Precedence() const override { return 10; }
	int Eval(int n) const override
	{
		return this->left->Eval(n) >= this->right->Eval(n);
	}
};

/** The logical and operator. */
struct BinaryAnd : public BinaryOperator {
	int Precedence() const override { return 5; }
	int Eval(int n) const override
	{
		return (this->left->Eval(n) != 0) && (this->right->Eval(n) != 0);
	}
};

/** The logical or operator. */
struct BinaryOr : public BinaryOperator {
	int Precedence() const override { return 4; }
	int Eval(int n) const override
	{
		return (this->left->Eval(n) != 0) || (this->right->Eval(n) != 0);
	}
};

#undef ERROR
#define ERROR(msg, ...) do { printf("Expression reducing error: " msg "\n", ##__VA_ARGS__); exit(1); } while (false)

/**
 * Reduce the expressions on the stack until no further reduction is possible.
 * @param stack Expression stack to operate on.
 */
static void Reduce(Stack &stack)
{
	/* As long as there is something we can reduce, look for the first sub-expr with the highest precedent score. */
	for (;;) {
		int stacksize = stack.size();
		int highest_reducible_precedence = 0;
		int index_to_reduce = -1;
		for (int i = 0; i < stacksize; ++i) {
			if (stack[i]->Reducible()) {
				int p = stack[i]->Precedence();
				if (p > highest_reducible_precedence) {
					highest_reducible_precedence = p;
					index_to_reduce = i;
				}
			}
		}
		if (highest_reducible_precedence == 0) return;

		stack[index_to_reduce]->Reduce(stack, index_to_reduce);

		for (int i = index_to_reduce - 1; i < stacksize; ++i) {
			if (stack.at(i) == nullptr) {
				for (int j = i + 1; j < stacksize; ++j) {
					stack.at(j - 1) = std::move(stack.at(j));
				}
				stack.pop_back();
				--stacksize;
				--i;
			}
		}
	}
}

#undef ERROR
#define ERROR(msg, ...) do { printf("Expression parsing error ['%s']: " msg "\n", input.c_str(), ##__VA_ARGS__); exit(1); } while (false)

/**
 * Parse the string representation of an expression.
 * @param input Expression string.
 * @return Evaluateable and fully reduced object.
 */
static std::unique_ptr<AbstractExpression> Parse(const std::string &input)
{
	Stack stack;

	const int in_len = input.size();
	for (int pos = 0; pos < in_len;) {
		if (input[pos] == ' ' || input[pos] == '\t') {
			++pos;
			continue;
		}

		if (input[pos] == 'n') {
			/* A variable. */
			stack.emplace_back(new VariableExpression);
			++pos;
			continue;
		}

		if (input[pos] == '(') {
			/* Parenthesised expression. Find the matching close-paren and reduce. */
			int depth = 0;
			int close_paren = pos + 1;
			for (;;) {
				if (close_paren >= in_len) ERROR("Unmatched opening parenthesis");
				if (input[close_paren] == '(') {
					++depth;
				} else if (input[close_paren] == ')') {
					if (depth > 0) {
						--depth;
					} else {
						break;
					}
				}
				++close_paren;
			}
			std::string substring = input.substr(pos + 1, close_paren - pos - 1);
			stack.push_back(Parse(substring));
			pos = close_paren + 1;
			continue;
		}

		if (input[pos] >= '0' && input[pos] <= '9') {
			/* A literal. */
			int val = input[pos] - '0';
			for (;;) {
				++pos;
				if (pos >= in_len || input[pos] < '0' || input[pos] > '9') break;
				val *= 10;
				val += input[pos] - '0';
			}
			stack.emplace_back(new LiteralExpression(val));
			continue;
		}

		if (input[pos] == '=') {
			/* Should be an equals sign. */
			if (pos + 1 >= in_len) ERROR("End of expression after '='");
			if (input[pos + 1] != '=') ERROR("Invalid token '%c' after '='", input[pos + 1]);
			stack.emplace_back(new BinaryEq);
			pos += 2;
			continue;
		}

		if (input[pos] == '&') {
			/* Should be an and sign. */
			if (pos + 1 >= in_len) ERROR("End of expression after '&'");
			if (input[pos + 1] != '&') ERROR("Invalid token '%c' after '&'", input[pos + 1]);
			stack.emplace_back(new BinaryAnd);
			pos += 2;
			continue;
		}

		if (input[pos] == '|') {
			/* Should be an or sign. */
			if (pos + 1 >= in_len) ERROR("End of expression after '|'");
			if (input[pos + 1] != '|') ERROR("Invalid token '%c' after '|'", input[pos + 1]);
			stack.emplace_back(new BinaryOr);
			pos += 2;
			continue;
		}

		if (input[pos] == '!') {
			/* Either a unary not or an unequals sign. */
			if (pos + 1 < in_len && input[pos + 1] == '=') {
				stack.emplace_back(new BinaryNeq);
				pos += 2;
				continue;
			}
			stack.emplace_back(new UnaryNot);
			++pos;
			continue;
		}

		if (input[pos] == '<') {
			/* Either a < or a <= sign. */
			if (pos + 1 < in_len && input[pos + 1] == '=') {
				stack.emplace_back(new BinaryLeq);
				pos += 2;
				continue;
			}
			stack.emplace_back(new BinaryLt);
			++pos;
			continue;
		}

		if (input[pos] == '>') {
			/* Either a > or a >= sign. */
			if (pos + 1 < in_len && input[pos + 1] == '=') {
				stack.emplace_back(new BinaryGeq);
				pos += 2;
				continue;
			}
			stack.emplace_back(new BinaryGt);
			++pos;
			continue;
		}

		if (input[pos] == '%') {
			/* Modulo operator. */
			stack.emplace_back(new BinaryMod);
			++pos;
			continue;
		}

		if (input[pos] == '*') {
			/* Multiplication operator. */
			stack.emplace_back(new BinaryMult);
			++pos;
			continue;
		}

		if (input[pos] == '/') {
			/* Division operator. */
			stack.emplace_back(new BinaryDiv);
			++pos;
			continue;
		}

		if (input[pos] == '+') {
			/* Addition operator. */
			stack.emplace_back(new BinaryPlus);
			++pos;
			continue;
		}

		if (input[pos] == '-') {
			/* Subtraction operator. */
			stack.emplace_back(new BinaryMinus);
			++pos;
			continue;
		}

		if (input[pos] == '?') {
			/* Ternary operator. */
			stack.emplace_back(new TernaryCondition);
			++pos;
			continue;
		}

		if (input[pos] == ':') {
			/* Ternary operator. */
			stack.emplace_back(new TernaryEvaluate);
			++pos;
			continue;
		}

		ERROR("Unrecognized token '%c'", input[pos]);
	}

	Reduce(stack);

	if (stack.empty()) ERROR("Empty expression");
	if (stack.size() > 1) ERROR("Multiple (%d) unconnected expressions", static_cast<int>(stack.size()));
	return std::move(stack.at(0));
}

}  // namespace EvaluateableExpressionImpl

/**
 * Parse the string representation of an expression to generate an object
 * whose value for various inputs can be evaluated programmatically.
 * @param input Expression string.
 * @return Evaluateable object.
 */
std::unique_ptr<EvaluateableExpression> ParseEvaluateableExpression(const std::string &input)
{
	return EvaluateableExpressionImpl::Parse(input);
}
