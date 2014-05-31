/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file check_data.cpp Check and simplify functions. */

#include "../stdafx.h"
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <ctime>
#include "ast.h"
#include "nodes.h"
#include "string_names.h"
#include "utils.h"

static std::shared_ptr<BlockNode> ConvertNodeGroup(std::shared_ptr<NodeGroup> ng);

/**
 * Check the number of expressions given in \a exprs, and expand them into the \a out array for easier access.
 * @param exprs %Expression list containing parameters.
 * @param out [out] Output array for storing \a expected expressions from the list.
 * @param expected Expected number of expressions in \a exprs.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node being checked and expanded.
 */
static void ExpandExpressions(std::shared_ptr<ExpressionList> exprs, std::shared_ptr<Expression> out[], size_t expected, const Position &pos, const char *node)
{
	if (exprs == nullptr) {
		if (expected == 0) return;
		fprintf(stderr, "Error at %s: No arguments found for node \"%s\" (expected %lu)\n", pos.ToString(), node, expected);
		exit(1);
	}
	if (exprs->exprs.size() != expected) {
		fprintf(stderr, "Error at %s: Found %lu arguments for node \"%s\", expected %lu\n", pos.ToString(), exprs->exprs.size(), node, expected);
		exit(1);
	}
	int idx = 0;
	for (auto &iter : exprs->exprs) out[idx++] = iter;
}

/**
 * Check that there are no expressions provided in \a exprs. Give an error otherwise.
 * @param exprs %Expression list containing parameters.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node being checked and expanded.
 */
static void ExpandNoExpression(std::shared_ptr<ExpressionList> exprs, const Position &pos, const char *node)
{
	if (exprs == nullptr || exprs->exprs.size() == 0) return;

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
static std::string GetString(std::shared_ptr<Expression> expr, int index, const char *node)
{
	/* Simple case, expression is a string literal. */
	std::shared_ptr<const StringLiteral> sl = std::dynamic_pointer_cast<const StringLiteral>(expr);
	if (sl != nullptr) return sl->text;

	/* General case, compute its value. */
	std::shared_ptr<const Expression> expr2 = expr->Evaluate(nullptr);
	sl = std::dynamic_pointer_cast<const StringLiteral>(expr2);
	if (sl == nullptr) {
		fprintf(stderr, "Error at %s: Expression parameter %d of node %s is not a string", expr->pos.ToString(), index + 1, node);
		exit(1);
	}
	return sl->text;
}

// No  foo(1234) { ... } nodes exist yet, so number parsing is not needed currently.
// /* DISABLED RECOGNITION BY DOXYGEN *
//  * Extract a number from the given expression.
//  * @param expr %Expression to evaluate.
//  * @param index Parameter number (0-based, for error reporting).
//  * @param node %Name of the node.
//  * @param symbols Symbols available for use in the expression.
//  * @return Value of the number.
//  */
// static long long GetNumber(std::shared_ptr<Expression> expr, int index, const char *node, const Symbol *symbols = nullptr)
// {
// 	/* Simple case, expression is a number literal. */
// 	std::shared_ptr<const NumberLiteral> nl = std::dynamic_pointer_cast<const NumberLiteral>(expr);
// 	if (nl != nullptr) return nl->value;
//
// 	/* General case, compute its value. */
// 	std::shared_ptr<const Expression> expr2 = expr->Evaluate(symbols);
// 	nl = std::dynamic_pointer_cast<const NumberLiteral>(expr2);
// 	if (nl == nullptr) {
// 		fprintf(stderr, "Error at %s: Expression parameter %d of node %s is not a number", expr->pos.ToString(), index + 1, node);
// 		exit(1);
// 	}
// 	return nl->value;
// }

/**
 * Convert a 'file' node (taking a string parameter for the filename, and a sequence of game blocks).
 * @param ng Generic tree of nodes to convert to a 'file' node.
 * @return The converted file node.
 */
static std::shared_ptr<FileNode> ConvertFileNode(std::shared_ptr<NodeGroup> ng)
{
	std::shared_ptr<Expression> argument;
	ExpandExpressions(ng->exprs, &argument, 1, ng->pos, "file");

	std::string filename = GetString(argument, 0, "file");
	auto fn = std::make_shared<FileNode>(filename);

	/* Add the meta blocks to the start of the file, while collecting game blocks for appending them later. */
	bool seen_info = false; // Did we encounter an info meta block?
	std::list<std::shared_ptr<GameBlock>> game_blocks;
	for (auto &iter : ng->values->values) {
		std::shared_ptr<NamedValue> nv = std::dynamic_pointer_cast<NamedValue>(iter);
		assert(nv != nullptr); // Should always hold, as ImportValue has been eliminated.
		if (nv->name != nullptr) fprintf(stderr, "Warning at %s: Unexpected name encountered, ignoring\n", nv->name->GetPosition().ToString());
		std::shared_ptr<NodeGroup> ng = std::dynamic_pointer_cast<NodeGroup>(nv->group);
		if (ng == nullptr) {
			fprintf(stderr, "Error at %s: Only node groups may be added\n", nv->group->GetPosition().ToString());
			exit(1);
		}
		auto bn = ConvertNodeGroup(ng);
		auto mb = std::dynamic_pointer_cast<MetaBlock>(bn);
		if (mb != nullptr) {
			seen_info |= strcmp(mb->blk_name, "INFO") == 0;
			fn->blocks.push_back(mb);
			continue;
		}
		auto gb = std::dynamic_pointer_cast<GameBlock>(bn);
		if (gb == nullptr) {
			fprintf(stderr, "Error at %s: Only game blocks can be added to a \"file\" node\n", nv->group->GetPosition().ToString());
			exit(1);
		}
		game_blocks.push_back(gb);
	}
	if (!seen_info) {
		fprintf(stderr, "Error at %s: Missing an INFO block in the file\n", ng->pos.ToString());
		exit(1);
	}

	for (auto &gb : game_blocks) fn->blocks.push_back(gb); // Append the game blocks.
	return fn;
}

/** All information that needs to be stored about a named value. */
class ValueInformation : public std::enable_shared_from_this<ValueInformation> {
public:
	ValueInformation();
	ValueInformation(const std::string &name, const Position &pos);

	long long GetNumber(const Position &pos, const char *node, const Symbol *symbols = nullptr);
	std::string GetString(const Position &pos, const char *node);
	std::shared_ptr<SpriteBlock> GetSprite(const Position &pos, const char *node);
	std::shared_ptr<Connection> GetConnection(const Position &pos, const char *node);
	std::shared_ptr<StringsNode> GetStrings(const Position &pos, const char *node);
	std::shared_ptr<Curve> GetCurve(const Position &pos, const char *node);

	std::string name; ///< %Name of the value.
	Position pos;     ///< %Position of the name.
	bool used;        ///< Is the value used?

	std::shared_ptr<const Expression> expr_value; ///< %Expression attached to it (if any).
	std::shared_ptr<BlockNode> node_value;        ///< Node attached to it (if any).
};

/** Default constructor. */
ValueInformation::ValueInformation() : pos("", 0)
{
	this->expr_value = nullptr;
	this->node_value = nullptr;
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
	this->expr_value = nullptr;
	this->node_value = nullptr;
	this->name = name;
	this->used = false;
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
	if (this->expr_value == nullptr) {
fail:
		fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a numeric value\n", pos.ToString(), this->name.c_str(), node);
		exit(1);
	}
	std::shared_ptr<const NumberLiteral> nl = std::dynamic_pointer_cast<const NumberLiteral>(this->expr_value); // Simple common case.
	if (nl != nullptr) return nl->value;

	std::shared_ptr<const Expression> expr2 = this->expr_value->Evaluate(symbols); // Generic case, evaluate the expression.
	nl = std::dynamic_pointer_cast<const NumberLiteral>(expr2);
	if (nl == nullptr) goto fail;

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
	if (this->expr_value == nullptr) {
fail:
		fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a string value\n", pos.ToString(), this->name.c_str(), node);
		exit(1);
	}
	std::shared_ptr<const StringLiteral> sl = std::dynamic_pointer_cast<const StringLiteral>(this->expr_value); // Simple common case.
	if (sl != nullptr) return sl->text;

	std::shared_ptr<const Expression> expr2 = this->expr_value->Evaluate(nullptr); // Generic case
	sl = std::dynamic_pointer_cast<const StringLiteral>(expr2);
	if (sl == nullptr) goto fail;

	return sl->text;
}

/**
 * Get a sprite (#SpriteBlock) from the given node value.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node.
 * @return The sprite.
 */
std::shared_ptr<SpriteBlock> ValueInformation::GetSprite(const Position &pos, const char *node)
{
	auto sb = std::dynamic_pointer_cast<SpriteBlock>(this->node_value);
	if (sb != nullptr) {
		this->node_value = nullptr;
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
std::shared_ptr<Connection> ValueInformation::GetConnection(const Position &pos, const char *node)
{
	auto cn = std::dynamic_pointer_cast<Connection>(this->node_value);
	if (cn != nullptr) {
		this->node_value = nullptr;
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
std::shared_ptr<StringsNode> ValueInformation::GetStrings(const Position &pos, const char *node)
{
	auto st = std::dynamic_pointer_cast<StringsNode>(this->node_value);
	if (st != nullptr) {
		this->node_value = nullptr;
		return st;
	}
	fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a strings node\n", pos.ToString(), this->name.c_str(), node);
	exit(1);
}

/**
 * Get a curve description from the node value.
 * @param pos %Position of the node (for reporting errors).
 * @param node %Name of the node.
 * @return The requested curve.
 */
std::shared_ptr<Curve> ValueInformation::GetCurve(const Position &pos, const char *node)
{
	/* First option, it's an expression (ie a fixed value). */
	if (this->expr_value != nullptr) {
		auto cv = std::make_shared<FixedTable>();
		cv->value = this->GetNumber(pos, node, nullptr);
		return cv;
	}
	/* Second option, it's a 'splines' block. */
	auto cv = std::dynamic_pointer_cast<CubicSplines>(this->node_value);
	if (cv != nullptr) {
		this->node_value = nullptr;
		return cv;
	}
	fprintf(stderr, "Error at %s: Field \"%s\" of node \"%s\" is not a valid car table node\n", pos.ToString(), this->name.c_str(), node);
	exit(1);
}

/**
 * Assign sub-nodes to the names of a 2D table.
 * @param bn Node to split in sub-nodes.
 * @param nt 2D name table.
 * @param vis Array being filled.
 * @param length [inout] Updated number of used entries in \a vis.
 */
static void AssignNames(std::shared_ptr<BlockNode> bn, std::shared_ptr<NameTable> nt, std::vector<std::shared_ptr<ValueInformation>> &vis, int *length)
{
	/* Is a table consisting of a single parameterized name? */
	if (nt->HasSingleElement()) {
		std::shared_ptr<IdentifierLine> il = nt->rows.front()->identifiers.front();
		if (!il->IsValid()) return; // Only available name cannot be used.

		ParameterizedName parms_name;
		HorVert hv = parms_name.DecodeName(il->name.c_str(), il->pos);
		if (hv != HV_NONE) {
			/* Expand the parameterized name. */
			int row = 0;
			do {
				int col = 0;
				do {
					const char *nm = parms_name.GetParmName(row, col);
					std::shared_ptr<ValueInformation> vi = std::make_shared<ValueInformation>();
					vi->expr_value = nullptr;
					vi->node_value = bn->GetSubNode(row, col, nm, il->pos);
					vi->name = std::string(nm);
					vi->pos = il->pos;
					vi->used = false;
					vis[*length] = vi;
					(*length)++;

					col++;
				} while (parms_name.vert_range.used && col < parms_name.vert_range.size());
				row++;
			} while (parms_name.hor_range.used && row < parms_name.hor_range.size());
			return;
		}
	}

	/* General case, a 2D table with non-parameterized names. */
	int row = 0;
	for (auto &nr : nt->rows) {
		int col = 0;
		for (auto &il : nr->identifiers) {
			if (il->IsValid()) {
				CheckIsSingleName(il->name, il->pos);
				std::shared_ptr<ValueInformation> vi = std::make_shared<ValueInformation>();
				vi->expr_value = nullptr;
				vi->node_value = bn->GetSubNode(row, col, il->name.c_str(), il->pos);
				vi->name = std::string(il->name);
				vi->pos = il->pos;
				vi->used = false;
				vis[*length] = vi;
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

	void PrepareNamedValues(std::shared_ptr<NamedValueList> values, bool allow_named, bool allow_unnamed, const Symbol *symbols = nullptr);
	std::shared_ptr<ValueInformation> FindValue(const char *fld_name);
	bool HasValue(const char *fld_name);
	long long GetNumber(const char *fld_name, const Symbol *symbols = nullptr);
	std::string GetString(const char *fld_name);
	std::shared_ptr<SpriteBlock> GetSprite(const char *fld_name);
	std::shared_ptr<Connection> GetConnection(const char *fld_name);
	std::shared_ptr<StringsNode> GetStrings(const char *fld_name);
	std::shared_ptr<Curve> GetCurve(const char *fld_name);
	void VerifyUsage();

	int named_count;   ///< Number of found values with a name.
	int unnamed_count; ///< Number of found value without a name.

	std::vector<std::shared_ptr<ValueInformation>> named_values;   ///< Information about each named value.
	std::vector<std::shared_ptr<ValueInformation>> unnamed_values; ///< Information about each unnamed value.

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
}

/**
 * Create space for the information of of the values.
 * @param named_count Number of named values.
 * @param unnamed_count Number of unnamed valued.
 */
void Values::CreateValues(int named_count, int unnamed_count)
{
	this->named_count = named_count;
	this->unnamed_count = unnamed_count;

	this->named_values.clear();
	this->named_values.reserve(named_count);
	for (int i = 0; i < named_count; i++) this->named_values.push_back(std::make_shared<ValueInformation>());

	this->unnamed_values.clear();
	this->unnamed_values.reserve(unnamed_count);
	for (int i = 0; i < unnamed_count; i++) this->unnamed_values.push_back(std::make_shared<ValueInformation>());
}

Values::~Values()
{
}

/**
 * Prepare the named values for access by field name.
 * @param values Named value to prepare.
 * @param allow_named Allow named values.
 * @param allow_unnamed Allow unnamed values.
 * @param symbols Optional symbols that may be used in the values.
 */
void Values::PrepareNamedValues(std::shared_ptr<NamedValueList> values, bool allow_named, bool allow_unnamed, const Symbol *symbols)
{
	/* Count number of named and unnamed values. */
	int named_count = 0;
	int unnamed_count = 0;
	for (auto &iter : values->values) {
		std::shared_ptr<NamedValue> nv = std::dynamic_pointer_cast<NamedValue>(iter);
		assert(nv != nullptr); // Should always hold, as ImportValue has been eliminated.
		if (nv->name == nullptr) { // Unnamed value.
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
	for (auto &iter : values->values) {
		std::shared_ptr<NamedValue> nv = std::dynamic_pointer_cast<NamedValue>(iter);
		assert(nv != nullptr); // Should always hold, as ImportValue has been eliminated.
		if (nv->name == nullptr) { // Unnamed value.
			std::shared_ptr<NodeGroup> ng = std::dynamic_pointer_cast<NodeGroup>(nv->group);
			if (ng != nullptr) {
				this->unnamed_values[unnamed_count]->expr_value = nullptr;
				this->unnamed_values[unnamed_count]->node_value = ConvertNodeGroup(ng);
				this->unnamed_values[unnamed_count]->name = "???";
				this->unnamed_values[unnamed_count]->pos = ng->GetPosition();
				this->unnamed_values[unnamed_count]->used = false;
				unnamed_count++;
				continue;
			}
			std::shared_ptr<ExpressionGroup> eg = std::dynamic_pointer_cast<ExpressionGroup>(nv->group);
			assert(eg != nullptr);
			this->unnamed_values[unnamed_count]->expr_value = eg->expr->Evaluate(symbols);
			this->unnamed_values[unnamed_count]->node_value = nullptr;
			this->unnamed_values[unnamed_count]->name = "???";
			this->unnamed_values[unnamed_count]->pos = ng->GetPosition();
			this->unnamed_values[unnamed_count]->used = false;
			unnamed_count++;
			continue;
		} else { // Named value.
			std::shared_ptr<NodeGroup> ng = std::dynamic_pointer_cast<NodeGroup>(nv->group);
			if (ng != nullptr) {
				auto bn = ConvertNodeGroup(ng);
				std::shared_ptr<SingleName> sn = std::dynamic_pointer_cast<SingleName>(nv->name);
				if (sn != nullptr) {
					this->named_values[named_count]->expr_value = nullptr;
					this->named_values[named_count]->node_value = bn;
					this->named_values[named_count]->name = std::string(sn->name);
					this->named_values[named_count]->pos = sn->pos;
					this->named_values[named_count]->used = false;
					named_count++;
					continue;
				}
				std::shared_ptr<NameTable> nt = std::dynamic_pointer_cast<NameTable>(nv->name);
				assert(nt != nullptr);
				AssignNames(bn, nt, this->named_values, &named_count);
				continue;
			}

			/* Expression group. */
			std::shared_ptr<ExpressionGroup> eg = std::dynamic_pointer_cast<ExpressionGroup>(nv->group);
			assert(eg != nullptr);
			std::shared_ptr<SingleName> sn = std::dynamic_pointer_cast<SingleName>(nv->name);
			if (sn == nullptr) {
				fprintf(stderr, "Error at %s: Expression must have a single name\n", nv->name->GetPosition().ToString());
				exit(1);
			}
			this->named_values[named_count]->expr_value = eg->expr->Evaluate(symbols);
			this->named_values[named_count]->node_value = nullptr;
			this->named_values[named_count]->name = std::string(sn->name);
			this->named_values[named_count]->pos = sn->pos;
			this->named_values[named_count]->used = false;
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
std::shared_ptr<ValueInformation> Values::FindValue(const char *fld_name)
{
	for (int i = 0; i < this->named_count; i++) {
		std::shared_ptr<ValueInformation> vi = this->named_values[i];
		if (!vi->used && vi->name == fld_name) {
			vi->used = true;
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
		std::shared_ptr<ValueInformation> vi = this->named_values[i];
		if (!vi->used && vi->name == fld_name) return true;
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
	return FindValue(fld_name)->GetNumber(this->pos, this->node_name);
}

/**
 * Get a string value from the named expression with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The value of the string.
 */
std::string Values::GetString(const char *fld_name)
{
	return FindValue(fld_name)->GetString(this->pos, this->node_name);
}

/**
 * Get a sprite (#SpriteBlock) from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The sprite.
 */
std::shared_ptr<SpriteBlock> Values::GetSprite(const char *fld_name)
{
	return FindValue(fld_name)->GetSprite(this->pos, this->node_name);
}

/**
 * Get a connection from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The connection.
 */
std::shared_ptr<Connection> Values::GetConnection(const char *fld_name)
{
	return FindValue(fld_name)->GetConnection(this->pos, this->node_name);
}

/**
 * Get a set of strings from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The set of strings.
 */
std::shared_ptr<StringsNode> Values::GetStrings(const char *fld_name)
{
	return FindValue(fld_name)->GetStrings(this->pos, this->node_name);
}

/**
 * Get a curve description from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The queried curve.
 */
std::shared_ptr<Curve> Values::GetCurve(const char *fld_name)
{
	return FindValue(fld_name)->GetCurve(this->pos, this->node_name);
}

/** Verify whether all named values were used in a node. */
void Values::VerifyUsage()
{
	for (int i = 0; i < this->unnamed_count; i++) {
		const std::shared_ptr<ValueInformation> &vi = this->unnamed_values[i];
		if (!vi->used) {
			fprintf(stderr, "Warning at %s: Unnamed value in node \"%s\" was not used\n", vi->pos.ToString(), this->node_name);
		}
	}
	for (int i = 0; i < this->named_count; i++) {
		const std::shared_ptr<ValueInformation> &vi = this->named_values[i];
		if (!vi->used) {
			fprintf(stderr, "Warning at %s: Named value \"%s\" was not used in node \"%s\"\n", vi->pos.ToString(), vi->name.c_str(), this->node_name);
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
static std::shared_ptr<TSELBlock> ConvertTSELNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "TSEL");
	auto blk = std::make_shared<TSELBlock>();

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
	{nullptr, 0},
};

/**
 * Convert a node group to a SURF game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created SURF game block.
 */
static std::shared_ptr<SURFBlock> ConvertSURFNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "SURF");
	auto sb = std::make_shared<SURFBlock>();

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
	{nullptr, 0}
};

/**
 * Convert a node group to a FUND game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created FUND game block.
 */
static std::shared_ptr<FUNDBlock> ConvertFUNDNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "FUND");
	auto fb = std::make_shared<FUNDBlock>();

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
	{nullptr, 0},
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
static std::shared_ptr<PATHBlock> ConvertPATHNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "PATH");
	auto blk = std::make_shared<PATHBlock>();

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
	{nullptr, 0},
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
static std::shared_ptr<PLATBlock> ConvertPLATNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "PLAT");
	auto blk = std::make_shared<PLATBlock>();

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
	{nullptr, 0},
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
static std::shared_ptr<SUPPBlock> ConvertSUPPNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "SUPP");
	auto blk = std::make_shared<SUPPBlock>();

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
static std::shared_ptr<TCORBlock> ConvertTCORNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "TCOR");
	auto blk = std::make_shared<TCORBlock>();

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
static std::shared_ptr<PRSGBlock> ConvertPRSGNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "PRSG");
	auto blk = std::make_shared<PRSGBlock>();

	Values vals("PRSG", ng->pos);
	vals.PrepareNamedValues(ng->values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> &vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto pg = std::dynamic_pointer_cast<PersonGraphics>(vi->node_value);
		if (pg == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a person_graphics node\n", vi->pos.ToString());
			exit(1);
		}
		blk->person_graphics.push_back(pg);
		vi->node_value = nullptr;
		if (blk->person_graphics.size() > 255) {
			fprintf(stderr, "Error at %s: Too many person graphics in a PRSG block\n", vi->pos.ToString());
			exit(1);
		}
		vi->used = true;
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
	{nullptr, 0}
};

/**
 * Convert a node group to an ANIM game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created ANIM game block.
 */
static std::shared_ptr<ANIMBlock> ConvertANIMNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "ANIM");
	auto blk = std::make_shared<ANIMBlock>();

	Values vals("ANIM", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true, _anim_symbols);

	blk->person_type = vals.GetNumber("person_type");
	blk->anim_type = vals.GetNumber("anim_type");

	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> &vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto fd = std::dynamic_pointer_cast<FrameData>(vi->node_value);
		if (fd == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a \"frame_data\" node\n", vi->pos.ToString());
			exit(1);
		}
		blk->frames.push_back(fd);
		vi->node_value = nullptr;
		if (blk->frames.size() > 0xFFFF) {
			fprintf(stderr, "Error at %s: Too many frames in an ANIM block\n", vi->pos.ToString());
			exit(1);
		}
		vi->used = true;
	}

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a node group to an ANSP game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created ANSP game block.
 */
static std::shared_ptr<ANSPBlock> ConvertANSPNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "ANSP");
	auto blk = std::make_shared<ANSPBlock>();

	Values vals("ANSP", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true, _anim_symbols);

	blk->tile_width  = vals.GetNumber("tile_width");
	blk->person_type = vals.GetNumber("person_type");
	blk->anim_type   = vals.GetNumber("anim_type");

	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> &vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto sp = std::dynamic_pointer_cast<SpriteBlock>(vi->node_value);
		if (sp == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a \"sprite\" node\n", vi->pos.ToString());
			exit(1);
		}
		blk->frames.push_back(sp);
		vi->node_value = nullptr;
		if (blk->frames.size() > 0xFFFF) {
			fprintf(stderr, "Error at %s: Too many frames in an ANSP block\n", vi->pos.ToString());
			exit(1);
		}
		vi->used = true;
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
	{nullptr, 0}
};

/**
 * Convert a GBOR game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static std::shared_ptr<GBORBlock> ConvertGBORNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GBOR");
	auto blk = std::make_shared<GBORBlock>();

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
	{nullptr, 0}
};

/**
 * Convert a GCHK game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static std::shared_ptr<GCHKBlock> ConvertGCHKNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GCHK");
	auto blk = std::make_shared<GCHKBlock>();

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
static std::shared_ptr<GSLIBlock> ConvertGSLINode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GSLI");
	auto blk = std::make_shared<GSLIBlock>();

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
static std::shared_ptr<GSCLBlock> ConvertGSCLNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GSCL");
	auto blk = std::make_shared<GSCLBlock>();

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
static std::shared_ptr<SheetBlock> ConvertSheetNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "sheet");
	auto sb = std::make_shared<SheetBlock>(ng->pos);

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

	sb->recolour = "";
	if (vals.HasValue("recolour")) sb->recolour = vals.GetString("recolour");

	sb->crop = true;
	if (vals.HasValue("crop")) sb->crop = vals.GetNumber("crop") != 0;
	sb->x_count = -1;
	if (vals.HasValue("x_count")) sb->x_count = vals.GetNumber("x_count");
	sb->y_count = -1;
	if (vals.HasValue("y_count")) sb->y_count = vals.GetNumber("y_count");

	std::shared_ptr<BitMask> bm(nullptr);
	if (vals.HasValue("mask")) {
		std::shared_ptr<ValueInformation> vi = vals.FindValue("mask");
		bm = std::dynamic_pointer_cast<BitMask>(vi->node_value);
		if (bm == nullptr) {
			fprintf(stderr, "Error at %s: Field \"mask\" of node \"sheet\" is not a bitmask node\n", vi->pos.ToString());
			exit(1);
		}
		vi->node_value = nullptr;
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
static std::shared_ptr<SpriteBlock> ConvertSpriteNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "sprite");
	auto sb = std::make_shared<SpriteBlock>();

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

		std::string recolour = "";
		if (vals.HasValue("recolour")) recolour = vals.GetString("recolour");

		bool crop = true;
		if (vals.HasValue("crop")) crop = vals.GetNumber("crop") != 0;

		std::shared_ptr<BitMask> bm;
		if (vals.HasValue("mask")) {
			std::shared_ptr<ValueInformation> vi = vals.FindValue("mask");
			bm = std::dynamic_pointer_cast<BitMask>(vi->node_value);
			if (bm == nullptr) {
				fprintf(stderr, "Error at %s: Field \"mask\" of node \"sprite\" is not a bitmask node\n", vi->pos.ToString());
				exit(1);
			}
		}

		ImageFile imf;
		const char *err = imf.LoadFile(file);
		if (err != nullptr) {
			fprintf(stderr, "Error at %s, loading of the sprite for \"%s\" failed: %s\n", ng->pos.ToString(), ng->name.c_str(), err);
			exit(1);
		}

		BitMaskData *bmd = (bm == nullptr) ? nullptr : &bm->data;
		if (imf.Is8bpp()) {
			Image8bpp img(&imf, bmd);
			if (recolour != "") fprintf(stderr, "Error at %s, cannot recolour an 8bpp image, ignoring the file.\n", ng->pos.ToString());
			err = sb->sprite_image.CopySprite(&img, xoffset, yoffset, xbase, ybase, width, height, crop);
		} else {
			Image32bpp img(&imf, bmd);
			if (recolour == "") {
				err = sb->sprite_image.CopySprite(&img, xoffset, yoffset, xbase, ybase, width, height, crop);
			} else {
				ImageFile rmf;
				err = rmf.LoadFile(recolour);
				if (err != nullptr) {
					fprintf(stderr, "Error at %s, loading of the recolour file failed: %s\n", ng->pos.ToString(), err);
					exit(1);
				}
				if (!rmf.Is8bpp()) {
					fprintf(stderr, "Error at %s, recolour file must be an 8bpp image.\n", ng->pos.ToString());
					exit(1);
				}
				Image8bpp rim(&rmf, nullptr);
				img.SetRecolourImage(&rim);
				err = sb->sprite_image.CopySprite(&img, xoffset, yoffset, xbase, ybase, width, height, crop);
			}
		}
		if (err != nullptr) {
			fprintf(stderr, "Error at %s, copying the sprite for \"%s\" failed: %s\n", ng->pos.ToString(), ng->name.c_str(), err);
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
static std::shared_ptr<BitMask> ConvertBitMaskNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "bitmask");
	auto bm = std::make_shared<BitMask>();

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
	{"grey",         COL_GREY},
	{"green_brown",  COL_GREEN_BROWN},
	{"orange_brown", COL_ORANGE_BROWN},
	{"yellow",       COL_YELLOW},
	{"dark_red",     COL_DARK_RED},
	{"dark_green",   COL_DARK_GREEN},
	{"light_green",  COL_LIGHT_GREEN},
	{"green",        COL_GREEN},
	{"pink_brown",   COL_PINK_BROWN},
	{"dark_purple",  COL_DARK_PURPLE},
	{"blue",         COL_BLUE},
	{"jade_green",   COL_DARK_JADE_GREEN},
	{"purple",       COL_PURPLE},
	{"red",          COL_RED},
	{"orange",       COL_ORANGE},
	{"sea_green",    COL_SEA_GREEN},
	{"pink",         COL_PINK},
	{"brown",        COL_BROWN},
	{nullptr, 0}
};

/** Colour ranges for the recolour node. */
static const Symbol _recolour_symbols[] = {
	{"grey",         COL_GREY},
	{"green_brown",  COL_GREEN_BROWN},
	{"orange_brown", COL_ORANGE_BROWN},
	{"yellow",       COL_YELLOW},
	{"dark_red",     COL_DARK_RED},
	{"dark_green",   COL_DARK_GREEN},
	{"light_green",  COL_LIGHT_GREEN},
	{"green",        COL_GREEN},
	{"pink_brown",   COL_PINK_BROWN},
	{"dark_purple",  COL_DARK_PURPLE},
	{"blue",         COL_BLUE},
	{"jade_green",   COL_DARK_JADE_GREEN},
	{"purple",       COL_PURPLE},
	{"red",          COL_RED},
	{"orange",       COL_ORANGE},
	{"sea_green",    COL_SEA_GREEN},
	{"pink",         COL_PINK},
	{"brown",        COL_BROWN},
	{nullptr, 0}
};

/**
 * Convert a 'person_graphics' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static std::shared_ptr<PersonGraphics> ConvertPersonGraphicsNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "person_graphics");
	auto pg = std::make_shared<PersonGraphics>();

	Values vals("person_graphics", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true, _person_graphics_symbols);

	pg->person_type = vals.GetNumber("person_type");

	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> &vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto rc = std::dynamic_pointer_cast<Recolouring>(vi->node_value);
		if (rc == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a recolour node\n", vi->pos.ToString());
			exit(1);
		}
		if (!pg->AddRecolour(rc->orig, rc->replace)) {
			fprintf(stderr, "Error at %s: Recolouring node cannot be stored (maximum is 3)\n", vi->pos.ToString());
			exit(1);
		}
		vi->used = true;
	}

	vals.VerifyUsage();
	return pg;
}

/**
 * Convert a 'recolour' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static std::shared_ptr<Recolouring> ConvertRecolourNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "recolour");
	auto rc = std::make_shared<Recolouring>();

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
static std::shared_ptr<FrameData> ConvertFrameDataNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "frame_data");
	auto fd = std::make_shared<FrameData>();

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
static std::shared_ptr<BDIRBlock> ConvertBDIRNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "BDIR");
	auto bb = std::make_shared<BDIRBlock>();

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
	{nullptr, 0},
};

/**
 * Convert a node group to a SHOP game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created SHOP game block.
 */
static std::shared_ptr<SHOPBlock> ConvertSHOPNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "SHOP");
	auto sb = std::make_shared<SHOPBlock>();

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
	sb->shop_text = std::make_shared<StringBundle>();
	sb->shop_text->Fill(vals.GetStrings("texts"));
	sb->shop_text->CheckTranslations(_shops_string_names, lengthof(_shops_string_names), ng->pos);

	int free_recolour = 0;
	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto rc = std::dynamic_pointer_cast<Recolouring>(vi->node_value);
		if (rc == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a \"recolour\" node\n", vi->pos.ToString());
			exit(1);
		}
		if (free_recolour >= 3) {
			fprintf(stderr, "Error at %s: Recolouring node cannot be stored (maximum is 3)\n", vi->pos.ToString());
			exit(1);
		}
		sb->recol[free_recolour] = *rc;
		free_recolour++;
		vi->used = true;
	}

	vals.VerifyUsage();
	return sb;
}

/**
 * Convert a node group to a GSLP game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created GSLP game block.
 */
static std::shared_ptr<GSLPBlock> ConvertGSLPNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "GSLP");
	auto gb = std::make_shared<GSLPBlock>();

	Values vals("GSLP", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	gb->vert_down = vals.GetSprite("vert_down");
	gb->steep_down = vals.GetSprite("steep_down");
	gb->gentle_down = vals.GetSprite("gentle_down");
	gb->level = vals.GetSprite("level");
	gb->gentle_up = vals.GetSprite("gentle_up");
	gb->steep_up = vals.GetSprite("steep_up");
	gb->vert_up = vals.GetSprite("vert_up");

	gb->wide_left = vals.GetSprite("wide_left");
	gb->normal_left = vals.GetSprite("normal_left");
	gb->tight_left = vals.GetSprite("tight_left");
	gb->no_bend = vals.GetSprite("no_bend");
	gb->tight_right = vals.GetSprite("tight_right");
	gb->normal_right = vals.GetSprite("normal_right");
	gb->wide_right = vals.GetSprite("wide_right");

	gb->bank_left = vals.GetSprite("bank_left");
	gb->bank_right = vals.GetSprite("bank_right");
	gb->no_banking = vals.GetSprite("no_banking");

	gb->triangle_right = vals.GetSprite("triangle_right");
	gb->triangle_left = vals.GetSprite("triangle_left");
	gb->triangle_up = vals.GetSprite("triangle_up");
	gb->triangle_bottom = vals.GetSprite("triangle_bottom");

	gb->has_platform = vals.GetSprite("has_platform");
	gb->no_platform = vals.GetSprite("no_platform");

	gb->has_power = vals.GetSprite("has_power");
	gb->no_power = vals.GetSprite("no_power");

	gb->disabled = vals.GetSprite("disabled");

	static const char *compass_names[] = {
		"compass_n", "compass_e", "compass_s", "compass_w", nullptr
	};
	for (uint i = 0; i < lengthof(gb->compass); i++) {
		const char *name = compass_names[i];
		assert(name != nullptr);
		gb->compass[i] = vals.GetSprite(name);
	}

	gb->bulldozer = vals.GetSprite("bulldozer");

	static const char *weather_names[] = {
		"weather_0", "weather_1", "weather_2", "weather_3", nullptr
	};
	for (uint i = 0; i < lengthof(gb->weather); i++) {
		const char *name = weather_names[i];
		assert(name != nullptr);
		gb->weather[i] = vals.GetSprite(name);
	}

	gb->pos_2d = vals.GetSprite("pos_2d");
	gb->neg_2d = vals.GetSprite("neg_2d");
	gb->pos_3d = vals.GetSprite("pos_3d");
	gb->neg_3d = vals.GetSprite("neg_3d");
	gb->close_button = vals.GetSprite("close_button");
	gb->terraform_dot = vals.GetSprite("terraform_dot");
	gb->gui_text = std::make_shared<StringBundle>();
	gb->gui_text->Fill(vals.GetStrings("texts"));
	gb->gui_text->CheckTranslations(_gui_string_names, lengthof(_gui_string_names), ng->pos);

	vals.VerifyUsage();
	return gb;
}

/**
 * Convert a 'strings' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'strings' node.
 */
static std::shared_ptr<StringsNode> ConvertStringsNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "strings");
	auto strs = std::make_shared<StringsNode>();

	Values vals("strings", ng->pos);
	vals.PrepareNamedValues(ng->values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto tn = std::dynamic_pointer_cast<StringNode>(vi->node_value);
		if (tn != nullptr) {
			strs->Add(*tn);
			vi->used = true;
			continue;
		}
		auto sn = std::dynamic_pointer_cast<StringsNode>(vi->node_value);
		if (sn != nullptr) {
			for (const auto &iter : sn->strings) strs->Add(iter);
			vi->used = true;
			continue;
		}
		fprintf(stderr, "Error at %s: Node is not a \"string\" nor a \"strings\" node\n", vi->pos.ToString());
		exit(1);
	}

	vals.VerifyUsage();
	return strs;
}

/**
 * Convert a 'string' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'string' node.
 */
static std::shared_ptr<StringNode> ConvertStringNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "string");
	auto tn = std::make_shared<StringNode>();
	Values vals("string", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	tn->name = vals.GetString("name");
	std::shared_ptr<ValueInformation> vi = vals.FindValue("text");
	tn->text = vi->GetString(ng->pos, "string");
	tn->text_pos = vi->pos;
	if (vals.HasValue("lang")) {
		vi = vals.FindValue("lang");
		tn->lang_index = GetLanguageIndex(vi->GetString(ng->pos, "string").c_str(), vi->pos);
	} else {
		tn->lang_index = -1;
	}

	vals.VerifyUsage();
	return tn;
}

/** Symbols of a 'track_voxel' node. */
static const Symbol _track_voxel_symbols[] = {
	{"north", 0},
	{"east",  1},
	{"south", 2},
	{"west",  3},
	{"nesw",  1 << 4},
	{"senw",  2 << 4},
	{"swne",  3 << 4},
	{"nwse",  4 << 4},
	{nullptr, 0}
};

/**
 * Convert a 'track_voxel' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'track_voxel' node.
 */
static std::shared_ptr<TrackVoxel> ConvertTrackVoxel(std::shared_ptr<NodeGroup> ng)
{
	static const char *direction[4] = {"n", "e", "s", "w"};

	ExpandNoExpression(ng->exprs, ng->pos, "track_voxel");
	auto tv = std::make_shared<TrackVoxel>();

	Values vals("track_voxel", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _track_voxel_symbols);

	tv->dx = vals.GetNumber("dx");
	tv->dy = vals.GetNumber("dy");
	tv->dz = vals.GetNumber("dz");
	tv->flags = vals.GetNumber("flags");

	char buffer[16];
	for (int i = 0; i < 4; i++) {
		strcpy(buffer, direction[i]);
		strcpy(buffer + 1, "_back");
		if (vals.HasValue(buffer)) tv->back[i] = vals.GetSprite(buffer);
	}
	for (int i = 0; i < 4; i++) {
		strcpy(buffer, direction[i]);
		strcpy(buffer + 1, "_front");
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
	{nullptr, 0}
};

/**
 * Convert a 'connection' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'connection' node.
 */
static std::shared_ptr<Connection> ConvertConnection(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "connection");
	auto cn = std::make_shared<Connection>();

	Values vals("connection", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _connection_symbols);

	cn->name = vals.GetString("name");
	cn->direction = vals.GetNumber("direction");

	vals.VerifyUsage();
	return cn;
}

/** Available symbols for the track_piece node. */
static const Symbol _track_piece_symbols[] = {
	{"none",  0},
	{"left",  1},
	{"right", 2},
	{nullptr, 0}
};

/**
 * Convert a 'track_piece' game node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'track_piece' node.
 */
static std::shared_ptr<TrackPieceNode> ConvertTrackPieceNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "track_piece");
	auto tb = std::make_shared<TrackPieceNode>();

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

	tb->car_xpos = vals.GetCurve("car_xpos");
	tb->car_ypos = vals.GetCurve("car_ypos");
	tb->car_zpos = vals.GetCurve("car_zpos");
	tb->car_roll = vals.GetCurve("car_roll");
	tb->car_pitch = (vals.HasValue("car_pitch")) ? vals.GetCurve("car_pitch") : NULL;
	tb->car_yaw = (vals.HasValue("car_yaw")) ? vals.GetCurve("car_yaw") : NULL;

	tb->ComputeTrackLength(ng->pos);

	/* Unnamed values are track voxels that need to be claimed (partly) for the track piece. */
	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto tv = std::dynamic_pointer_cast<TrackVoxel>(vi->node_value);
		if (tv == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a \"track_voxel\" node\n", vi->pos.ToString());
			exit(1);
		}
		tb->track_voxels.push_back(tv);
		vi->node_value = nullptr;
		vi->used = true;
	}

	vals.VerifyUsage();
	return tb;
}

/**
 * Convert a 'RCST' node to a game block.
 * @param ng Generic tree of nodes to convert.
 * @return The converted RCST game block.
 */
static std::shared_ptr<RCSTBlock> ConvertRCSTNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "RCST");
	auto rb = std::make_shared<RCSTBlock>();

	Values vals("RCST", ng->pos);
	vals.PrepareNamedValues(ng->values, true, true);

	rb->coaster_type = vals.GetNumber("coaster_type");
	rb->platform_type = vals.GetNumber("platform_type");
	rb->number_trains = vals.GetNumber("max_number_trains");
	rb->number_cars = vals.GetNumber("max_number_cars");
	rb->text = std::make_shared<StringBundle>();
	rb->text->Fill(vals.GetStrings("texts"));
	rb->text->CheckTranslations(_coaster_string_names, lengthof(_coaster_string_names), ng->pos);

	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto tb = std::dynamic_pointer_cast<TrackPieceNode>(vi->node_value);
		if (tb == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a \"track_piece\" node\n", vi->pos.ToString());
			exit(1);
		}
		rb->track_blocks.push_back(tb);
		vi->node_value = nullptr;
		vi->used = true;
	}

	vals.VerifyUsage();
	return rb;
}

/**
 * Convert a 'CARS' node to a game block.
 * @param ng Generic tree of nodes to convert.
 * @return The converted CARS game block.
 */
static std::shared_ptr<CARSBlock> ConvertCARSNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "CARS");
	auto rb = std::make_shared<CARSBlock>();
	Values vals("CARS", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	rb->tile_width     = vals.GetNumber("tile_width");
	rb->z_height       = vals.GetNumber("z_height");
	rb->length         = vals.GetNumber("length");
	rb->inter_length   = vals.GetNumber("inter_length");
	rb->num_passengers = vals.GetNumber("num_passengers");
	rb->num_entrances  = vals.GetNumber("num_entrances");

	char buffer[32];
	for (int yaw = 0; yaw < 16; yaw++) {
		for (int roll = 0; roll < 16; roll++) {
			for (int pitch = 0; pitch < 16; pitch++) {
				int index = pitch + roll * 16 + yaw *16*16;
				sprintf(buffer, "car_p%dr%dy%d", pitch, roll, yaw);
				rb->sprites[index] = vals.GetSprite(buffer);
			}
		}
	}

	vals.VerifyUsage();
	return rb;
}

/**
 * Convert a 'splines' block to a node block.
 * @param ng Generic tree of nodes to convert.
 * @return The converted splines node block.
 */
static std::shared_ptr<BlockNode> ConvertSplinesNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, ng->name.c_str());
	auto blk = std::make_shared<CubicSplines>();
	Values vals(ng->name.c_str(), ng->pos);
	vals.PrepareNamedValues(ng->values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto cs = std::dynamic_pointer_cast<CubicSpline>(vi->node_value);
		if (cs == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a \"cubic\" node\n", vi->pos.ToString());
			exit(1);
		}
		blk->curve.push_back(cs);
		vi->node_value = nullptr;
		vi->used = true;
	}

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a 'cubic' node to a block node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted cubic spline block.
 */
static std::shared_ptr<BlockNode> ConvertCubicNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, ng->name.c_str());
	auto blk = std::make_shared<CubicSpline>();
	Values vals(ng->name.c_str(), ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	blk->a = vals.GetNumber("a");
	blk->b = vals.GetNumber("b");
	blk->c = vals.GetNumber("c");
	blk->d = vals.GetNumber("d");
	blk->steps = vals.GetNumber("steps");
	if (blk->steps < 100) {
		fprintf(stderr, "Error at %s: 'steps' should be at least 100.\n", ng->pos.ToString());
		exit(1);
	}

	vals.VerifyUsage();
	return blk;
}

/** Platform coaster type symbols. */
static const Symbol _coaster_platform_symbols[] = {
	{"wood", 1},
	{nullptr, 0}
};

/**
 * Convert a 'CSPL' node to a game block.
 * @param ng Generic tree of nodes to convert.
 * @return The converted CSPL game block.
 */
static std::shared_ptr<CSPLBlock> ConvertCSPLNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "CSPL");
	auto blk = std::make_shared<CSPLBlock>();
	Values vals("CSPL", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _coaster_platform_symbols);

	blk->tile_width  = vals.GetNumber("tile_width");
	blk->type        = vals.GetNumber("type");
	blk->ne_sw_back  = vals.GetSprite("ne_sw_back");
	blk->ne_sw_front = vals.GetSprite("ne_sw_front");
	blk->se_nw_back  = vals.GetSprite("se_nw_back");
	blk->se_nw_front = vals.GetSprite("se_nw_front");
	blk->sw_ne_back  = vals.GetSprite("sw_ne_back");
	blk->sw_ne_front = vals.GetSprite("sw_ne_front");
	blk->nw_se_back  = vals.GetSprite("nw_se_back");
	blk->nw_se_front = vals.GetSprite("nw_se_front");

	vals.VerifyUsage();
	return blk;
}

/** Fence symbols. */
static const Symbol _fence_symbols[] = {
	{"border", 1},
	{"wood", 2},
	{"conifer", 3},
	{"bricks", 4},
	{nullptr, 0}
};

/**
 * Convert a 'FENC' block.
 * @param ng Generic tree of nodes to convert.
 * @return The converted FENC game block.
 */
static std::shared_ptr<FENCBlock> ConvertFENCNode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "FENC");
	auto blk = std::make_shared<FENCBlock>();
	Values vals("FENC", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false, _fence_symbols);

	blk->tile_width = vals.GetNumber("width");
	blk->type       = vals.GetNumber("type");
	blk->ne_hor     = vals.GetSprite("ne_hor");
	blk->ne_n       = vals.GetSprite("ne_n");
	blk->ne_e       = vals.GetSprite("ne_e");
	blk->se_hor     = vals.GetSprite("se_hor");
	blk->se_e       = vals.GetSprite("se_e");
	blk->se_s       = vals.GetSprite("se_s");
	blk->sw_hor     = vals.GetSprite("sw_hor");
	blk->sw_s       = vals.GetSprite("sw_s");
	blk->sw_w       = vals.GetSprite("sw_w");
	blk->nw_hor     = vals.GetSprite("nw_hor");
	blk->nw_w       = vals.GetSprite("nw_w");
	blk->nw_n       = vals.GetSprite("nw_n");

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a 'INFO' block.
 * @param ng Generic tree of nodes to convert.
 * @return The converted INFO eta block.
 */
static std::shared_ptr<INFOBlock> ConvertINFONode(std::shared_ptr<NodeGroup> ng)
{
	ExpandNoExpression(ng->exprs, ng->pos, "INFO");
	auto blk = std::make_shared<INFOBlock>();
	Values vals("INFO", ng->pos);
	vals.PrepareNamedValues(ng->values, true, false);

	char buffer[32];
	time_t cur_time = time(nullptr);
	struct tm *now = gmtime(&cur_time);
	strftime(buffer, lengthof(buffer), "%Y%m%dT%H%M%S", now);
	blk->build = buffer;

	blk->name = std::string(vals.GetString("name"), 0, 63);
	blk->uri = std::string(vals.GetString("uri"), 0, 127);
	if (vals.HasValue("website")) {
		blk->website = std::string(vals.GetString("website"), 0, 127);
	}
	if (vals.HasValue("description")) {
		blk->description = std::string(vals.GetString("description"), 0, 511);
	}

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a node group.
 * @param ng Node group to convert.
 * @return The converted node.
 */
static std::shared_ptr<BlockNode> ConvertNodeGroup(std::shared_ptr<NodeGroup> ng)
{
	if (ng->name == "file") return ConvertFileNode(ng);
	if (ng->name == "sheet") return ConvertSheetNode(ng);
	if (ng->name == "sprite") return ConvertSpriteNode(ng);
	if (ng->name == "person_graphics") return ConvertPersonGraphicsNode(ng);
	if (ng->name == "recolour") return ConvertRecolourNode(ng);
	if (ng->name == "frame_data") return ConvertFrameDataNode(ng);
	if (ng->name == "strings") return ConvertStringsNode(ng);
	if (ng->name == "string") return ConvertStringNode(ng);
	if (ng->name == "track_voxel") return ConvertTrackVoxel(ng);
	if (ng->name == "connection") return ConvertConnection(ng);
	if (ng->name == "track_piece") return ConvertTrackPieceNode(ng);
	if (ng->name == "bitmask") return ConvertBitMaskNode(ng);
	if (ng->name == "splines") return ConvertSplinesNode(ng);
	if (ng->name == "cubic") return ConvertCubicNode(ng);

	/* Game blocks. */
	if (ng->name == "TSEL") return ConvertTSELNode(ng);
	if (ng->name == "TCOR") return ConvertTCORNode(ng);
	if (ng->name == "SURF") return ConvertSURFNode(ng);
	if (ng->name == "FUND") return ConvertFUNDNode(ng);
	if (ng->name == "PRSG") return ConvertPRSGNode(ng);
	if (ng->name == "ANIM") return ConvertANIMNode(ng);
	if (ng->name == "ANSP") return ConvertANSPNode(ng);
	if (ng->name == "PATH") return ConvertPATHNode(ng);
	if (ng->name == "PLAT") return ConvertPLATNode(ng);
	if (ng->name == "SUPP") return ConvertSUPPNode(ng);
	if (ng->name == "SHOP") return ConvertSHOPNode(ng);
	if (ng->name == "GBOR") return ConvertGBORNode(ng);
	if (ng->name == "GCHK") return ConvertGCHKNode(ng);
	if (ng->name == "GSLI") return ConvertGSLINode(ng);
	if (ng->name == "GSCL") return ConvertGSCLNode(ng);
	if (ng->name == "BDIR") return ConvertBDIRNode(ng);
	if (ng->name == "GSLP") return ConvertGSLPNode(ng);
	if (ng->name == "RCST") return ConvertRCSTNode(ng);
	if (ng->name == "CARS") return ConvertCARSNode(ng);
	if (ng->name == "CSPL") return ConvertCSPLNode(ng);
	if (ng->name == "FENC") return ConvertFENCNode(ng);
	if (ng->name == "INFO") return ConvertINFONode(ng);

	/* Unknown type of node. */
	fprintf(stderr, "Error at %s: Do not know how to check and simplify node \"%s\".\n", ng->pos.ToString(), ng->name.c_str());
	exit(1);
}

/**
 * Check and convert the tree to nodes.
 * @param values Root node of the tree.
 * @return The converted and checked sequence of file data to write.
 */
FileNodeList *CheckTree(std::shared_ptr<NamedValueList> values)
{
	assert(sizeof(_surface_sprite) / sizeof(_surface_sprite[0]) == SURFACE_COUNT);
	assert(sizeof(_foundation_sprite) / sizeof(_foundation_sprite[0]) == FOUNDATION_COUNT);

	FileNodeList *file_nodes = new FileNodeList;
	Values vals("root", Position("", 1));
	vals.PrepareNamedValues(values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		std::shared_ptr<ValueInformation> vi = vals.unnamed_values[i];
		if (vi->used) continue;
		auto fn = std::dynamic_pointer_cast<FileNode>(vi->node_value);
		if (fn == nullptr) {
			fprintf(stderr, "Error at %s: Node is not a file node\n", vi->pos.ToString());
			exit(1);
		}
		file_nodes->files.push_back(fn);
		vi->node_value = nullptr;
		vi->used = true;
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
	const char **names = nullptr;
	const char *nice_name = nullptr;
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
	if (fp == nullptr) {
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
	const char **names = nullptr;
	const char *nice_name = nullptr;
	const char *lower_name = nullptr;
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
	if (fp == nullptr) {
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
	fprintf(fp, "\tnullptr,\n");
	fprintf(fp, "};\n");
	fclose(fp);
}
