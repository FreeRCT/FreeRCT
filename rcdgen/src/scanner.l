%{
#include "scanner_funcs.h"
#include "tokens.h"

int line = 1;
std::string filename;
char *text = NULL;
int text_size = 0;
int text_index;
int string_line; ///< Starting line of the string.
%}

%x in_string
%x in_comment
%x in_line_comment

%option noyywrap

%%

"," {
	return COMMA;
}

"-" {
	yylval.line = line;
	return MINUS;
}

"(" {
	return PAR_OPEN;
}

")" {
	return PAR_CLOSE;
}

":" {
	return COLON;
}

"|" {
	return PIPE;
}

";" {
	return SEMICOLON;
}

"{" {
	return CURLY_OPEN;
}

"}" {
	return CURLY_CLOSE;
}

\" {
	text_index = 0;
	string_line = line;
	BEGIN(in_string);
}

<in_string>\" {
	BEGIN(INITIAL);
	AddChar('\0');
	yylval.chars.line = string_line;
	int length = strlen(text);
	yylval.chars.value = (char *)malloc(length + 1);
	memcpy(yylval.chars.value, text, length + 1);
	return STRING;
}

<in_string>. {
	AddChar(yytext[0]);
}

<in_string>\\\\ {
	AddChar('\\');
}

<in_string>\\\" {
	AddChar('\"');
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

[A-Za-z_][A-Za-z0-9#_]* {
	yylval.chars.line = line;
	int length = strlen(yytext);
	yylval.chars.value = (char *)malloc(length + 1);
	memcpy(yylval.chars.value, yytext, length + 1);
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

[ \t] {
}

\n {
	line++;
}

. {
	fprintf(stderr, "Unrecognized character encountered at line %d\n", line);
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

/**
 * Setup the scanner for the new file.
 * @param fname Name of the file being parsed.
 * @param new_file New input stream to switch to.
 */
void SetupScanner(const char *fname, FILE *new_file)
{
	yyrestart(new_file);
	BEGIN(INITIAL);

	filename = (fname == NULL) ? "<stdin>" : fname;
	line = 1;
}
