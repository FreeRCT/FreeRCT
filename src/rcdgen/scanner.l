%{
#include "scanner_funcs.h"
#include "tokens.h"

#if defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wdeprecated"
#endif

int line = 1;
std::string filename;
std::string text;
int string_line; ///< Starting line of the string.
%}

%x in_string
%x in_comment
%x in_line_comment

%option noyywrap

%option nounput
%option noinput

%option never-interactive 
%option nounistd

%%

"-"  { yylval.line = line; return MINUS; }
"+"  { yylval.line = line; return ADD; }
"&"  { yylval.line = line; return AND; }
"|"  { yylval.line = line; return PIPE; }
"^"  { yylval.line = line; return XOR; }
"<<" { yylval.line = line; return SHL; }
">>" { yylval.line = line; return SHR; }
"*"  { yylval.line = line; return MUL; }
"~"  { yylval.line = line; return NEG; }
"/"  { yylval.line = line; return DIV; }
"%"  { yylval.line = line; return MOD; }

"," { return COMMA; }
"(" { return PAR_OPEN; }
")" { return PAR_CLOSE; }
":" { return COLON; }
";" { return SEMICOLON; }
"{" { return CURLY_OPEN; }
"}" { return CURLY_CLOSE; }

\" {
	text.clear();
	string_line = line;
	BEGIN(in_string);
}

<in_string>\" {
	BEGIN(INITIAL);
	yylval.chars.line = yylval.line = string_line;
	yylval.chars.value = text;
	return STRING;
}

<in_string>. {
	text += yytext[0];
}

<in_string>\\\\ {
	text += '\\';
}

<in_string>\\\" {
	text += '\"';
}

0 {
	yylval.number.line = line;
	yylval.number.value = 0;
	return NUMBER;
}

[1-9][0-9]* {
	yylval.number.line = line;
	yylval.number.value = atoll(yytext);
	return NUMBER;
}

"bitset" {
	yylval.line = line;
	return BITSET_KW;
}

"import" {
	yylval.line = line;
	return IMPORT_KW;
}

[A-Za-z_]([A-Za-z0-9#_]|[{](hor|vert)[(][0-9][0-9]*\.\.[0-9][0-9]*[)][}])* {
	/* a letter, followed by 0 or more times a letter or digit, or {hor/vert(num..num)}. */
	yylval.chars.line = yylval.line = line;
	yylval.chars.value = yytext;
	return IDENTIFIER;
}

"/*" {
	BEGIN(in_comment);
}

<in_comment>"*/" {
	BEGIN(INITIAL);
}

<in_comment>. {
}

<in_comment>\n {
	line++;
}

"//" {
	BEGIN(in_line_comment);
}

<in_line_comment>. {
}

<in_line_comment>\n {
	BEGIN(INITIAL);
	line++;
}

[ \t\r] {
}

\n {
	line++;
}

. {
	fprintf(stderr, "Unrecognized character 0x%02x encountered at line %d\n", yytext[0], line);
	exit(1);
}

<in_comment><<EOF>> {
	BEGIN(INITIAL);
}
<in_string><<EOF>> {
	BEGIN(INITIAL);
}
<in_line_comment><<EOF>> {
	BEGIN(INITIAL);
}

%%

/* Documentation is in scanner_funcs.h, since this file is not being scanned by Doxygen. */
void SetupScanner(const char *fname, FILE *new_file)
{
	yyrestart(new_file == nullptr ? stdin : new_file);
	BEGIN(INITIAL);

	filename = (fname == nullptr) ? "<stdin>" : fname;
	line = 1;
}

#if defined(__clang__)
	#pragma GCC diagnostic pop /* "-Wdeprecated" */
#endif
