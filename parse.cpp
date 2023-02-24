// TODO: use newid and not new id in the right places
// TODO: factor out switches in expressions

#include "parse.hpp"
#include "lex.hpp"
#include "error.hpp"

ASTNode Expression();
ASTNode Block();
ASTNode UnaryExpr();
ASTNode Statement();

Tokinfo expect( TokenID type ) {
    auto tok = lex();

    if ( tok.token != type )
        error( tok.linenum, "expected token of type %s", token_to_string( type ) );

    return tok;
}

void check_for_bad_semicolon() {
    // check if we inserted semicolon for a newline for more specific error message
    auto tok = lex();
    unlex( tok );
    if ( tok.token != TOKEN_SEMICOLON ) return;

    if ( tok.lexeme == "" )
        error( tok.linenum, "unexpected newline"  );
    else
        error( tok.linenum, "syntax error on \";\"" );
}

ASTNode newidentifier() {
    auto tok = expect( TOKEN_ID );

    return ASTNode ( AST_NEWID, tok.linenum, tok.lexeme );
}

ASTNode typeidentifier() {
    auto tok = expect( TOKEN_ID );

    return ASTNode ( AST_TYPEID, tok.linenum, tok.lexeme );
}

ASTNode BreakStmt() {
    auto tok = expect( TOKEN_BREAK );

    return ASTNode ( AST_BREAK, tok.linenum, tok.lexeme );
}

ASTNode ReturnStmt() {
    auto tok = expect( TOKEN_RETURN );

    return ASTNode ( AST_RETURN, tok.linenum, tok.lexeme );
}

ASTNode IfStmt() {
    auto tok = expect( TOKEN_IF );

    ASTNode result( AST_IF, tok.linenum, tok.lexeme );

    {
        auto expr = Expression();
        result.add_child( expr );
    }

    check_for_bad_semicolon();

    {
        auto blk = Block();
        result.add_child( blk );
    }

    tok = lex();

    if ( tok.token != TOKEN_ELSE ) {
        unlex( tok );
        return result;
    }

    result.type = AST_IFELSE;

    if ( tok.token == TOKEN_LBRACE ) {
        auto blk = Block();
        result.add_child( blk );
    } else if ( tok.token == TOKEN_IF ) {
        auto ifstmt = IfStmt();
        result.add_child( ifstmt );
    } else {
        // TODO: make decent error
        error( tok.linenum, "" );
    }

    return result;
}

ASTNode ForStmt() {
    auto tok = expect( TOKEN_FOR );

    ASTNode result( AST_FOR, tok.linenum );

    // [ Condition ]
    tok = lex();
    unlex( tok );
    if ( tok.token != TOKEN_LBRACE ) {
        auto expr = Expression();
        result.add_child( expr );
    } else result.add_child( { AST_ID, -1, "$true" } );

    tok = lex();
    unlex( tok );

    check_for_bad_semicolon();

    auto blk = Block();
    result.add_child( blk );

    return result;
}

ASTNode PL5() {
    ASTNode result = UnaryExpr();

    Tokinfo tok;
    while ( ( tok = lex() ).token == TOKEN_STAR || tok.token == TOKEN_SLASH || tok.token == TOKEN_PERCENT ) {
        auto right = UnaryExpr();
        auto left = result;
        result = ASTNode ( AST_UNSET, tok.linenum, tok.lexeme );
        if ( tok.token == TOKEN_STAR ) result.type = AST_STAR;
        if ( tok.token == TOKEN_SLASH ) result.type = AST_SLASH;
        if ( tok.token == TOKEN_PERCENT ) result.type = AST_PERCENT;

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode PL4() {
    ASTNode result = PL5();

    Tokinfo tok;
    while ( ( tok = lex() ).token == TOKEN_PLUS || tok.token == TOKEN_MINUS ) {
        auto right = PL5();
        auto left = result;
        result = ASTNode ( AST_UNSET, tok.linenum, tok.lexeme );
        if ( tok.token == TOKEN_PLUS ) result.type = AST_PLUS;
        if ( tok.token == TOKEN_MINUS ) result.type = AST_MINUS;

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode PL3() {
    ASTNode result = PL4();

    Tokinfo tok;
    while ( ( tok = lex() )
                  .token == TOKEN_EQ
            || tok.token == TOKEN_NEQ
            || tok.token == TOKEN_GEQ
            || tok.token == TOKEN_LEQ
            || tok.token == TOKEN_LT
            || tok.token == TOKEN_GT) {
        auto right = PL4();
        auto left = result;
        result = ASTNode ( AST_UNSET, tok.linenum, tok.lexeme );
        switch ( tok.token ) {
            case TOKEN_EQ:
                result.type = AST_EQ;
                break;
            case TOKEN_NEQ:
                result.type = AST_NEQ;
                break;
            case TOKEN_GEQ:
                result.type = AST_GEQ;
                break;
            case TOKEN_LEQ:
                result.type = AST_LEQ;
                break;
            case TOKEN_LT:
                result.type = AST_LT;
                break;
            case TOKEN_GT:
                result.type = AST_NEQ;
                break;
            default:
                // TODO: FIX THIS
                error( tok.linenum, "unreachable" );
        }

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode PL2() {
    ASTNode result = PL3();

    Tokinfo tok;
    while ( ( tok = lex() ).token == TOKEN_LOGIC_AND ) {
        auto right = PL3();
        auto left = result;
        result = ASTNode ( AST_LOGIC_AND, tok.linenum, tok.lexeme );

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode PL1() {
    ASTNode result = PL2();

    Tokinfo tok;
    while ( ( tok = lex() ).token == TOKEN_LOGIC_OR ) {
        auto right = PL2();
        auto left = result;
        result = ASTNode ( AST_LOGIC_OR, tok.linenum, tok.lexeme );

        result.add_child( left );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}


ASTNode Expression() {
    return PL1();
}

ASTNode Block() {
    expect( TOKEN_LBRACE );

    ASTNode result;
    result.type = AST_BLOCK;

    Tokinfo tok;
    while ( ( tok = lex() ).token != TOKEN_RBRACE ) {
        unlex( tok );
        auto stmt = Statement();
        result.add_child( stmt );
        expect( TOKEN_SEMICOLON );
    }
    unlex( tok );

    expect( TOKEN_RBRACE );

    return result;
}

// "formal" in the reference compiler I think
ASTNode ParameterDecl() {
    ASTNode result ( AST_FORMAL );

    auto newid = newidentifier();
    result.add_child( newid );

    auto typeident = typeidentifier();
    result.add_child( typeident );

    return result;
}

ASTNode VarDecl() {
    auto tok = expect( TOKEN_VAR );

    ASTNode result ( AST_VAR, tok.linenum, tok.lexeme );

    auto newid = newidentifier();
    result.add_child( newid );

    auto typeident = typeidentifier();
    result.add_child( typeident );

    return result;
}

ASTNode FunctionDecl() {
    auto tok = expect( TOKEN_FUNC );

    ASTNode result ( AST_FUNC, tok.linenum, tok.lexeme );

    {
        auto ident = newidentifier();
        result.add_child( ident ); // FunctionName
    }

    expect( TOKEN_LPAREN );

    // add signature
    ASTNode sig ( AST_SIGNATURE );
    ASTNode formals ( AST_FORMALS );

    tok = lex();
    unlex( tok );

    if ( tok.token == TOKEN_ID ) {
        // ParameterList

        auto params = ParameterDecl();
        formals.add_child( params );

        while ( ( tok = lex() ).token == TOKEN_COMMA ) {
            params = ParameterDecl();
            formals.add_child( params );
        }
        unlex( tok );

        check_for_bad_semicolon();

        // consume trailing comma in params list if there is one
        if ( ( tok = lex() ).token != TOKEN_COMMA ) unlex( tok );
    }

    expect( TOKEN_RPAREN );

    sig.add_child( formals );

    tok = lex();
    unlex( tok );

    if ( tok.token == TOKEN_ID ) {
        auto tident = typeidentifier();
        sig.add_child( tident );
    } else {
        ASTNode inserted_void( AST_TYPEID, -1, "$void" );

        sig.add_child( inserted_void );
    }

    result.add_child( sig );

    check_for_bad_semicolon();

    auto blk = Block();
    result.add_child( blk ); // FunctionBody

    return result;
}

ASTNode TopLevelDecl() {
    auto tok = lex();
    unlex( tok );

    ASTNode result;

    switch ( tok.token ) {
        case TOKEN_FUNC:
            result = FunctionDecl();
            break;
        case TOKEN_VAR:
            result = VarDecl();
            break;
        default:
            printf("current token is: %s\n", tok.lexeme.data());
            error( tok.linenum, "top level declarations must begin with 'func' or 'var'" );
    }

    return result;
}

ASTNode Operand() {
    auto tok = lex();

    switch ( tok.token ) {
        case TOKEN_INT:
            return ASTNode ( AST_INT, tok.linenum, tok.lexeme );
        case TOKEN_STRING:
            return ASTNode ( AST_STRING, tok.linenum, tok.lexeme );
        case TOKEN_ID:
            return ASTNode ( AST_ID, tok.linenum, tok.lexeme );
        default: {

            unlex( tok );
            expect( TOKEN_LPAREN );
            auto result = Expression();
            expect( TOKEN_RPAREN );

            return result;
        }
    }
}

bool is_expression_next() {
    auto tok = lex();
    unlex( tok );

    return tok.token == TOKEN_BANG
        || tok.token == TOKEN_MINUS
        || tok.token == TOKEN_INT
        || tok.token == TOKEN_STRING
        || tok.token == TOKEN_ID
        || tok.token == TOKEN_LPAREN
    ;
}

ASTNode Arguments() {
    expect( TOKEN_LPAREN );
    ASTNode result = { AST_ACTUALS };

    if ( is_expression_next() ) {
        auto expr = Expression();
        result.add_child( expr );

        Tokinfo tok;
        while ( ( tok = lex() ).token == TOKEN_COMMA ){
            expr = Expression();
            result.add_child( expr );
        }
        unlex( tok );

        check_for_bad_semicolon();
        // consume trailing comma in params list if there is one
        if ( ( tok = lex() ).token != TOKEN_COMMA ) unlex( tok );
    }

    expect( TOKEN_RPAREN );

    return result;
}

ASTNode Assignment() {
    ASTNode result = { AST_ASSIGN };

    auto expr = Expression();
    result.add_child( expr );

    result.linenum = expect( TOKEN_ASSIGN ).linenum;

    expr = Expression();
    result.add_child( expr );

    return result;
}

ASTNode Statement() {
    auto tok = lex();
    unlex( tok );

    switch ( tok.token ) {
        case TOKEN_VAR:
            return VarDecl();
        case TOKEN_RETURN:
            return ReturnStmt();
        case TOKEN_BREAK:
            return BreakStmt();
        case TOKEN_LBRACE:
            return Block();
        case TOKEN_IF:
            return IfStmt();
        case TOKEN_FOR:
            return ForStmt();
        default:
            if ( !is_expression_next() ) return { AST_EMPTYSTMT };

            auto result = Expression();

            if ( ( tok = lex() ).token == TOKEN_ASSIGN ) {
                auto right = Expression();

                auto left = result;

                result = ASTNode ( AST_ASSIGN, tok.linenum, tok.lexeme );
                result.add_child( left );
                result.add_child( right );

                if ( ( tok = lex() ).token == TOKEN_ASSIGN ) error( tok.linenum, "syntax error on \"=\"" );
            } else {
                auto tmp = result;
                result   = ASTNode( AST_EXPRSTMT );
                result.add_child( tmp );
            }

            unlex( tok );

            return result;
    }
}

ASTNode UnaryExpr() {
    auto tok = lex();

    ASTNode result;

    switch ( tok.token ) {
        case TOKEN_BANG: {
            result.type = AST_LOGIC_NOT;
            result.linenum = tok.linenum;

            auto exp = UnaryExpr();
            result.add_child( exp );

            break;
        }
        case TOKEN_MINUS: {
                result.type = AST_UMINUS;
                result.linenum = tok.linenum;

                auto exp = UnaryExpr();
                result.add_child( exp );

            break;
        }
        default:
            unlex( tok );

            result = Operand();

            while ( ( tok = lex() ).token == TOKEN_LPAREN ) {
                unlex( tok );

                auto args = Arguments();
                result.add_child( args );
            }
            unlex( tok );
            break;
    }

    return result;
}


// SourceFile
ASTNode parse() {
    ASTNode root( AST_PROGRAM );

    Tokinfo tok;

    while ( ( tok = lex() ).token != TOKEN_EOF ) {
        unlex( tok );
        auto decl = TopLevelDecl();
        root.add_child( decl );

        expect( TOKEN_SEMICOLON );
    }
    unlex( tok );

    return root;
}

// it's 11 at night and i've been coding since 7 am. forgive me
std::string ASTNode_to_string( ASTNode n ) {
    std::string numstring;
    if ( n.linenum > 0 ) numstring = "@ line " + std::to_string( n.linenum );

    std::string lexstring;
    if ( n.lexeme.size() > 0) lexstring = "[" + n.lexeme + "] ";

    switch ( n.type ) {
        case AST_PROGRAM:   return "program";
        case AST_FUNC:      return "func " + numstring;
        case AST_NEWID:     return "newid " + lexstring + numstring;
        case AST_SIGNATURE: return "sig";
        case AST_TYPEID:    return "typeid " + lexstring + numstring;
        case AST_BLOCK:     return "block";
        case AST_FOR:       return "for " + numstring;
        case AST_ID:        return "id " + lexstring + numstring;
        case AST_EMPTYSTMT: return "emptystmt";
        case AST_EXPRSTMT:  return "exprstmt";
        case AST_IF:        return "if " + numstring;
        case AST_EQ:        return "== "  + numstring;
        case AST_PLUS:      return "+ "   + numstring;
        case AST_INT:       return "int " + lexstring + numstring;
        case AST_ASSIGN:    return "= " + numstring;
        case AST_STAR:      return "* " + numstring;
        case AST_FORMAL:    return "formal";
        case AST_FORMALS:   return "formals";
        case AST_ACTUALS:   return "actuals";
        case AST_STRING:    return "string " + lexstring + numstring;
        default:
            printf( "thing is: %d '%s'\n", n.type, n.lexeme.data() );
            return "foo";
    }

    // TODO: kill
    return "bar";
}
