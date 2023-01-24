#include <include/Syntax/Parser.h>
#include <string.h>

void CkParserCreateInstance(CkParserInstance *dest, CkToken *pPassedTokens, size_t passedCount)
{
	dest->position = 0;
	dest->passedTokenCount = passedCount;
	dest->pPassedTokens = pPassedTokens;
}

void CkParserDelete(CkParserInstance *dest)
{
	memset(dest, 0, sizeof(CkParserInstance));
}

void CkParserReadToken(CkParserInstance *parser, CkToken *token)
{
	// A EOF token will be returned if the parser tries
	// to read a non-existing token.
	if (parser->position >= parser->passedTokenCount) {
		token->kind = 0;
		token->position = parser->position;
		token->value.u64 = 0;
		return;
	}

	memcpy_s(token, sizeof(CkToken), parser->pPassedTokens + parser->position, sizeof(CkToken));
	parser->position++;
}

bool_t CkParserRewind(CkParserInstance *parser, size_t elems)
{
	// If the amount of tokens cannot be rewinded, the 
	// parser is set back to zero and false is returned.
	if (elems > parser->position) {
		parser->position = 0;
		return FALSE;
	}

	parser->position -= elems;
	return TRUE;
}
