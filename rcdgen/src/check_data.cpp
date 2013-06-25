/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file check_data.cpp Check and simplify functions. */

#include "../../src/stdafx.h"
#include <map>
#include <string>
#include "ast.h"
#include "nodes.h"
#include "string_names.h"

static BlockNode *ConvertNodeGroup(NodeGroup *ng);

/**
 * Check the number of expressions given in \a exprs, and expand them into the \a out array for easier access.
 * @param exprs %Expression list containing parameters.
 * @param out [out] Output array for storing \a expected expressions from the list.
 * @param expected Expected number of expressions in \a exprs.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node being checked and expanded.
 */
static void ExpandExpressions(ExpressionList *exprs, ExpressionRef out[], size_t expected, const Position &pos, const char *node)
{
	if (exprs == NULL) {
		if (expected == 0) return;
		fprintf(stderr, "Error at %s: No arguments found for node \"%s\" (expected %lu)\n", pos.ToString(), node, expected);
		exit(1);
	}
	if (exprs->exprs.size() != expected) {
		fprintf(stderr, "Error at %s: Found %lu arguments for node \"%s\", expected %lu\n", pos.ToString(), exprs->exprs.size(), node, expected);
		exit(1);
	}
	int idx = 0;
	for (std::list<ExpressionRef>::iterator iter = exprs->exprs.begin(); iter != exprs->exprs.end(); iter++) {
		out[idx++] = *iter;
	}
}

/**
 * Check that there are no expressions provided in \a exprs. Give an error otherwise.
 * @param exprs %Expression list containing parameters.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node being checked and expanded.
 */
static void ExpandNoExpression(ExpressionList *exprs, const Position &pos, const char *node)
{
	if (exprs == NULL || exprs->exprs.size() == 0) return;

	fprintf(stderr, "Error at %s: No arguments expected for node \"%s\" (found %lu)\n", pos.ToString(), node, exprs->exprs.size());
	exit(1);
}

/**
 * Extract a string from the given expression.
 * @param expr %Expression to evaluate.
 * @param index Parameter number (0-based, for error reporting).
 * @param node %Name of the node.
 * @return Value of the string (caller should release the memory after use).
 */
static char *GetString(ExpressionRef &expr, int index, const char *node)
{
	/* Simple case, expression is a string literal. */
	StringLiteral *sl = dynamic_cast<StringLiteral *>(expr.Access());
	if (sl != NULL) return sl->CopyText();

	/* General case, compute its value. */
	ExpressionRef expr2 = expr.Access()->Evaluate(NULL);
	sl = dynamic_cast<StringLiteral *>(expr2.Access());
	if (sl == NULL) {
		fprintf(stderr, "Error at %s: Expression parameter %d of node %s is not a string", expr.Access()->pos.ToString(), index + 1, node);
		exit(1);
	}
	char *result = sl->CopyText();
	return result;
}

// No  foo(1234) { ... } nodes exist yet, so number parsing is not needed currently.
// /**
//  * Extract a number from the given expression.
//  * @param expr %Expression to evaluate.
//  * @param index Parameter number (0-based, for error reporting).
//  * @param node %Name of the node.
//  * @param symbols Symbols available for use in the expression.
//  * @return Value of the number.
//  */
// static long long GetNumber(Expression *expr, int index, const char *node, const Symbol *symbols = NULL)
// {
// 	/* Simple case, expression is a number literal. */
// 	NumberLiteral *nl = dynamic_cast<NumberLiteral *>(expr);
// 	if (nl != NULL) return nl->value;
// 
// 	/* General case, compute its value. */
// 	Expression *expr2 = expr->Evaluate(symbols);
// 	nl = dynamic_cast<NumberLiteral *>(expr2);
// 	if (nl == NULL) {
// 		fprintf(stderr, "Error at %s: Expression parameter %d of node %s is not a number", expr->pos.ToString(), index + 1, node);
// 		exit(1);
// 	}
// 	long long value = nl->value;
// 	delete expr2;
// 	return value;
// }

/**
 * Convert a 'file' node (taking a string parameter for the filename, and a sequence of game blocks).
 * @param ng Generic tree of nodes to convert to a 'file' node.
 * @return The converted file node.
 */
static FileNode *ConvertFileNode(NodeGroup *ng)
{
	ExpressionRef argument;
	ExpandExpressions(ng->exprs, &argument, 1, ng->pos, "file");

	char *filename = GetString(argument, 0, "file");
	FileNode *fn = new FileNode(filename);

	for (std::list<BaseNamedValue *>::iterator iter = ng->values->values.begin(); iter != ng->values->values.end(); iter++) {
		NamedValue *nv = dynamic_cast<NamedValue *>(*iter);
		assert(nv != NULL); // Should always hold, as ImportValue has been eliminated.
		if (nv->name != NULL) fprintf(stderr, "Warning at %s: Unexpected name encountered, ignoring\n", nv->name->GetPosition().ToString());
		NodeGroup *ng = nv->group->CastToNodeGroup();
		if (ng == NULL) {
			fprintf(stderr, "Error at %s: Only node groups may be added\n", nv->group->GetPosition().ToString());
			exit(1);
		}
		BlockNode *bn = ConvertNodeGroup(ng);
		GameBlock *gb = dynamic_cast<GameBlock *>(bn);
		if (gb == NULL) {
			fprintf(stderr, "Error at %s: Only game blocks can be added to a \"file\" node\n", nv->group->GetPosition().ToString());
			exit(1);
		}
		fn->blocks.push_back(gb);
	}
	return fn;
}

/** All information that needs to be stored about a named value. */
class ValueInformation {
public:
	ValueInformation();
	ValueInformation(const std::string &name, const Position &pos);
	ValueInformation(const ValueInformation &vi);
	ValueInformation &operator=(const ValueInformation &vi);
	~ValueInformation();

	long long GetNumber(const Position &pos, const char *node, const Symbol *symbols = NULL);
	std::string GetString(const Position &pos, const char *node);
	SpriteBlock *GetSprite(const Position &pos, const char *node);
	Connection *GetConnection(const Position &pos, const char *node);
	Strings *GetStrings(const Position &pos, const char *node);

	Position pos;             ///< %Position of the name.
	ExpressionRef expr_value; ///< %Expression attached to it (if any).
	BlockNode *node_value;    ///< Node attached to it (if any).
	std::string name;         ///< %Name of the value.
	bool used;                ///< Is the value used?
};

/** Default constructor. */
ValueInformation::ValueInformation() : pos("", 0)
{
	this->expr_value.Give(NULL);
	this->node_value = NULL;
	this->name = "_unknown_";
	this->used = false;
}

/**
 * Constructor of the class.
 * @param name %Name of the value.
 * @param pos %Position of the name.
 */
ValueInformation::ValueInformation(const std::string &name, const Position &pos) : pos(pos)
{
	this->expr_value.Give(NULL);
	this->node_value = NULL;
	this->name = name;
	this->used = false;
}

/**
 * Copy-constructor, moves ownership of #node_value to the assigned object.
 * @param vi Original object.
 */
ValueInformation::ValueInformation(const ValueInformation &vi) : pos(vi.pos)
{
	ValueInformation &w_vi = const_cast<ValueInformation &>(vi);
	this->expr_value = w_vi.expr_value;
	this->node_value = w_vi.node_value;
	w_vi.node_value = NULL;

	this->name = vi.name;
	this->used = vi.used;
}

/**
 * Assignment operator, moves ownership of #node_value to the assigned object.
 * @param vi Original object.
 * @return Assigned object.
 */
ValueInformation &ValueInformation::operator=(const ValueInformation &vi)
{
	if (&vi != this) {
		ValueInformation &w_vi = const_cast<ValueInformation &>(vi);
		this->expr_value = w_vi.expr_value;
		this->node_value = w_vi.node_value;
		w_vi.node_value = NULL;

		this->name = vi.name;
		this->pos  = vi.pos;
		this->used = vi.used;
	}
	return *this;
}

ValueInformation::~ValueInformation()
{
	delete this->node_value;
}

/**
 * Extract a number from the given expression.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node.
 * @param symbols Symbols available for use in the expression.
 * @return Numeric value.
 */
long long ValueInformation::GetNumber(const Position &pos, const char *node, const Symbol *symbols)
{
	if (this->expr_value.Access() == NULL) {
fail:
		fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a numeric value\n", pos.ToString(), this->name.c_str(), node);
		exit(1);
	}
	NumberLiteral *nl = dynamic_cast<NumberLiteral *>(this->expr_value.Access()); // Simple common case.
	if (nl != NULL) return nl->value;

	ExpressionRef expr2 = this->expr_value.Access()->Evaluate(symbols); // Generic case, evaluate the expression.
	nl = dynamic_cast<NumberLiteral *>(expr2.Access());
	if (nl == NULL) goto fail;

	return nl->value;
}

/**
 * Extract a string from the given expression.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node.
 * @return String value, should be freed by the user.
 */
std::string ValueInformation::GetString(const Position &pos, const char *node)
{
	if (this->expr_value.Access() == NULL) {
fail:
		fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a string value\n", pos.ToString(), this->name.c_str(), node);
		exit(1);
	}
	StringLiteral *sl = dynamic_cast<StringLiteral *>(this->expr_value.Access()); // Simple common case.
	if (sl != NULL) return std::string(sl->text);

	ExpressionRef expr2 = this->expr_value.Access()->Evaluate(NULL); // Generic case
	sl = dynamic_cast<StringLiteral *>(expr2.Access());
	if (sl == NULL) goto fail;

	return std::string(sl->text);
}

/**
 * Get a sprite (#SpriteBlock) from the given node value.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node.
 * @return The sprite.
 */
SpriteBlock *ValueInformation::GetSprite(const Position &pos, const char *node)
{
	SpriteBlock *sb = dynamic_cast<SpriteBlock *>(this->node_value);
	if (sb != NULL) {
		this->node_value = NULL;
		return sb;
	}
	fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a sprite node\n", pos.ToString(), this->name.c_str(), node);
	exit(1);
}

/**
 * Get a connection from the given node value.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node.
 * @return The connection.
 */
Connection *ValueInformation::GetConnection(const Position &pos, const char *node)
{
	Connection *cn = dynamic_cast<Connection *>(this->node_value);
	if (cn != NULL) {
		this->node_value = NULL;
		return cn;
	}
	fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a connection node\n", pos.ToString(), this->name.c_str(), node);
	exit(1);
}

/**
 * Get a set of strings from the given node value.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node.
 * @return The set of strings.
 */
Strings *ValueInformation::GetStrings(const Position &pos, const char *node)
{
	Strings *st = dynamic_cast<Strings *>(this->node_value);
	if (st != NULL) {
		this->node_value = NULL;
		return st;
	}
	fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a strings node\n", pos.ToString(), this->name.c_str(), node);
	exit(1);
}

/**
 * Assign sub-nodes to the names of a 2D table.
 * @param bn Node to split in sub-nodes.
 * @param nt 2D name table.
 * @param vis Array being filled.
 * @param length [inout] Updated number of used entries in \a vis.
 */
void AssignNames(BlockNode *bn, NameTable *nt, ValueInformation *vis, int *length)
{
	int row = 0;
	for (std::list<NameRow *>::iterator row_iter = nt->rows.begin(); row_iter != nt->rows.end(); row_iter++) {
		NameRow *nr = *row_iter;
		int col = 0;
		for (std::list<IdentifierLine *>::iterator col_iter = nr->identifiers.begin(); col_iter != nr->identifiers.end(); col_iter++) {
			IdentifierLine *il = *col_iter;
			if (il->IsValid()) {
				vis[*length].expr_value = NULL;
				vis[*length].node_value = bn->GetSubNode(row, col, il->name, il->pos);
				vis[*length].name = std::string(il->name);
				vis[*length].pos = il->pos;
				vis[*length].used = false;
				(*length)++;
			}
			col++;
		}
		row++;
	}
}

/** Class for storing found named values. */
class Values {
public:
	Values(const char *node_name, const Position &pos);
	~Values();

	const Position pos;    ///< %Position of the node.
	const char *node_name; ///< Name of the node using the values (memory is not released).

	void PrepareNamedValues(NamedValueList *values, bool allow_named, bool allow_unnamed, const Symbol *symbols = NULL);
	ValueInformation &FindValue(const char *fld_name);
	bool HasValue(const char *fld_name);
	long long GetNumber(const char *fld_name, const Symbol *symbols = NULL);
	std::string GetString(const char *fld_name);
	SpriteBlock *GetSprite(const char *fld_name);
	Connection *GetConnection(const char *fld_name);
	Strings *GetStrings(const char *fld_name);
	void VerifyUsage();

	int named_count;   ///< Number of found values with a name.
	int unnamed_count; ///< Number of found value without a name.

	ValueInformation *named_values;   ///< Information about each named value.
	ValueInformation *unnamed_values; ///< Information about each unnamed value.

private:
	void CreateValues(int named_count, int unnamed_count);
};

/**
 * Constructor of the values collection.
 * @param node_name Name of the node using the values (caller owns the memory).
 * @param pos %Position of the node.
 */
Values::Values(const char *node_name, const Position &pos) : pos(pos)
{
	this->node_name = node_name;

	this->named_count = 0;
	this->unnamed_count = 0;
	this->named_values = NULL;
	this->unnamed_values = NULL;
}

/**
 * Create space for the information of of the values.
 * @param named_count Number of named values.
 * @param unnamed_count Number of unnamed valued.
 */
void Values::CreateValues(int named_count, int unnamed_count)
{
	delete[] this->named_values;
	delete[] this->unnamed_values;

	this->named_count = named_count;
	this->unnamed_count = unnamed_count;

	this->named_values = (named_count > 0) ? new ValueInformation[named_count] : NULL;
	this->unnamed_values = (unnamed_count > 0) ? new ValueInformation[unnamed_count] : NULL;
}

Values::~Values()
{
	delete[] this->named_values;
	delete[] this->unnamed_values;
}

/**
 * Prepare the named values for access by field name.
 * @param values Named value to prepare.
 * @param allow_named Allow named values.
 * @param allow_unnamed Allow unnamed values.
 * @param symbols Optional symbols that may be used in the values.
 */
void Values::PrepareNamedValues(NamedValueList *values, bool allow_named, bool allow_unnamed, const Symbol *symbols)
{
	/* Count number of named and unnamed values. */
	int named_count = 0;
	int unnamed_count = 0;
	for (std::list<BaseNamedValue *>::iterator iter = values->values.begin(); iter != values->values.end(); iter++) {
		NamedValue *nv = dynamic_cast<NamedValue *>(*iter);
		assert(nv != NULL); // Should always hold, as ImportValue has been eliminated.
		if (nv->name == NULL) { // Unnamed value.
			if (!allow_unnamed) {
				fprintf(stderr, "Error at %s: Value should have a name\n", nv->group->GetPosition().ToString());
				exit(1);
			}
			unnamed_count++;
		} else {
			if (!allow_named) {
				fprintf(stderr, "Error at %s: Value should not have a name\n", nv->group->GetPosition().ToString());
				exit(1);
			}
			int count = nv->name->GetNameCount();
			named_count += count;
		}
	}

	this->CreateValues(named_count, unnamed_count);

	named_count = 0;
	unnamed_count = 0;
	for (std::list<BaseNamedValue *>::iterator iter = values->values.begin(); iter != values->values.end(); iter++) {
		NamedValue *nv = dynamic_cast<NamedValue *>(*iter);
		assert(nv != NULL); // Should always hold, as ImportValue has been eliminated.
		if (nv->name == NULL) { // Unnamed value.
			NodeGroup *ng = nv->group->CastToNodeGroup();
			if (ng != NULL) {
				this->unnamed_values[unnamed_count].expr_value.Give(NULL);
				this->unnamed_values[unnamed_count].node_value = ConvertNodeGroup(ng);
				this->unnamed_values[unnamed_count].name = "???";
				this->unnamed_values[unnamed_count].pos = ng->GetPosition();
				this->unnamed_values[unnamed_count].used = false;
				unnamed_count++;
				continue;
			}
			ExpressionGroup *eg = nv->group->CastToExpressionGroup();
			assert(eg != NULL);
			this->unnamed_values[unnamed_count].expr_value = eg->expr.Access()->Evaluate(symbols);
			this->unnamed_values[unnamed_count].node_value = NULL;
			this->unnamed_values[unnamed_count].name = "???";
			this->unnamed_values[unnamed_count].pos = ng->GetPosition();
			this->unnamed_values[unnamed_count].used = false;
			unnamed_count++;
			continue;
		} else { // Named value.
			NodeGroup *ng = nv->group->CastToNodeGroup();
			if (ng != NULL) {
				BlockNode *bn = ConvertNodeGroup(ng);
				SingleName *sn = dynamic_cast<SingleName *>(nv->name);
				if (sn != NULL) {
					this->named_values[named_count].expr_value.Give(NULL);
					this->named_values[named_count].node_value = bn;
					this->named_values[named_count].name = std::string(sn->name);
					this->named_values[named_count].pos = sn->pos;
					this->named_values[named_count].used = false;
					named_count++;
					continue;
				}
				NameTable *nt = dynamic_cast<NameTable *>(nv->name);
				assert(nt != NULL);
				AssignNames(bn, nt, this->named_values, &named_count);
				continue;
			}

			/* Expression group. */
			ExpressionGroup *eg = nv->group->CastToExpressionGroup();
			assert(eg != NULL);
			SingleName *sn = dynamic_cast<SingleName *>(nv->name);
			if (sn == NULL) {
				fprintf(stderr, "Error at %s: Expression must have a single name\n", nv->name->GetPosition().ToString());
				exit(1);
			}
			this->named_values[named_count].expr_value = eg->expr.Access()->Evaluate(symbols);
			this->named_values[named_count].node_value = NULL;
			this->named_values[named_count].name = std::string(sn->name);
			this->named_values[named_count].pos = sn->pos;
			this->named_values[named_count].used = false;
			named_count++;
			continue;
		}
	}
	assert(named_count == this->named_count);
	assert(unnamed_count == this->unnamed_count);
}

/**
 * Find the value information named \a fld_name.
 * @param fld_name %Name of the field looking for.
 * @return Reference in the \a vis array.
 */
ValueInformation &Values::FindValue(const char *fld_name)
{
	for (int i = 0; i < this->named_count; i++) {
		ValueInformation &vi = this->named_values[i];
		if (!vi.used && vi.name == fld_name) {
			vi.used = true;
			return vi;
		}
	}
	fprintf(stderr, "Error at %s: Cannot find a value for field \"%s\" in node \"%s\"\n", this->pos.ToString(), fld_name, this->node_name);
	exit(1);
}

/**
 * check whether a field with name \a fld_name exists.
 * @param fld_name %Name of the field looking for.
 * @return Whether the field exists (\c true means it exists, \c false means it does not exist).
 */
bool Values::HasValue(const char *fld_name)
{
	for (int i = 0; i < this->named_count; i++) {
		ValueInformation &vi = this->named_values[i];
		if (!vi.used && vi.name == fld_name) return true;
	}
	return false;
}

/**
 * Get a numeric value from the named expression with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @param symbols Optional set of identifiers recognized as numeric value.
 * @return The numeric value of the expression.
 */
long long Values::GetNumber(const char *fld_name, const Symbol *symbols)
{
	return FindValue(fld_name).GetNumber(this->pos, this->node_name);
}

/**
 * Get a string value from the named expression with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The value of the string.
 */
std::string Values::GetString(const char *fld_name)
{
	return FindValue(fld_name).GetString(this->pos, this->node_name);
}

/**
 * Get a sprite (#SpriteBlock) from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The sprite.
 */
SpriteBlock *Values::GetSprite(const char *fld_name)
{
	return FindValue(fld_name).GetSprite(this->pos, this->node_name);
}

/**
 * Get a connection from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The connection.
 */
Connection *Values::GetConnection(const char *fld_name)
{
	return FindValue(fld_name).GetConnection(this->pos, this->node_name);
}

/**
 * Get a set of strings from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The set of strings.
 */
Strings *Values::GetStrings(const char *fld_name)
{
	return FindValue(fld_name).GetStrings(this->pos, this->node_name);
}

/** Verify whether all named values were used in a node. */
void Values::VerifyUsage()
{
	for (int i = 0; i < this->unnamed_count; i++) {
		const ValueInformation &vi = this->unnamed_values[i];
		if (!vi.used) {
			fprintf(stderr, "Warning at %s: Unnamed value in node \"%s\" was not used\n", vi.pos.ToString(), this->node_name);
		}
	}
	for (int i = 0; i < this->named_count; i++) {
		const ValueInformation &vi = this->named_values[i];
		if (!vi.used) {
			fprintf(stderr, "Warning at %s: Named value \"%s\" was not used in node \"%s\"\n", vi.pos.ToString(), vi.name.c_str(), this->node_name);
		}
	}
}

/** Names of surface sprites in a single direction of view. */
static const char *_surface_sprite[] = {
	"#",    // SF_FLAT
	"#n",   // SF_N
	"#e",   // SF_E
	"#ne",  // SF_NE
	"#s",   // SF_S
	"#ns",  // SF_NS
	"#es",  // SF_ES
	"#nes", // SF_NES
	"#w",   // SF_W
	"#nw",  // SF_WN
	"#ew",  // SF_WE
	"#new", // SF_WNE
	"#sw",  // SF_WS
	"#nsw", // SF_WNS
	"#esw", // SF_WES
	"#Nb",  // SF_STEEP_N
	"#Eb",  // SF_STEEP_E
	"#Sb",  // SF_STEEP_S
	"#Wb",  // SF_STEEP_W
	"#Nt",  // SF_STEEP_N
	"#Et",  // SF_STEEP_E
	"#St",  // SF_STEEP_S
	"#Wt",  // SF_STEEP_W
};

/**
 * Convert a node group to a TSEL game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created TSEL game block.
 */
static TSELBlock *ConvertTSELNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "TSEL");
	TSELBlock *blk = new TSELBlock;

	Values vals("TSEL", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	/* get the fields and their value. */
	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height   = vals.GetNumber("z_height");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);
		blk->sprites[i] = vals.GetSprite(buffer);
	}

	vals.VerifyUsage();
	return blk;
}

/** Available types of surface. */
static const Symbol _surface_types[] = {
	{"reserved",      0},
	{"the_green",    16},
	{"short_grass",  17},
	{"medium_grass", 18},
	{"long_grass",   19},
	{"sand",         32},
	{"cursor",       48},
	{NULL, 0},
};

/**
 * Convert a node group to a SURF game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created SURF game block.
 */
static SURFBlock *ConvertSURFNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "SURF");
	SURFBlock *sb = new SURFBlock;

	Values vals("SURF", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _surface_types);

	sb->surf_type  = vals.GetNumber("surf_type");
	sb->tile_width = vals.GetNumber("tile_width");
	sb->z_height   = vals.GetNumber("z_height");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);
		sb->sprites[i] = vals.GetSprite(buffer);
	}

	vals.VerifyUsage();
	return sb;
}

/** Names of the foundation sprites. */
static const char *_foundation_sprite[] = {
	"se_e0", // FND_SE_E0,
	"se_0s", // FND_SE_0S,
	"se_es", // FND_SE_ES,
	"sw_s0", // FND_SW_S0,
	"sw_0w", // FND_SW_0W,
	"sw_sw", // FND_SW_SW,
};

/** Numeric symbols of the foundation types. */
static const Symbol _fund_symbols[] = {
	{"reserved",  0},
	{"ground",   16},
	{"wood",     32},
	{"brick",    48},
	{NULL, 0}
};

/**
 * Convert a node group to a FUND game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created FUND game block.
 */
static FUNDBlock *ConvertFUNDNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "FUND");
	FUNDBlock *fb = new FUNDBlock;

	Values vals("FUND", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _fund_symbols);

	fb->found_type = vals.GetNumber("found_type");
	fb->tile_width = vals.GetNumber("tile_width");
	fb->z_height   = vals.GetNumber("z_height");

	for (int i = 0; i < FOUNDATION_COUNT; i++) {
		fb->sprites[i] = vals.GetSprite(_foundation_sprite[i]);
	}

	vals.VerifyUsage();
	return fb;
}

/** Symbols for the PATH game block. */
static const Symbol _path_symbols[] = {
	{"concrete", 16},
	{NULL, 0},
};

/** Names of the PATH sprites. */
static const char *_path_sprites[] = {
	"empty",
	"ne",
	"se",
	"ne_se",
	"ne_se_e",
	"sw",
	"ne_sw",
	"se_sw",
	"se_sw_s",
	"ne_se_sw",
	"ne_se_sw_e",
	"ne_se_sw_s",
	"ne_se_sw_e_s",
	"nw",
	"ne_nw",
	"ne_nw_n",
	"nw_se",
	"ne_nw_se",
	"ne_nw_se_n",
	"ne_nw_se_e",
	"ne_nw_se_n_e",
	"nw_sw",
	"nw_sw_w",
	"ne_nw_sw",
	"ne_nw_sw_n",
	"ne_nw_sw_w",
	"ne_nw_sw_n_w",
	"nw_se_sw",
	"nw_se_sw_s",
	"nw_se_sw_w",
	"nw_se_sw_s_w",
	"ne_nw_se_sw",
	"ne_nw_se_sw_n",
	"ne_nw_se_sw_e",
	"ne_nw_se_sw_n_e",
	"ne_nw_se_sw_s",
	"ne_nw_se_sw_n_s",
	"ne_nw_se_sw_e_s",
	"ne_nw_se_sw_n_e_s",
	"ne_nw_se_sw_w",
	"ne_nw_se_sw_n_w",
	"ne_nw_se_sw_e_w",
	"ne_nw_se_sw_n_e_w",
	"ne_nw_se_sw_s_w",
	"ne_nw_se_sw_n_s_w",
	"ne_nw_se_sw_e_s_w",
	"ne_nw_se_sw_n_e_s_w",
	"ramp_ne",
	"ramp_nw",
	"ramp_se",
	"ramp_sw",
};

/**
 * Convert a node group to a PATH game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created PATH game block.
 */
static PATHBlock *ConvertPATHNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "PATH");
	PATHBlock *blk = new PATHBlock;

	Values vals("PATH", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _path_symbols);

	blk->path_type = vals.GetNumber("path_type");
	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height = vals.GetNumber("z_height");

	for (int i = 0; i < PTS_COUNT; i++) {
		blk->sprites[i] = vals.GetSprite(_path_sprites[i]);
	}

	vals.VerifyUsage();
	return blk;
}

/** Symbols for the platform game block. */
static const Symbol _platform_symbols[] = {
	{"wood", 16},
	{NULL, 0},
};

/** Sprite names of the platform game block. */
static const char *_platform_sprites[] = {
	"ns",
	"ew",
	"ramp_ne",
	"ramp_se",
	"ramp_sw",
	"ramp_nw",
	"right_ramp_ne",
	"right_ramp_se",
	"right_ramp_sw",
	"right_ramp_nw",
	"left_ramp_ne",
	"left_ramp_se",
	"left_ramp_sw",
	"left_ramp_nw",
};

/**
 * Convert a node group to a PLAT game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created PLAT game block.
 */
static PLATBlock *ConvertPLATNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "PLAT");
	PLATBlock *blk = new PLATBlock;

	Values vals("PLAT", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _platform_symbols);

	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height = vals.GetNumber("z_height");
	blk->platform_type = vals.GetNumber("platform_type");

	for (int i = 0; i < PLA_COUNT; i++) {
		blk->sprites[i] = vals.GetSprite(_platform_sprites[i]);
	}

	vals.VerifyUsage();
	return blk;
}

/** Symbols for the support game block. */
static const Symbol _support_symbols[] = {
	{"wood", 16},
	{NULL, 0},
};

/** Sprite names of the support game block. */
static const char *_support_sprites[] = {
	"s_ns",
	"s_ew",
	"d_ns",
	"d_ew",
	"p_ns",
	"p_ew",
	"n#n",
	"n#e",
	"n#ne",
	"n#s",
	"n#ns",
	"n#es",
	"n#nes",
	"n#w",
	"n#nw",
	"n#ew",
	"n#new",
	"n#sw",
	"n#nsw",
	"n#esw",
	"n#N",
	"n#E",
	"n#S",
	"n#W",
};

/**
 * Convert a node group to a PLAT game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created PLAT game block.
 */
static SUPPBlock *ConvertSUPPNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "SUPP");
	SUPPBlock *blk = new SUPPBlock;

	Values vals("SUPP", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _support_symbols);

	blk->support_type = vals.GetNumber("support_type");
	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height = vals.GetNumber("z_height");

	for (int i = 0; i < SPP_COUNT; i++) {
		blk->sprites[i] = vals.GetSprite(_support_sprites[i]);
	}

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a node group to a TCOR game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created TCOR game block.
 */
static TCORBlock *ConvertTCORNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "TCOR");
	TCORBlock *blk = new TCORBlock;

	Values vals("TCOR", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	/* get the fields and their value. */
	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height   = vals.GetNumber("z_height");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);

		buffer[0] = 'n';
		blk->north[i] = vals.GetSprite(buffer);

		buffer[0] = 'e';
		blk->east[i] = vals.GetSprite(buffer);

		buffer[0] = 's';
		blk->south[i] = vals.GetSprite(buffer);

		buffer[0] = 'w';
		blk->west[i] = vals.GetSprite(buffer);
	}

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a node group to a PRSG game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created PRSG game block.
 */
static PRSGBlock *ConvertPRSGNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "PRSG");
	PRSGBlock *blk = new PRSGBlock;

	Values vals("PRSG", ng->pos);
	vals.PrepareNamedValues(ng->values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		PersonGraphics *pg = dynamic_cast<PersonGraphics *>(vi.node_value);
		if (pg == NULL) {
			fprintf(stderr, "Error at %s: Node is not a person_graphics node\n", vi.pos.ToString());
			exit(1);
		}
		blk->person_graphics.push_back(*pg);
		vi.node_value = NULL;
		if (blk->person_graphics.size() > 255) {
			fprintf(stderr, "Error at %s: Too many person graphics in a PRSG block\n", vi.pos.ToString());
			exit(1);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return blk;
}

/** Symbols for an ANIM and ANSP blocks. */
static const Symbol _anim_symbols[] = {
	{"pillar",  8},
	{"earth",  16},
	{"walk_ne", 1}, // Walk in north-east direction.
	{"walk_se", 2}, // Walk in south-east direction.
	{"walk_sw", 3}, // Walk in south-west direction.
	{"walk_nw", 4}, // Walk in north-west direction.
	{NULL, 0}
};

/**
 * Convert a node group to an ANIM game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created ANIM game block.
 */
static ANIMBlock *ConvertANIMNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "ANIM");
	ANIMBlock *blk = new ANIMBlock;

	Values vals("ANIM", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true, _anim_symbols);

	blk->person_type = vals.GetNumber("person_type");
	blk->anim_type = vals.GetNumber("anim_type");

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		FrameData *fd = dynamic_cast<FrameData *>(vi.node_value);
		if (fd == NULL) {
			fprintf(stderr, "Error at %s: Node is not a \"frame_data\" node\n", vi.pos.ToString());
			exit(1);
		}
		blk->frames.push_back(*fd);
		vi.node_value = NULL;
		if (blk->frames.size() > 0xFFFF) {
			fprintf(stderr, "Error at %s: Too many frames in an ANIM block\n", vi.pos.ToString());
			exit(1);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a node group to an ANSP game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created ANSP game block.
 */
static ANSPBlock *ConvertANSPNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "ANSP");
	ANSPBlock *blk = new ANSPBlock;

	Values vals("ANSP", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true, _anim_symbols);

	blk->tile_width  = vals.GetNumber("tile_width");
	blk->person_type = vals.GetNumber("person_type");
	blk->anim_type   = vals.GetNumber("anim_type");

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		SpriteBlock *sp = dynamic_cast<SpriteBlock *>(vi.node_value);
		if (sp == NULL) {
			fprintf(stderr, "Error at %s: Node is not a \"sprite\" node\n", vi.pos.ToString());
			exit(1);
		}
		blk->frames.push_back(sp);
		vi.node_value = NULL;
		if (blk->frames.size() > 0xFFFF) {
			fprintf(stderr, "Error at %s: Too many frames in an ANSP block\n", vi.pos.ToString());
			exit(1);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return blk;
}

/** Symbols of the GBOR block. */
static const Symbol _gbor_symbols[] = {
	{"titlebar", 32},
	{"button", 48},
	{"pressed_button", 49},
	{"rounded_button", 52},
	{"pressed_rounded_button", 53},
	{"frame", 64},
	{"panel", 68},
	{"inset", 80},
	{NULL, 0}
};

/**
 * Convert a GBOR game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static GBORBlock *ConvertGBORNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GBOR");
	GBORBlock *blk = new GBORBlock;

	Values vals("GBOR", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _gbor_symbols);

	blk->widget_type = vals.GetNumber("widget_type");
	blk->border_top = vals.GetNumber("border_top");
	blk->border_left = vals.GetNumber("border_left");
	blk->border_right = vals.GetNumber("border_right");
	blk->border_bottom = vals.GetNumber("border_bottom");
	blk->min_width = vals.GetNumber("min_width");
	blk->min_height = vals.GetNumber("min_height");
	blk->h_stepsize = vals.GetNumber("h_stepsize");
	blk->v_stepsize = vals.GetNumber("v_stepsize");
	blk->tl = vals.GetSprite("top_left");
	blk->tm = vals.GetSprite("top_middle");
	blk->tr = vals.GetSprite("top_right");
	blk->ml = vals.GetSprite("middle_left");
	blk->mm = vals.GetSprite("middle_middle");
	blk->mr = vals.GetSprite("middle_right");
	blk->bl = vals.GetSprite("bottom_left");
	blk->bm = vals.GetSprite("bottom_middle");
	blk->br = vals.GetSprite("bottom_right");

	vals.VerifyUsage();
	return blk;
}

/** Symbols of the GCHK block. */
static const Symbol _gchk_symbols[] = {
	{"check_box", 96},
	{"radio_button", 112},
	{NULL, 0}
};

/**
 * Convert a GCHK game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static GCHKBlock *ConvertGCHKNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GCHK");
	GCHKBlock *blk = new GCHKBlock;

	Values vals("GCHK", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _gchk_symbols);

	blk->widget_type = vals.GetNumber("widget_type");
	blk->empty = vals.GetSprite("empty");
	blk->filled = vals.GetSprite("filled");
	blk->empty_pressed = vals.GetSprite("empty_pressed");
	blk->filled_pressed = vals.GetSprite("filled_pressed");
	blk->shaded_empty = vals.GetSprite("shaded_empty");
	blk->shaded_filled = vals.GetSprite("shaded_filled");

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a GSLI game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static GSLIBlock *ConvertGSLINode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GSLI");
	GSLIBlock *blk = new GSLIBlock;

	Values vals("GSLI", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	blk->min_length = vals.GetNumber("min_length");
	blk->step_size = vals.GetNumber("step_size");
	blk->width = vals.GetNumber("width");
	blk->widget_type = vals.GetNumber("widget_type");
	blk->left = vals.GetSprite("left");
	blk->middle = vals.GetSprite("middle");
	blk->right = vals.GetSprite("right");
	blk->slider = vals.GetSprite("slider");

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a GSCL game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static GSCLBlock *ConvertGSCLNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GSCL");
	GSCLBlock *blk = new GSCLBlock;

	Values vals("GSCL", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	blk->min_length = vals.GetNumber("min_length");
	blk->step_back = vals.GetNumber("step_back");
	blk->min_bar_length = vals.GetNumber("min_bar_length");
	blk->bar_step = vals.GetNumber("bar_step");
	blk->widget_type = vals.GetNumber("widget_type");
	blk->left_button = vals.GetSprite("left_button");
	blk->right_button = vals.GetSprite("right_button");
	blk->left_pressed = vals.GetSprite("left_pressed");
	blk->right_pressed = vals.GetSprite("right_pressed");
	blk->left_bottom = vals.GetSprite("left_bottom");
	blk->middle_bottom = vals.GetSprite("middle_bottom");
	blk->right_bottom = vals.GetSprite("right_bottom");
	blk->left_top = vals.GetSprite("left_top");
	blk->middle_top = vals.GetSprite("middle_top");
	blk->right_top = vals.GetSprite("right_top");
	blk->left_top_pressed = vals.GetSprite("left_top_pressed");
	blk->middle_top_pressed = vals.GetSprite("middle_top_pressed");
	blk->right_top_pressed = vals.GetSprite("right_top_pressed");

	vals.VerifyUsage();
	return blk;
}


/**
 * Convert a node group to a sprite-sheet block.
 * @param ng Generic tree of nodes to convert.
 * @return The created sprite-sheet node.
 */
static BlockNode *ConvertSheetNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "sheet");

	SheetBlock *sb = new SheetBlock(ng->pos);

	Values vals("sheet", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	sb->file     = vals.GetString("file");
	sb->x_base   = vals.GetNumber("x_base");
	sb->y_base   = vals.GetNumber("y_base");
	sb->x_step   = vals.GetNumber("x_step");
	sb->y_step   = vals.GetNumber("y_step");
	sb->x_offset = vals.GetNumber("x_offset");
	sb->y_offset = vals.GetNumber("y_offset");
	sb->width    = vals.GetNumber("width");
	sb->height   = vals.GetNumber("height");

	BitMask *bm = NULL;
	if (vals.HasValue("mask")) {
		ValueInformation &vi = vals.FindValue("mask");
		bm = dynamic_cast<BitMask *>(vi.node_value);
		if (bm == NULL) {
			fprintf(stderr, "Error at %s: Field \"mask\" of node \"sheet\" is not a bitmask node\n", vi.pos.ToString());
			exit(1);
		}
		vi.node_value = NULL;
	}
	sb->mask = bm;

	vals.VerifyUsage();
	return sb;
}

/**
 * Convert a 'sprite' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static SpriteBlock *ConvertSpriteNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "sprite");
	SpriteBlock *sb = new SpriteBlock;

	Values vals("sprite", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	if (vals.named_count > 0) {
		std::string file = vals.GetString("file");
		int xbase        = vals.GetNumber("x_base");
		int ybase        = vals.GetNumber("y_base");
		int width        = vals.GetNumber("width");
		int height       = vals.GetNumber("height");
		int xoffset      = vals.GetNumber("x_offset");
		int yoffset      = vals.GetNumber("y_offset");

		BitMask *bm = NULL;
		if (vals.HasValue("mask")) {
			ValueInformation &vi = vals.FindValue("mask");
			bm = dynamic_cast<BitMask *>(vi.node_value);
			if (bm == NULL) {
				fprintf(stderr, "Error at %s: Field \"mask\" of node \"sprite\" is not a bitmask node\n", vi.pos.ToString());
				exit(1);
			}
		}

		BitMaskData *bmd = (bm == NULL) ? NULL : &bm->data;
		Image img;
		img.LoadFile(file.c_str(), bmd);
		const char *err = sb->sprite_image.CopySprite(&img, xoffset, yoffset, xbase, ybase, width, height);
		if (err != NULL) {
			fprintf(stderr, "Error at %s, loading of the sprite for \"%s\" failed: %s\n", ng->pos.ToString(), ng->name, err);
			exit(1);
		}
	}

	vals.VerifyUsage();
	return sb;
}

/**
 * Convert a 'bitmask' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted bit mask node.
 */
static BitMask *ConvertBitMaskNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "bitmask");
	BitMask *bm = new BitMask();

	Values vals("bitmask", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	bm->data.x_pos = vals.GetNumber("x_pos");
	bm->data.y_pos = vals.GetNumber("y_pos");
	bm->data.type  = vals.GetString("type");

	vals.VerifyUsage();
	return bm;
}

/** Names of person types and colour ranges. */
static const Symbol _person_graphics_symbols[] = {
	{"pillar",  8},
	{"earth",  16},
	{"grey",        COL_GREY},
	{"green_brown", COL_GREEN_BROWN},
	{"brown",       COL_BROWN},
	{"yellow",      COL_YELLOW},
	{"dark_red",    COL_DARK_RED},
	{"dark_green",  COL_DARK_GREEN},
	{"light_green", COL_LIGHT_GREEN},
	{"green",       COL_GREEN},
	{"light_red",   COL_LIGHT_RED},
	{"dark_blue",   COL_DARK_BLUE},
	{"blue",        COL_BLUE},
	{"light_blue",  COL_LIGHT_BLUE},
	{"purple",      COL_PURPLE},
	{"red",         COL_RED},
	{"orange",      COL_ORANGE},
	{"sea_green",   COL_SEA_GREEN},
	{"pink",        COL_PINK},
	{"beige",       COL_BEIGE},
	{NULL, 0}
};

/** Colour ranges for the recolour node. */
static const Symbol _recolour_symbols[] = {
	{"grey",        COL_GREY},
	{"green_brown", COL_GREEN_BROWN},
	{"brown",       COL_BROWN},
	{"yellow",      COL_YELLOW},
	{"dark_red",    COL_DARK_RED},
	{"dark_green",  COL_DARK_GREEN},
	{"light_green", COL_LIGHT_GREEN},
	{"green",       COL_GREEN},
	{"light_red",   COL_LIGHT_RED},
	{"dark_blue",   COL_DARK_BLUE},
	{"blue",        COL_BLUE},
	{"light_blue",  COL_LIGHT_BLUE},
	{"purple",      COL_PURPLE},
	{"red",         COL_RED},
	{"orange",      COL_ORANGE},
	{"sea_green",   COL_SEA_GREEN},
	{"pink",        COL_PINK},
	{"beige",       COL_BEIGE},
	{NULL, 0}
};

/**
 * Convert a 'person_graphics' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static PersonGraphics *ConvertPersonGraphicsNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "person_graphics");
	PersonGraphics *pg = new PersonGraphics;

	Values vals("person_graphics", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true, _person_graphics_symbols);

	pg->person_type = vals.GetNumber("person_type");

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		Recolouring *rc = dynamic_cast<Recolouring *>(vi.node_value);
		if (rc == NULL) {
			fprintf(stderr, "Error at %s: Node is not a recolour node\n", vi.pos.ToString());
			exit(1);
		}
		if (!pg->AddRecolour(rc->orig, rc->replace)) {
			fprintf(stderr, "Error at %s: Recolouring node cannot be stored (maximum is 3)\n", vi.pos.ToString());
			exit(1);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return pg;
}

/**
 * Convert a 'recolour' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static Recolouring *ConvertRecolourNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "recolour");
	Recolouring *rc = new Recolouring;

	Values vals("recolour", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _recolour_symbols);

	rc->orig = vals.GetNumber("original");
	rc->replace = vals.GetNumber("replace");

	vals.VerifyUsage();
	return rc;
}

/**
 * Convert a 'frame_data' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static FrameData *ConvertFrameDataNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "frame_data");
	FrameData *fd = new FrameData;

	Values vals("frame_data", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	fd->duration = vals.GetNumber("duration");
	fd->change_x = vals.GetNumber("change_x");
	fd->change_y = vals.GetNumber("change_y");

	vals.VerifyUsage();
	return fd;
}

/**
 * Convert a 'BDIR' game node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted BDIR game block.
 */
static BDIRBlock *ConvertBDIRNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "BDIR");
	BDIRBlock *bb = new BDIRBlock;

	Values vals("BDIR", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	bb->tile_width = vals.GetNumber("tile_width");
	bb->sprite_ne = vals.GetSprite("ne");
	bb->sprite_se = vals.GetSprite("se");
	bb->sprite_sw = vals.GetSprite("sw");
	bb->sprite_nw = vals.GetSprite("nw");

	vals.VerifyUsage();
	return bb;
}

/** Symbols of the shop game block. */
static const Symbol _shop_symbols[] = {
	{"ne_entrance", 0},
	{"se_entrance", 1},
	{"sw_entrance", 2},
	{"nw_entrance", 3},
	{"drink", 8},
	{"ice_cream", 9},
	{"non_salt_food", 16},
	{"salt_food", 24},
	{"umbrella", 32},
	{"map", 40},
	{NULL, 0},
};

/**
 * Convert a node group to a SHOP game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created SHOP game block.
 */
static SHOPBlock *ConvertSHOPNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "SHOP");
	SHOPBlock *sb = new SHOPBlock;

	Values vals("SHOP", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true, _shop_symbols);

	sb->tile_width = vals.GetNumber("tile_width");
	sb->height = vals.GetNumber("height");
	sb->flags = vals.GetNumber("flags");
	sb->ne_view = vals.GetSprite("ne");
	sb->se_view = vals.GetSprite("se");
	sb->sw_view = vals.GetSprite("sw");
	sb->nw_view = vals.GetSprite("nw");
	sb->item_cost[0] = vals.GetNumber("cost_item1");
	sb->item_cost[1] = vals.GetNumber("cost_item2");
	sb->ownership_cost = vals.GetNumber("cost_ownership");
	sb->opened_cost = vals.GetNumber("cost_opened");
	sb->item_type[0] = vals.GetNumber("type_item1");
	sb->item_type[1] = vals.GetNumber("type_item2");
	sb->shop_text = vals.GetStrings("texts");
	sb->shop_text->CheckTranslations(_shops_string_names, lengthof(_shops_string_names), ng->pos);

	int free_recolour = 0;
	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		Recolouring *rc = dynamic_cast<Recolouring *>(vi.node_value);
		if (rc == NULL) {
			fprintf(stderr, "Error at %s: Node is not a \"recolour\" node\n", vi.pos.ToString());
			exit(1);
		}
		if (free_recolour >= 3) {
			fprintf(stderr, "Error at %s: Recolouring node cannot be stored (maximum is 3)\n", vi.pos.ToString());
			exit(1);
		}
		sb->recol[free_recolour] = *rc;
		free_recolour++;
		vi.used = true;
	}

	vals.VerifyUsage();
	return sb;
}

/**
 * Convert a node group to a GSLP game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created GSLP game block.
 */
static GSLPBlock *ConvertGSLPNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GSLP");
	GSLPBlock *gb = new GSLPBlock;

	Values vals("GSLP", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	gb->vert_down = vals.GetSprite("vert_down");
	gb->steep_down = vals.GetSprite("steep_down");
	gb->gentle_down = vals.GetSprite("gentle_down");
	gb->level = vals.GetSprite("level");
	gb->gentle_up = vals.GetSprite("gentle_up");
	gb->steep_up = vals.GetSprite("steep_up");
	gb->vert_up = vals.GetSprite("vert_up");
	gb->pos_2d = vals.GetSprite("pos_2d");
	gb->neg_2d = vals.GetSprite("neg_2d");
	gb->pos_3d = vals.GetSprite("pos_3d");
	gb->neg_3d = vals.GetSprite("neg_3d");
	gb->close_button = vals.GetSprite("close_button");
	gb->terraform_dot = vals.GetSprite("terraform_dot");
	gb->gui_text = vals.GetStrings("texts");
	gb->gui_text->CheckTranslations(_gui_string_names, lengthof(_gui_string_names), ng->pos);

	vals.VerifyUsage();
	return gb;
}

/**
 * Convert a 'strings' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'strings' node.
 */
static Strings *ConvertStringsNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "strings");
	Strings *strs = new Strings;

	Values vals("strings", ng->pos);
	vals.PrepareNamedValues(ng->values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		TextNode *tn = dynamic_cast<TextNode *>(vi.node_value);
		if (tn == NULL) {
			fprintf(stderr, "Error at %s: Node is not a \"string\" node\n", vi.pos.ToString());
			exit(1);
		}
		std::set<TextNode>::iterator iter = strs->texts.find(*tn);
		if (iter != strs->texts.end()) {
			for (int j = 0; j < LNG_COUNT; j++) {
				if (tn->pos[j].line >= 0) {
					if ((*iter).pos[j].line >= 0) {
						fprintf(stderr, "Error at %s: ", tn->pos[j].ToString());
						fprintf(stderr, "\"string\" node conflicts with %s\n", (*iter).pos[j].ToString());
						exit(1);
					}
					(*iter).pos[j] = tn->pos[j];
					(*iter).texts[j] = tn->texts[j];
				}
			}
		} else {
			strs->texts.insert(*tn);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return strs;
}

/**
 * Convert a 'string' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'string' node.
 */
static TextNode *ConvertTextNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "string");
	TextNode *tn = new TextNode;

	Values vals("string", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	tn->name = vals.GetString("name");
	ValueInformation &vi = vals.FindValue("lang");
	int lng = GetLanguageIndex(vi.GetString(ng->pos, "string").c_str(), vi.pos);
	vi = vals.FindValue("text");
	tn->pos[lng] = vi.pos;
	tn->texts[lng] = vi.GetString(ng->pos, "string");

	vals.VerifyUsage();
	return tn;
}

/** Symbols of a 'track_voxel' node. */
static const Symbol _track_voxel_symbols[] = {
	{"north", 0},
	{"east",  1},
	{"south", 2},
	{"west",  3},
	{NULL, 0}
};

/**
 * Convert a 'track_voxel' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'track_voxel' node.
 */
static TrackVoxel *ConvertTrackVoxel(NodeGroup *ng)
{
	static const char *direction[4] = {"ne", "se", "sw", "nw"};

	ExpandNoExpression(ng->exprs, ng->pos, "track_voxel");
	TrackVoxel *tv = new TrackVoxel;

	Values vals("track_voxel", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _track_voxel_symbols);

	tv->dx = vals.GetNumber("dx");
	tv->dy = vals.GetNumber("dy");
	tv->dz = vals.GetNumber("dz");
	tv->space = vals.GetNumber("space");

	char buffer[16];
	for (int i = 0; i < 4; i++) {
		strcpy(buffer, direction[i]);
		strcpy(buffer + 2, "_back");
		if (vals.HasValue(buffer)) tv->back[i] = vals.GetSprite(buffer);
	}
	for (int i = 0; i < 4; i++) {
		strcpy(buffer, direction[i]);
		strcpy(buffer + 2, "_front");
		if (vals.HasValue(buffer)) tv->front[i] = vals.GetSprite(buffer);
	}

	vals.VerifyUsage();
	return tv;
}

/** Direction symbols of the connection node. */
static const Symbol _connection_symbols[] = {
	{"ne", 0},
	{"se", 1},
	{"sw", 2},
	{"nw", 3},
	{NULL, 0}
};

/**
 * Convert a 'connection' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'connection' node.
 */
static Connection *ConvertConnection(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "connection");
	Connection *cn = new Connection();

	Values vals("connection", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _connection_symbols);

	cn->name = vals.GetString("name");
	cn->direction = vals.GetNumber("direction");

	vals.VerifyUsage();
	return cn;
}

static const Symbol _track_piece_symbols[] = {
	{"none",  0},
	{"left",  1},
	{"right", 2},
	{NULL,    0}
};

/**
 * Convert a 'track_piece' game node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'track_piece' node.
 */
static TrackPieceNode *ConvertTrackPieceNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "track_piece");
	TrackPieceNode *tb = new TrackPieceNode;

	Values vals("track_piece", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true, _track_piece_symbols);

	tb->track_flags = vals.GetNumber("track_flags");
	tb->banking = vals.GetNumber("banking");
	tb->slope = vals.GetNumber("slope");
	tb->bend = vals.GetNumber("bend");
	tb->cost = vals.GetNumber("cost");

	tb->entry = vals.GetConnection("entry");
	tb->exit  = vals.GetConnection("exit");
	tb->exit_dx = vals.GetNumber("exit_dx");
	tb->exit_dy = vals.GetNumber("exit_dy");
	tb->exit_dz = vals.GetNumber("exit_dz");
	tb->speed = vals.HasValue("speed") ? vals.GetNumber("speed") : 0;

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		TrackVoxel *tv = dynamic_cast<TrackVoxel *>(vi.node_value);
		if (tv == NULL) {
			fprintf(stderr, "Error at %s: Node is not a \"track_voxel\" node\n", vi.pos.ToString());
			exit(1);
		}
		tb->track_voxels.push_back(tv);
		vi.node_value = NULL;
		vi.used = true;
	}

	vals.VerifyUsage();
	return tb;
}

/**
 * Convert a 'RCST' node to a game block.
 * @param ng Generic tree of nodes to convert.
 * @return The converted RCST game block.
 */
static RCSTBlock *ConvertRCSTNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "RCST");
	RCSTBlock *rb = new RCSTBlock;

	Values vals("RCST", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true);

	rb->coaster_type = vals.GetNumber("coaster_type");
	rb->platform_type = vals.GetNumber("platform_type");
	rb->text = vals.GetStrings("texts");
	rb->text->CheckTranslations(_coaster_string_names, lengthof(_coaster_string_names), ng->pos);

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		TrackPieceNode *tb = dynamic_cast<TrackPieceNode *>(vi.node_value);
		if (tb == NULL) {
			fprintf(stderr, "Error at %s: Node is not a \"track_piece\" node\n", vi.pos.ToString());
			exit(1);
		}
		rb->track_blocks.push_back(tb);
		vi.node_value = NULL;
		vi.used = true;
	}

	vals.VerifyUsage();
	return rb;
}

/**
 * Convert a node group.
 * @param ng Node group to convert.
 * @return The converted node.
 */
static BlockNode *ConvertNodeGroup(NodeGroup *ng)
{
	if (strcmp(ng->name, "file")  == 0) return ConvertFileNode(ng);
	if (strcmp(ng->name, "sheet") == 0) return ConvertSheetNode(ng);
	if (strcmp(ng->name, "sprite") == 0) return ConvertSpriteNode(ng);
	if (strcmp(ng->name, "person_graphics") == 0) return ConvertPersonGraphicsNode(ng);
	if (strcmp(ng->name, "recolour") == 0) return ConvertRecolourNode(ng);
	if (strcmp(ng->name, "frame_data") == 0) return ConvertFrameDataNode(ng);
	if (strcmp(ng->name, "strings") == 0) return ConvertStringsNode(ng);
	if (strcmp(ng->name, "string") == 0) return ConvertTextNode(ng);
	if (strcmp(ng->name, "track_voxel") == 0) return ConvertTrackVoxel(ng);
	if (strcmp(ng->name, "connection") == 0) return ConvertConnection(ng);
	if (strcmp(ng->name, "track_piece") == 0) return ConvertTrackPieceNode(ng);
	if (strcmp(ng->name, "bitmask") == 0) return ConvertBitMaskNode(ng);

	/* Game blocks. */
	if (strcmp(ng->name, "TSEL") == 0) return ConvertTSELNode(ng);
	if (strcmp(ng->name, "TCOR") == 0) return ConvertTCORNode(ng);
	if (strcmp(ng->name, "SURF") == 0) return ConvertSURFNode(ng);
	if (strcmp(ng->name, "FUND") == 0) return ConvertFUNDNode(ng);
	if (strcmp(ng->name, "PRSG") == 0) return ConvertPRSGNode(ng);
	if (strcmp(ng->name, "ANIM") == 0) return ConvertANIMNode(ng);
	if (strcmp(ng->name, "ANSP") == 0) return ConvertANSPNode(ng);
	if (strcmp(ng->name, "PATH") == 0) return ConvertPATHNode(ng);
	if (strcmp(ng->name, "PLAT") == 0) return ConvertPLATNode(ng);
	if (strcmp(ng->name, "SUPP") == 0) return ConvertSUPPNode(ng);
	if (strcmp(ng->name, "SHOP") == 0) return ConvertSHOPNode(ng);
	if (strcmp(ng->name, "GBOR") == 0) return ConvertGBORNode(ng);
	if (strcmp(ng->name, "GCHK") == 0) return ConvertGCHKNode(ng);
	if (strcmp(ng->name, "GSLI") == 0) return ConvertGSLINode(ng);
	if (strcmp(ng->name, "GSCL") == 0) return ConvertGSCLNode(ng);
	if (strcmp(ng->name, "BDIR") == 0) return ConvertBDIRNode(ng);
	if (strcmp(ng->name, "GSLP") == 0) return ConvertGSLPNode(ng);
	if (strcmp(ng->name, "RCST") == 0) return ConvertRCSTNode(ng);

	/* Unknown type of node. */
	fprintf(stderr, "Error at %s: Do not know how to check and simplify node \"%s\"\n", ng->pos.ToString(), ng->name);
	exit(1);
}

/**
 * Check and convert the tree to nodes.
 * @param values Root node of the tree.
 * @return The converted and checked sequence of file data to write.
 */
FileNodeList *CheckTree(NamedValueList *values)
{
	assert(sizeof(_surface_sprite) / sizeof(_surface_sprite[0]) == SURFACE_COUNT);
	assert(sizeof(_foundation_sprite) / sizeof(_foundation_sprite[0]) == FOUNDATION_COUNT);

	FileNodeList *file_nodes = new FileNodeList;
	Values vals("root", Position("", 1));
	vals.PrepareNamedValues(values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		FileNode *fn = dynamic_cast<FileNode *>(vi.node_value);
		if (fn == NULL) {
			fprintf(stderr, "Error at %s: Node is not a file node\n", vi.pos.ToString());
			exit(1);
		}
		file_nodes->files.push_back(fn);
		vi.node_value = NULL;
		vi.used = true;
	}
	vals.VerifyUsage();
	return file_nodes;
}

/**
 * Generate a header file with string names.
 * @param prefix Kind of strings (either \c "GUI" or \c "SHOPS").
 * @param base Value of the first entry.
 * @param header Name of the output header file.
 */
void GenerateStringsHeaderFile(const char *prefix, const char *base, const char *header)
{
	const char **names = NULL;
	const char *nice_name = NULL;
	uint length = 0;

	if (strcmp(prefix, "GUI") == 0) {
		names = _gui_string_names;
		length = lengthof(_gui_string_names);
		nice_name = "Gui";
	} else if (strcmp(prefix, "SHOPS") == 0) {
		names = _shops_string_names;
		length = lengthof(_shops_string_names);
		nice_name = "Shops";
	} else if (strcmp(prefix, "COASTERS") == 0) {
		names = _coaster_string_names;
		length = lengthof(_coaster_string_names);
		nice_name = "Coasters";
	} else {
		fprintf(stderr, "ERROR: Prefix \"%s\" is not known.\n", prefix);
		exit(1);
	}

	FILE *fp = fopen(header, "w");
	if (fp == NULL) {
		fprintf(stderr, "ERROR: Cannot open header output file \"%s\" for writing.\n", header);
		exit(1);
	}
	fprintf(fp, "// GUI string table for FreeRCT\n");
	fprintf(fp, "// Automagically generated, do not edit\n");
	fprintf(fp, "\n");
	fprintf(fp, "#ifndef %s_STRING_TABLE_H\n", prefix);
	fprintf(fp, "#define %s_STRING_TABLE_H\n", prefix);
	fprintf(fp, "\n");
	fprintf(fp, "/** %s strings table. */\n", nice_name);
	fprintf(fp, "enum %sStrings {\n", nice_name);
	for (uint i = 0; i < length; i++) {
		if (i == 0) {
			fprintf(fp, "\t%s_%s = %s,\n", prefix, names[i], base);
		} else {
			fprintf(fp, "\t%s_%s,\n", prefix, names[i]);
		}
	}
	fprintf(fp, "\n");
	fprintf(fp, "\t%s_STRING_TABLE_END,\n", prefix);
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "#endif\n");
	fclose(fp);
}

/**
 * Generate a code file with string names.
 * @param prefix Kind of strings (either \c "GUI" or \c "SHOPS").
 * @param base Value of the first entry.
 * @param code Name of the output code file.
 */
void GenerateStringsCodeFile(const char *prefix, const char *base, const char *code)
{
	const char **names = NULL;
	const char *nice_name = NULL;
	const char *lower_name = NULL;
	uint length = 0;

	if (strcmp(prefix, "GUI") == 0) {
		names = _gui_string_names;
		length = lengthof(_gui_string_names);
		nice_name = "Gui";
		lower_name = "gui";
	} else if (strcmp(prefix, "SHOPS") == 0) {
		names = _shops_string_names;
		length = lengthof(_shops_string_names);
		nice_name = "Shops";
		lower_name = "shops";
	} else if (strcmp(prefix, "COASTERS") == 0) {
		names = _coaster_string_names;
		length = lengthof(_coaster_string_names);
		nice_name = "Coasters";
		lower_name = "coasters";
	} else {
		fprintf(stderr, "ERROR: Prefix \"%s\" is not known.\n", prefix);
		exit(1);
	}

	FILE *fp = fopen(code, "w");
	if (fp == NULL) {
		fprintf(stderr, "ERROR: Cannot open code output file \"%s\" for writing.\n", code);
		exit(1);
	}
	fprintf(fp, "// GUI string table for FreeRCT\n");
	fprintf(fp, "// Automagically generated, do not edit\n");
	fprintf(fp, "\n");
	fprintf(fp, "/** %s string table array. */\n", nice_name);
	fprintf(fp, "const char *_%s_strings_table[] = {\n", lower_name);
	for (uint i = 0; i < length; i++) {
		fprintf(fp, "\t\"%s\",\n", names[i]);
	}
	fprintf(fp, "\tNULL,\n");
	fprintf(fp, "};\n");
	fclose(fp);
}
