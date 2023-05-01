#include <Syntax/Preprocessor.h>
#include <Syntax/Lex.h>

#include <string.h>

static void _ExpandIf( CkPreprocessor *preprocessor, CkPreprocessorIf *branch )
{
	size_t macro_count = CkListLength( preprocessor->macros );

	if ( branch->condition == NULL ) {
		CkListAddRange( preprocessor->output, branch->expands );
		return;
	}
	if ( !branch->negative ) {
		for ( size_t j = 0; j < macro_count; j++ ) {
			CkMacro *p_macro = (CkMacro *)CkListAccess( preprocessor->macros, j );
			if ( !strcmp( p_macro->name, branch->condition ) ) {
				CkListAddRange( preprocessor->output, branch->expands );
				return;
			}
		}
	} else {
		for ( size_t j = 0; j < macro_count; j++ ) {
			CkMacro *p_macro = (CkMacro *)CkListAccess( preprocessor->macros, j );
			if ( !strcmp( p_macro->name, branch->condition ) )
				goto LElse;
		}
		CkListAddRange( preprocessor->output, branch->expands );
		return;
	}
LElse:
	if ( branch->elseBranch) _ExpandIf( preprocessor, branch->elseBranch );
}

size_t CkPreprocessorExpand( CkArenaFrame *allocator, CkPreprocessor *preprocessor )
{
	size_t len = CkListLength( preprocessor->input );
	size_t expansions = 0;
	preprocessor->output = CkListStart( allocator, sizeof( CkToken ) );
	for ( size_t i = 0; i < len; i++ ) {
		CkToken *p_token = (CkToken *)CkListAccess( preprocessor->input, i );
		size_t macro_count = CkListLength( preprocessor->macros );

		if ( p_token->kind == PP_DIRECTIVE_DEFINE ) {
			CkMacro *new = (CkMacro *)p_token->value.ptr;
			bool_t all_good = TRUE;
			for ( size_t j = 0; j < macro_count; j++ ) {
				CkMacro *p_macro = (CkMacro *)CkListAccess( preprocessor->macros, j );
				if ( !strcmp( p_macro->name, new->name ) ) {
					CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
						"Macro '%s' was already defined", p_macro->name );
					preprocessor->errors = TRUE;
					all_good = FALSE;
					break;
				}
			}
			if ( all_good ) CkListAdd( preprocessor->macros, new );
		} else if ( p_token->kind == PP_DIRECTIVE_UNDEFINE ) {
			for ( size_t j = 0; j < macro_count; j++ ) {
				CkMacro *p_macro = (CkMacro *)CkListAccess( preprocessor->macros, j );
				if ( !strcmp( p_macro->name, p_token->value.cstr ) ) {
					CkListRemove( preprocessor->macros, j );
					break;
				}
			}
			// No errors if previously not defined, intended behaviour
		} else if ( p_token->kind == PP_DIRECTIVE_IFDEF || p_token->kind == PP_DIRECTIVE_IFNDEF )
			_ExpandIf( preprocessor, (CkPreprocessorIf *)p_token->value.ptr );
		else if ( p_token->kind == PP_DIRECTIVE_ELIFDEF || p_token->kind == PP_DIRECTIVE_ELIFNDEF || p_token->kind == PP_DIRECTIVE_ELSE ) {
			CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
				"Headless else directive" );
			preprocessor->errors = TRUE;
		} else if ( p_token->kind == PP_DIRECTIVE_MESSAGE )
			CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_MESSAGE, "user-message",
				"%s", p_token->value.cstr );
		else if ( p_token->kind == PP_DIRECTIVE_WARNING )
			CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_WARNING, "user-warning",
				"%s", p_token->value.cstr );
		else if ( p_token->kind == PP_DIRECTIVE_ERROR )
			CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
				"%s", p_token->value.cstr );
		else if ( p_token->kind == PP_DIRECTIVE_UNKNOWN || p_token->kind == PP_DIRECTIVE_MALFORMED )
			continue;
		else if ( p_token->kind == 'I' ) {
			CkMacro *expand_target = NULL;
			for ( size_t j = 0; j < macro_count; j++ ) {
				CkMacro *p_macro = (CkMacro *)CkListAccess( preprocessor->macros, j );
				if ( !strcmp( p_macro->name, p_token->value.cstr ) ) {
					expand_target = p_macro;
				}
			}
			if ( expand_target == NULL ) { CkListAdd( preprocessor->output, p_token ); continue; }
			// ----- Macro expansion -----
			if ( expand_target->argcount == 0 ) {
				CkListAddRange( preprocessor->output, expand_target->expands );
				expansions++;
			} else {
				CkToken *p_current;
				CkList *p_arguments;
				size_t expand_count;
				if ( i + 1 >= len ) {
					CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
						"Insufficient parameters for macro expansion" );
					preprocessor->errors = TRUE;
					goto LContinueParentLoop;
				}
				p_current = (CkToken *)CkListAccess( preprocessor->input, ++i );
				if ( p_current->kind != '(' ) {
					CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
						"Expected opening bracket (" );
					preprocessor->errors = TRUE;
					goto LContinueParentLoop;
				}
				p_arguments = CkListStart( allocator, sizeof( CkList * ) );
				while ( TRUE ) {
					CkList *p_arg = CkListStart( allocator, sizeof( CkToken ) );

					if ( i + 1 >= len ) {
						CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
							"Insufficient parameters for macro expansion" );
						preprocessor->errors = TRUE;
						goto LContinueParentLoop;
					}

					p_current = (CkToken *)CkListAccess( preprocessor->input, ++i );
					if ( p_current->kind != '$' ) {
						CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
							"Expected macro expansion block `$<tokens>$`" );
						preprocessor->errors = TRUE;
						goto LContinueParentLoop;
					}

					while ( TRUE ) {
						if ( i + 1 >= len ) {
							CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
								"Expected macro expansion block terminator $" );
							preprocessor->errors = TRUE;
							goto LContinueParentLoop;
						}
						p_current = (CkToken *)CkListAccess( preprocessor->input, ++i );
						if ( p_current->kind != '$' && p_current->kind != 0 ) {
							CkListAdd( p_arg, p_current );
							continue;
						}
						break;
					}
					if ( i + 1 >= len ) {
						CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
							"Expected comma , or closing bracket )" );
						preprocessor->errors = TRUE;
						goto LContinueParentLoop;
					}
					p_current = (CkToken *)CkListAccess( preprocessor->input, ++i );
					CkListAdd( p_arguments, &p_arg );
					if ( p_current->kind == ')' ) break;
					else if ( p_current->kind == ',' );
					else {
						CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
							"Expected comma , or closing bracket )" );
						preprocessor->errors = TRUE;
						goto LContinueParentLoop;
					}
				}

				if ( CkListLength( p_arguments ) != expand_target->argcount ) {
					CkDiagnosticThrow( preprocessor->pDhi, p_token, CK_DIAG_SEVERITY_ERROR, "",
						"Expected %zu macro arguments, got %zu", expand_target->argcount, CkListLength( p_arguments ) );
					preprocessor->errors = TRUE;
					goto LContinueParentLoop;
				}

				// ----- Replace wildcards by arguments & generating -----
				expand_count = CkListLength( expand_target->expands );
				for ( size_t j = 0; j < expand_count; j++ ) {
					CkToken *c_expand = CkListAccess( expand_target->expands, j );
					if ( c_expand->kind == PP_MACRO_WILDCARD ) {
						CkListAddRange(
							preprocessor->output,
							*(CkList **)CkListAccess(p_arguments, c_expand->value.u64)
							);
					} else CkListAdd( preprocessor->output, c_expand );
				}
			}
		} else {
			CkListAdd( preprocessor->output, p_token );
		}
	LContinueParentLoop:;
	}
	return expansions;
}
