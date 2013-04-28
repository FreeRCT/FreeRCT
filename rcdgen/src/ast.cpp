/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ast.cpp AST data structure implementation. */

#include "stdafx.h"
#include "ast.h"
#include "scanner_funcs.h"

Position::Position()
{
	this->filename = "unknown";
	this->line = 0;
}

/**
 * Constructor of a position.
 * @param filename Name of the file containing the position.
 * @param line Line number in the file.
 */
Position::Position(const char *filename, int line)
{
	this->filename = filename;
	this->line = line;
}

/**
 * Constructor of a position.
 * @param filename Name of the file containing the position.
 * @param line Line number in the file.
 */
Position::Position(std::string &filename, int line)
{
	this->filename = filename;
	this->line = line;
}

/**
 * Constructor of a position.
 * @param pos %Position to copy.
 */
Position::Position(const Position &pos)
{
	this->filename = pos.filename;
	this->line = pos.line;
}

/**
 * Assignment of a position.
 * @param pos %Position to copy.
 * @return The assigned object.
 */
Position &Position::operator=(const Position &pos)
{
	if (&pos != this) {
		this->filename = pos.filename;
		this->line = pos.line;
	}
	return *this;
}

Position::~Position()
{
}

/**
 * Construct a human-readable position string.
 * @return Human-readable indication of a position (filename and line number).
 */
const char *Position::ToString() const
{
	static char buffer[256];
	snprintf(buffer, 255, "\"%s\" line %d", this->filename.c_str(), this->line);
	return buffer;
}

ExpressionList::ExpressionList()
{
}

ExpressionList::~ExpressionList()
{
	for (std::list<Expression *>::iterator iter = this->exprs.begin(); iter != this->exprs.end(); iter++) {
		delete (*iter);
	}
}

/**
 * Constructor of the base expression class.
 * @param pos %Position of the expression node.
 */
Expression::Expression(const Position &pos) : pos(pos)
{
}

Expression::~Expression()
{
}

/**
 * \fn  Expression *Expression::Evaluate(const Symbol *symbols) const
 * Evaluation of the expression. Reduces it to its value or throws a fatal error.
 * @param symbols Sequence of known identifier names.
 * @return The computed reduced expression.
 */

/**
 * Unary expression.
 * @param pos %Position of the operator.
 * @param oper Unary operator. Only \c '-' is supported currently.
 * @param child Sub-exoression.
 */
UnaryOperator::UnaryOperator(const Position &pos, int oper, Expression *child) : Expression(pos)
{
	this->oper = oper;
	this->child = child;
}

UnaryOperator::~UnaryOperator()
{
	delete child;
}

Expression *UnaryOperator::Evaluate(const Symbol *symbols) const
{
	Expression *result = child->Evaluate(symbols);
	NumberLiteral *number = dynamic_cast<NumberLiteral *>(result);
	if (number != NULL) {
		number->value = -number->value;
		return number;
	}
	fprintf(stderr, "Evaluate error at %s: Cannot negate the value of the child expression", this->pos.ToString());
	exit(1);
}

/**
 * A string literal as elementary expression.
 * @param pos %Position of the string literal.
 * @param text String literal content itself.
 */
StringLiteral::StringLiteral(const Position &pos, char *text) : Expression(pos)
{
	this->text = text;
}

StringLiteral::~StringLiteral()
{
	free(this->text);
}

Expression *StringLiteral::Evaluate(const Symbol *symbols) const
{
	char *copy = this->CopyText();
	return new StringLiteral(this->pos, copy);
}

/**
 * Return a copy of the text of the string literal.
 * @return A copy of the string value.
 */
char *StringLiteral::CopyText() const
{
	int length = strlen(this->text);
	char *copy = (char *)malloc(length + 1);
	memcpy(copy, this->text, length + 1);
	return copy;
}

/**
 * An identifier as elementary expression.
 * @param pos %Position of the identifier.
 * @param name The identifier to store.
 */
IdentifierLiteral::IdentifierLiteral(const Position &pos, char *name) : Expression(pos)
{
	this->name = name;
}

IdentifierLiteral::~IdentifierLiteral()
{
	free(this->name);
}

Expression *IdentifierLiteral::Evaluate(const Symbol *symbols) const
{
	if (symbols != NULL) {
		for (;;) {
			if (symbols->name == NULL) break;
			if (strcmp(symbols->name, this->name) == 0) return new NumberLiteral(this->pos, symbols->value);
			symbols++;
		}
	}
	fprintf(stderr, "Evaluate error %s: Identifier \"%s\" is not known\n", this->pos.ToString(), this->name);
	exit(1);
}

/**
 * A literal number as elementary expression.
 * @param pos %Position of the value.
 * @param value The number itself.
 */
NumberLiteral::NumberLiteral(const Position &pos, long long value) : Expression(pos)
{
	this->value = value;
}

NumberLiteral::~NumberLiteral()
{
}

Expression *NumberLiteral::Evaluate(const Symbol *symbols) const
{
	return new NumberLiteral(this->pos, this->value);
}

/**
 * Constructor of a bitset expression node.
 * @param pos %Position that uses the 'bitset' node.
 * @param args Arguments of the bitsset node, may be \c NULL.
 */
BitSet::BitSet(const Position &pos, ExpressionList *args) : Expression(pos)
{
	this->args = args;
}

BitSet::~BitSet()
{
	delete this->args;
}

Expression *BitSet::Evaluate(const Symbol *symbols) const
{
	long long value = 0;
	if (this->args != NULL) {
		for (std::list<Expression *>::const_iterator iter = this->args->exprs.begin(); iter != this->args->exprs.end(); iter++) {
			Expression *e = (*iter)->Evaluate(symbols);
			NumberLiteral *nl = dynamic_cast<NumberLiteral *>(e);
			if (nl == NULL) {
				fprintf(stderr, "Error at %s: Bit set argument is not an number\n", (*iter)->pos.ToString());
				exit(1);
			}
			value |= 1ll << nl->value;
			delete e;
		}
	}
	return new NumberLiteral(this->pos, value);
}


Name::Name()
{
}

Name::~Name() {
}

/**
 * \fn Name::GetPosition() const
 * Get a position representing the name (group).
 * @return %Position pointing to the name part.
 */

/**
 * \fn Name::GetNameCount() const
 * Get the number of names attached to the 'name' part.
 * @return The number of names in this #Name.
 */

/**
 * A name for a group consisting of a single label.
 * @param pos %Position of the label name.
 * @param name The label name itself.
 */
SingleName::SingleName(const Position &pos, char *name) : Name(), pos(pos)
{
	this->name = name;
}

SingleName::~SingleName()
{
	free(this->name);
}

const Position &SingleName::GetPosition() const
{
	return this->pos;
}

int SingleName::GetNameCount() const
{
	return 1;
}

/**
 * An identifier with a line number.
 * @param pos %Position of the name.
 * @param name The identifier to store.
 */
IdentifierLine::IdentifierLine(const Position &pos, char *name) : pos(pos)
{
	this->name = name;
}

/**
 * Copy constructor.
 * @param il Existing identifier line to copy.
 */
IdentifierLine::IdentifierLine(const IdentifierLine &il) : pos(il.pos)
{
	int length = strlen(il.name);
	this->name = (char *)malloc(length + 1);
	memcpy(this->name, il.name, length + 1);
}

/**
 * Assignment operator of an identifier line.
 * @param il Identifier line being copied.
 * @return The identifier line copied to.
 */
IdentifierLine &IdentifierLine::operator=(const IdentifierLine &il)
{
	if (&il == this) return *this;
	free(this->name);

	this->pos = il.pos;
	int length = strlen(il.name);
	this->name = (char *)malloc(length + 1);
	memcpy(this->name, il.name, length + 1);
	return *this;
}

IdentifierLine::~IdentifierLine()
{
	free(this->name);
}

/**
 * Retrieve a position for this identifier.
 * @return The position.
 */
const Position &IdentifierLine::GetPosition() const
{
	return this->pos;
}

/**
 * Is it a valid identifier to use?
 * @return Whether the name is valid for use.
 */
bool IdentifierLine::IsValid() const
{
	if (this->name == NULL) return false;
	if (this->name[0] == '\0' || this->name[0] == '_') return false;
	return true;
}

NameRow::NameRow()
{
}

NameRow::~NameRow()
{
	for (std::list<IdentifierLine *>::iterator iter = this->identifiers.begin(); iter != this->identifiers.end(); iter++) {
		delete *iter;
	}
}

static const Position _dummy_position("", -1); ///< Dummy position.

/**
 * Get a line number of the row, or \c 0 if none is available.
 * @return %Position of the row.
 */
const Position &NameRow::GetPosition() const
{
	if (this->identifiers.size() > 0) return this->identifiers.front()->GetPosition();
	return _dummy_position;
}

/**
 * Get the number of valid names in this row.
 * @return Number of valid names.
 */
int NameRow::GetNameCount() const
{
	int count = 0;
	for (std::list<IdentifierLine *>::const_iterator iter = this->identifiers.begin(); iter != this->identifiers.end(); iter++) {
		if ((*iter)->IsValid()) count++;
	}
	return count;
}

NameTable::NameTable() : Name()
{
}

NameTable::~NameTable()
{
	for (std::list<NameRow *>::iterator iter = this->rows.begin(); iter != this->rows.end(); iter++) {
		delete *iter;
	}
}

const Position &NameTable::GetPosition() const
{
	for (std::list<NameRow *>::const_iterator iter = this->rows.begin(); iter != this->rows.end(); iter++) {
		const Position &pos = (*iter)->GetPosition();
		if (pos.line > 0) return pos;
	}
	return _dummy_position;
}

int NameTable::GetNameCount() const
{
	int count = 0;
	for (std::list<NameRow *>::const_iterator iter = this->rows.begin(); iter != this->rows.end(); iter++) {
		count += (*iter)->GetNameCount();
	}
	return count;
}

Group::Group()
{
}

Group::~Group()
{
}

/**
 * \fn Group::GetPosition() const
 * Get a position representing the name (group).
 * @return %Position pointing to the name part.
 */

/**
 * Cast the group to a #NodeGroup.
 * @return a node group if the cast succeeded, else \c NULL.
 */
/* virtual */ NodeGroup *Group::CastToNodeGroup()
{
	return NULL;
}

/**
 * Cast the group to a #ExpressionGroup.
 * @return an expression group if the cast succeeded, else \c NULL.
 */
/* virtual */ ExpressionGroup *Group::CastToExpressionGroup()
{
	return NULL;
}

/**
 * Construct a node.
 * @param pos %Position of the label name.
 * @param name The label name itself.
 * @param exprs Actual parameters of the node.
 * @param values Named values of the node.
 */
NodeGroup::NodeGroup(const Position &pos, char *name, ExpressionList *exprs, NamedValueList *values) : Group(), pos(pos)
{
	this->name = name;
	this->exprs = exprs;
	this->values = values;
}

NodeGroup::~NodeGroup()
{
	free(this->name);
	delete this->exprs;
	delete this->values;
}

/* virtual */ const Position &NodeGroup::GetPosition() const
{
	return this->pos;
}

/* virtual */ NodeGroup *NodeGroup::CastToNodeGroup()
{
	return this;
}

/** Handle imports in the body. */
void NodeGroup::HandleImports()
{
	this->values->HandleImports();
}

/**
 * Wrap an expression in a group.
 * @param expr %Expression to wrap.
 */
ExpressionGroup::ExpressionGroup(Expression *expr) : Group()
{
	this->expr = expr;
}

ExpressionGroup::~ExpressionGroup()
{
	delete this->expr;
}

/* virtual */ const Position &ExpressionGroup::GetPosition() const
{
	return this->expr->pos;
}

/* virtual */ ExpressionGroup *ExpressionGroup::CastToExpressionGroup()
{
	return this;
}

BaseNamedValue::BaseNamedValue()
{
}

BaseNamedValue::~BaseNamedValue()
{
}

/**
 * \fn  void BaseNamedValue::HandleImports()
 * Perform the import operation.
 */

/**
 * Construct a value with a name.
 * @param name (may be \c NULL).
 * @param group %Group value.
 */
NamedValue::NamedValue(Name *name, Group *group) : BaseNamedValue()
{
	this->name = name;
	this->group = group;
}

NamedValue::~NamedValue()
{
	delete this->name;
	delete this->group;
}

void NamedValue::HandleImports()
{
	NodeGroup *ng = this->group->CastToNodeGroup();
	if (ng != NULL) ng->HandleImports();
}

/**
 * Constructor of the import class.
 * @param pos %Position of the import.
 * @param filename File name being imported.
 */
ImportValue::ImportValue(const Position &pos, char *filename) : BaseNamedValue(), pos(pos)
{
	this->filename = filename;
}

ImportValue::~ImportValue()
{
	free(this->filename);
}

void ImportValue::HandleImports()
{
	/* Do nothing, the surrounding NamedValueList handles this import. */
}

NamedValueList::NamedValueList()
{
}

NamedValueList::~NamedValueList()
{
	for (std::list<BaseNamedValue *>::iterator iter = this->values.begin(); iter != this->values.end(); iter++) {
		delete (*iter);
	}
}

/** Handle imports in the body. */
void NamedValueList::HandleImports()
{
	bool has_import = false;
	std::list<BaseNamedValue *> values;

	for (std::list<BaseNamedValue *>::iterator iter = this->values.begin(); iter != this->values.end(); iter++) {
		ImportValue *iv = dynamic_cast<ImportValue *>(*iter);
		if (iv != NULL) {
			has_import = true;
			NamedValueList *nv = LoadFile(iv->filename, iv->pos.line);
			for (std::list<BaseNamedValue *>::iterator iter2 = nv->values.begin(); iter2 != nv->values.end(); iter2++) {
				values.push_back(*iter2);
			}
			nv->values.clear();
			delete nv;
			delete *iter; // Is not copied into 'values' and will get lost below.
		} else {
			(*iter)->HandleImports();
			values.push_back(*iter);
		}
	}
	if (has_import) this->values = values;
}


/**
 * Load a file, and parse the contents.
 * @param filename Name of the file to load. \c NULL means to read \c stdin.
 * @param line Line number of the current file.
 * @return The parsed node tree.
 */
NamedValueList *LoadFile(const char *filename, int line)
{
	static int nest_level = 0;
	static const char *include_cache[10];
	static int line_number_cache[10];

	/* Check for too many nested include levels. */
	if (nest_level > 0) line_number_cache[nest_level - 1] = line;

	if (nest_level >= (int)lengthof(include_cache)) {
		fprintf(stderr, "Error: Too many nested file imports\n");
		fprintf(stderr, "       While importing \"%s\"\n", filename);
		for (int i = nest_level; i >= 0; i--) {
			fprintf(stderr, "       from \"%s\" at line %d\n", include_cache[i], line_number_cache[i]);
		}
		exit(1);
	}

	include_cache[nest_level] = filename;
	nest_level++;

	/* Parse the input. */
	FILE *infile = NULL;
	if (filename != NULL) {
		infile = fopen(filename, "rb");
		if (infile == NULL) {
			fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
			exit(1);
		}
	}
	_parsed_data = NULL;
	SetupScanner(filename, infile);
	yyparse();

	if (infile != NULL) fclose(infile);

	if (_parsed_data == NULL) {
		fprintf(stderr, "Parsing of the input file did not give a result\n");
		exit(1);
	}

	/* Process imports. */
	NamedValueList *nvs = _parsed_data;
	nvs->HandleImports(); // Recursively calls this function, so _parsed_data is not safe.

	/* Restore to pre-call state. */
	include_cache[nest_level] = NULL;
	nest_level--;
	return nvs;
}
