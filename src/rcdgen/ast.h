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
#include "../reference_count.h"

/** Position in a source file. */
class Position {
public:
	Position();
	Position(const char *filename, int line);
	Position(const std::string &filename, int line);
	Position(const Position &pos);
	Position &operator=(const Position &pos);
	~Position();

	const char *ToString() const;

	std::string filename; ///< File containing the line.
	int line;             ///< Number of the line referred to.
};

/** A Symbol in a 'symbol table'. */
struct Symbol {
	const char *name; ///< Name of the symbol.
	int value;        ///< Value of the symbol.
};

class Expression;

/** Counted reference to an expression. */
typedef DataReference<Expression> ExpressionRef;

/** Base class of expressions. */
class Expression : public RefCounter {
public:
	Expression(const Position &pos);

	virtual ExpressionRef Evaluate(const Symbol *symbols) const = 0;

	const Position pos; ///< %Position of the expression.

protected:
	virtual ~Expression();
};

/** A sequence of expressions. */
class ExpressionList {
public:
	ExpressionList();
	~ExpressionList();

	std::list<ExpressionRef> exprs; ///< The sequence of expressions.
};

/** Unary operator expression. */
class UnaryOperator : public Expression {
public:
	UnaryOperator(const Position &pos, int oper, ExpressionRef &child);

	ExpressionRef Evaluate(const Symbol *symbols) const override;

	int oper; ///< Operation performed, currently only \c '-' (unary negation is supported).
	ExpressionRef child; ///< Child expression (should be numeric).

protected:
	~UnaryOperator();
};

/** String literal elementary expression node. */
class StringLiteral : public Expression {
public:
	StringLiteral(const Position &pos, const std::string &text);

	ExpressionRef Evaluate(const Symbol *symbols) const override;

	const std::string text; ///< Text of the string literal (decoded).

protected:
	~StringLiteral();
};

/** Identifier elementary expression node. */
class IdentifierLiteral : public Expression {
public:
	IdentifierLiteral(const Position &pos, const std::string &name);

	ExpressionRef Evaluate(const Symbol *symbols) const override;

	const std::string name; ///< The identifier of the expression.

protected:
	~IdentifierLiteral();
};

/** Number literal elementary expression node. */
class NumberLiteral : public Expression {
public:
	NumberLiteral(const Position &pos, long long value);

	ExpressionRef Evaluate(const Symbol *symbols) const override;

	long long value; ///< Value of the number literal.

protected:
	~NumberLiteral();
};

/** Bit set expression ('or' of '1 << arg'). */
class BitSet : public Expression {
public:
	BitSet(const Position &pos, ExpressionList *args);

	ExpressionRef Evaluate(const Symbol *symbols) const override;

	ExpressionList *args; ///< Arguments of the bitset, if any.

protected:
	~BitSet();
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
	SingleName(const Position &pos, char *name);
	~SingleName();

	const Position &GetPosition() const override;
	int GetNameCount() const override;

	const Position pos; ///< %Position of the label.
	const std::string name;   ///< The label itself.
};

/** Somewhat generic class for storing an identifier and its position. */
class IdentifierLine {
public:
	IdentifierLine(const Position &pos, char *name);
	IdentifierLine(const IdentifierLine &il);
	IdentifierLine &operator=(const IdentifierLine &il);
	~IdentifierLine();

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
	~NameRow();

	const Position &GetPosition() const;
	int GetNameCount() const;

	std::list<IdentifierLine *> identifiers; ///< Identifiers in this row.
};

/**
 * A 2D table of identifiers.
 * Names here may be parameterized, and represent many identifiers.
 */
class NameTable : public Name {
public:
	NameTable();
	~NameTable();

	bool HasSingleElement() const;
	const Position &GetPosition() const override;
	int GetNameCount() const override;

	std::list<NameRow *> rows; ///< Rows of the table.
};

class NamedValueList;
class NodeGroup;
class ExpressionGroup;

/** Base class of the value part of a named value. */
class Group {
public:
	Group();
	virtual ~Group();

	virtual const Position &GetPosition() const = 0;
	virtual NodeGroup *CastToNodeGroup();
	virtual ExpressionGroup *CastToExpressionGroup();
};

/** Value part consisting of a node. */
class NodeGroup : public Group {
public:
	NodeGroup(const Position &pos, char *name, ExpressionList *exprs, NamedValueList *values);
	~NodeGroup();

	const Position &GetPosition() const override;
	NodeGroup *CastToNodeGroup() override;

	void HandleImports();

	const Position pos;     ///< %Position of the node name.
	const std::string name; ///< Node name itself.
	ExpressionList *exprs;  ///< Parameters of the node.
	NamedValueList *values; ///< Named values of the node.
};

/** Value part of a group consisting of an expression. */
class ExpressionGroup : public Group {
public:
	ExpressionGroup(ExpressionRef &expr);
	~ExpressionGroup();

	const Position &GetPosition() const override;
	ExpressionGroup *CastToExpressionGroup() override;

	ExpressionRef expr; ///< %Expression to store.
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
	NamedValue(Name *name, Group *group);
	~NamedValue();

	void HandleImports() override;

	Name *name;   ///< %Name part, may be \c nullptr.
	Group *group; ///< Value part.
};

/** Node to import another file. */
class ImportValue : public BaseNamedValue {
public:
	ImportValue(const Position &pos, char *filename);
	~ImportValue();

	void HandleImports() override;

	const Position pos;         ///< %Position of the import.
	const std::string filename; ///< Name of the file to import.
};

/** Sequence of named values. */
class NamedValueList {
public:
	NamedValueList();
	~NamedValueList();

	void HandleImports();

	std::list<BaseNamedValue *> values; ///< Named values in the sequence.
};

NamedValueList *LoadFile(const char *filename, int line = 0);

#endif
