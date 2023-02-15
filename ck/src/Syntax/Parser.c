#include <include/Syntax/Parser.h>
#include <ckmem/CDebug.h>
#include <string.h>

void CkParserCreateInstance(
	CkArenaFrame *arena,
	CkArenaFrame *genArena,
	CkParserInstance *dest,
	CkList *pPassedTokens,
	size_t passedCount,
	CkDiagnosticHandlerInstance *pDhi )
{
	CK_ARG_NON_NULL( arena );
	CK_ARG_NON_NULL( genArena );
	CK_ARG_NON_NULL( dest );
	CK_ARG_NON_NULL( pPassedTokens );
	CK_ARG_NON_NULL( pDhi );
	dest->position = 0;
	dest->passedTokenCount = passedCount;
	dest->pPassedTokens = pPassedTokens;
	dest->pDhi = pDhi;
	dest->arena = arena;
	dest->genArena = genArena;
}

void CkParserDelete( CkParserInstance *dest )
{
	(void)dest;
}

void CkParserReadToken( CkParserInstance *parser, CkToken *token )
{
	CkToken *source;

	CK_ARG_NON_NULL( parser );
	CK_ARG_NON_NULL( token );

	// A EOF token will be returned if the parser tries
	// to read a non-existing token.
	if ( parser->position >= parser->passedTokenCount ) {
		token->kind = 0;
		token->position = parser->position;
		token->value.u64 = 0;
		return;
	}

	source = CkListAccess( parser->pPassedTokens, parser->position );
	memcpy( token, source, sizeof( CkToken ) );
	parser->position++;
}

bool_t CkParserRewind( CkParserInstance *parser, size_t elems )
{
	CK_ARG_NON_NULL( parser );
	// If the amount of tokens cannot be rewinded, the 
	// parser is set back to zero and false is returned.
	if ( elems > parser->position ) {
		parser->position = 0;
		return FALSE;
	}

	parser->position -= elems;
	return TRUE;
}
