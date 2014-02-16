/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ast.h AST data structures. */

#ifndef AST_H
#define AST_H

#include <list>
#include <string>
#include <memory>

/** Position in a source file. */
class Position {
public:
	Position();
	Position(const char *filename, int line);
	Position(const std::string &filename, int line);
	Position(const Position &pos);
	Position &operator=(const Position &pos);

	const char *ToString() const;

	std::string filename; ///< File containing the line.
	int line;             ///< Number of the line referred to.
};

/** A Symbol in a 'symbol table'. */
struct Symbol {
	const char *name; ///< Name of the symbol.
	int value;        ///< Value of the symbol.
};

/** Base class of expressions. */
class Expression {
public:
	Expression(const Position &pos);
	virtual ~Expression();

	virtual std::shared_ptr<const Expression> Evaluate(const Symbol *symbols) const = 0;

	const Position pos; ///< %Position of the expression.
};

/** A sequence of expressions. */
class ExpressionList {
public:
	ExpressionList();

	std::list<std::shared_ptr<Expression>> exprs; ///< The sequence of expressions.
};

/** Unary operator expression. */
class UnaryOperator : public Expression {
public:
	UnaryOperator(const Position &pos, int oper, std::shared_ptr<Expression> child);

	std::shared_ptr<const Expression> Evaluate(const Symbol *symbols) const override;

	int oper; ///< Operation performed.
	std::shared_ptr<Expression> child; ///< Child expression (should be numeric).
};

/** Binary operator expression. */
class BinaryOperator : public Expression {
public:
	BinaryOperator(const Position &pos, std::shared_ptr<Expression> left, int oper, std::shared_ptr<Expression> right);

	std::shared_ptr<const Expression> Evaluate(const Symbol *symbols) const override;

	int oper; ///< Operation performed.
	std::shared_ptr<Expression> left;  ///< Left child expression (should be numeric).
	std::shared_ptr<Expression> right; ///< Right child expression (should be numeric).
};

/** String literal elementary expression node. */
class StringLiteral : public Expression {
public:
	StringLiteral(const Position &pos, const std::string &text);

	std::shared_ptr<const Expression> Evaluate(const Symbol *symbols) const override;

	const std::string text; ///< Text of the string literal (decoded).
};

/** Identifier elementary expression node. */
class IdentifierLiteral : public Expression {
public:
	IdentifierLiteral(const Position &pos, const std::string &name);

	std::shared_ptr<const Expression> Evaluate(const Symbol *symbols) const override;

	const std::string name; ///< The identifier of the expression.
};

/** Number literal elementary expression node. */
class NumberLiteral : public Expression {
public:
	NumberLiteral(const Position &pos, long long value);

	std::shared_ptr<const Expression> Evaluate(const Symbol *symbols) const override;

	long long value; ///< Value of the number literal.
};

/** Bit set expression ('or' of '1 << arg'). */
class BitSet : public Expression {
public:
	BitSet(const Position &pos, std::shared_ptr<ExpressionList> args);

	std::shared_ptr<const Expression> Evaluate(const Symbol *symbols) const override;

	std::shared_ptr<ExpressionList> args; ///< Arguments of the bitset, if any.
};

/** Base class for labels of named values. */
class Name {
public:
	Name();
	virtual ~Name();

	virtual const Position &GetPosition() const = 0;
	virtual int GetNameCount() const = 0;
};

/** Label of a named value containing a single name. */
class SingleName : public Name {
public:
	SingleName(const Position &pos, const std::string &name);

	const Position &GetPosition() const override;
	int GetNameCount() const override;

	const Position pos; ///< %Position of the label.
	const std::string name;   ///< The label itself.
};

/** Somewhat generic class for storing an identifier and its position. */
class IdentifierLine {
public:
	IdentifierLine(const Position &pos, const std::string &name);

	const Position &GetPosition() const;
	bool IsValid() const;

	Position pos;     ///< %Position of the label.
	std::string name; ///< The label itself.
};

/**
 * A row of identifiers.
 * Names here may be parameterized, and represent many identifiers.
 */
class NameRow {
public:
	NameRow();

	const Position &GetPosition() const;
	int GetNameCount() const;

	std::list<std::shared_ptr<IdentifierLine>> identifiers; ///< Identifiers in this row.
};

/**
 * A 2D table of identifiers.
 * Names here may be parameterized, and represent many identifiers.
 */
class NameTable : public Name {
public:
	NameTable();

	bool HasSingleElement() const;
	const Position &GetPosition() const override;
	int GetNameCount() const override;

	std::list<std::shared_ptr<NameRow>> rows; ///< Rows of the table.
};

/** Base class of the value part of a named value. */
class Group {
public:
	Group();
	virtual ~Group();

	virtual const Position &GetPosition() const = 0;
};

class NamedValueList;

/** Value part consisting of a node. */
class NodeGroup : public Group {
public:
	NodeGroup(const Position &pos, const std::string &name, std::shared_ptr<ExpressionList> exprs, std::shared_ptr<NamedValueList> values);

	const Position &GetPosition() const override;

	void HandleImports();

	const Position pos;     ///< %Position of the node name.
	const std::string name; ///< Node name itself.
	std::shared_ptr<ExpressionList> exprs;  ///< Parameters of the node.
	std::shared_ptr<NamedValueList> values; ///< Named values of the node.
};

/** Value part of a group consisting of an expression. */
class ExpressionGroup : public Group {
public:
	ExpressionGroup(std::shared_ptr<Expression> expr);

	const Position &GetPosition() const override;

	std::shared_ptr<Expression> expr; ///< %Expression to store.
};

/** Base class for named values. */
class BaseNamedValue {
public:
	BaseNamedValue();
	virtual ~BaseNamedValue();

	virtual void HandleImports() = 0;
};

/** A value with a name. */
class NamedValue : public BaseNamedValue {
public:
	NamedValue(std::shared_ptr<Name> name, std::shared_ptr<Group> group);

	void HandleImports() override;

	std::shared_ptr<Name> name;   ///< %Name part, may be \c NULL.
	std::shared_ptr<Group> group; ///< Value part.
};

/** Node to import another file. */
class ImportValue : public BaseNamedValue {
public:
	ImportValue(const Position &pos, const std::string &filename);

	void HandleImports() override;

	const Position pos;         ///< %Position of the import.
	const std::string filename; ///< Name of the file to import.
};

/** Sequence of named values. */
class NamedValueList {
public:
	NamedValueList();

	void HandleImports();

	std::list<std::shared_ptr<BaseNamedValue>> values; ///< Named values in the sequence.
};

std::shared_ptr<NamedValueList> LoadFile(const char *filename, int line = 0);

#endif
