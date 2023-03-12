#include "parse.hpp"
#include "lex.hpp"
#include "error.hpp"

ASTNode Expression();
ASTNode Statement();

// when we know what token should come next, use this
// it throws an error if the expected token isn't next
Tokinfo expect( TokenID type ) {
    auto tok = lex();

    if ( tok.token != type )
        error( tok.linenum, "expected token of type %s", token_to_string( type ) );

    return tok;
}

// check if the next token could start an expression
bool is_expression_next() {
    auto tok = lex();
    unlex( tok );

    return tok.token == TOKEN_BANG
        || tok.token == TOKEN_MINUS
        || tok.token == TOKEN_INT
        || tok.token == TOKEN_STRING
        || tok.token == TOKEN_ID
        || tok.token == TOKEN_LPAREN;
}

// there are certain places where semicolon insertion makes putting a newline
// there illegal cause a semicolon gets inserted. this function checks that
void check_for_bad_semicolon() {
    auto tok = lex();
    unlex( tok );
    if ( tok.token != TOKEN_SEMICOLON ) return;

    if ( tok.lexeme == "" )
        error( tok.linenum, "unexpected newline"    );
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

ASTNode Operand() {
    auto tok = lex();

    switch ( tok.token ) {
        case TOKEN_INT:    return ASTNode ( AST_INT,    tok.linenum, tok.lexeme );
        case TOKEN_STRING: return ASTNode ( AST_STRING, tok.linenum, tok.lexeme );
        case TOKEN_ID:     return ASTNode ( AST_ID,     tok.linenum, tok.lexeme );

        default: {
            unlex ( tok          );
            expect( TOKEN_LPAREN );

            auto result = Expression();

            expect( TOKEN_RPAREN );

            return result;
        }
    }
}

ASTNode Arguments() {
    expect( TOKEN_LPAREN );

    ASTNode result = { AST_ACTUALS };

    // ExpressionList - at least one expression, but possibly more, all delimited by commas
    if ( is_expression_next() ) {
        auto expr = Expression();
        result.add_child( expr );

        Tokinfo tok;
        while ( ( tok = lex() ).token == TOKEN_COMMA && is_expression_next() ) {
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

ASTNode UnaryExpr() {
    auto tok = lex();

    ASTNode result;

    switch ( tok.token ) {
        // logical not
        case TOKEN_BANG: {
            result   = ASTNode ( AST_LOGIC_NOT, tok.linenum );

            auto exp = UnaryExpr();
            result.add_child( exp );

            break;
        }
        // unary minus aka "take the negative"
        case TOKEN_MINUS: {
            result = ASTNode( AST_UMINUS, tok.linenum );

            auto exp = UnaryExpr();

            if ( exp.type == AST_INT && exp.lexeme[0] != '-' ) {
                result = exp;
                result.lexeme = "-" + result.lexeme;
            } else
                result.add_child( exp );

            break;
        }
        default:
            unlex( tok );

            result = Operand();

            while ( ( tok = lex() ).token == TOKEN_LPAREN ) {
                unlex( tok );

                auto left  = result;
                result     = ASTNode( AST_FUNCCALL );
                auto right = Arguments();

                result.add_child( left );
                result.add_child( right );
            }

            unlex( tok );
            break;
    }

    return result;
}

ASTNode MulExpr() {
    ASTNode result = UnaryExpr();

    Tokinfo tok;
    // children are unary expressions, multiplied
    while ( ( tok = lex() )
                  .token == TOKEN_STAR
            || tok.token == TOKEN_SLASH
            || tok.token == TOKEN_PERCENT ) {
        auto right = UnaryExpr();
        auto left  = result;

        result = ASTNode ( AST_UNSET, tok.linenum, tok.lexeme );
        if ( tok.token == TOKEN_STAR    ) result.type = AST_MUL;
        if ( tok.token == TOKEN_SLASH   ) result.type = AST_DIV;
        if ( tok.token == TOKEN_PERCENT ) result.type = AST_MOD;

        result.add_child( left  );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode AddExpr() {
    ASTNode result = MulExpr();

    Tokinfo tok;
    // children are Mul expressions, added to each other
    while ( ( tok = lex() ).token == TOKEN_PLUS || tok.token == TOKEN_MINUS ) {
        auto right = MulExpr();
        auto left  = result;

        result = ASTNode ( AST_UNSET, tok.linenum, tok.lexeme );
        if ( tok.token == TOKEN_PLUS  ) result.type = AST_PLUS;
        if ( tok.token == TOKEN_MINUS ) result.type = AST_MINUS;

        result.add_child( left  );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode RelExpr() {
    ASTNode result = AddExpr();

    Tokinfo tok;
    // children are Add expressions, compared to each other
    while ( ( tok = lex() )
                  .token == TOKEN_EQ
            || tok.token == TOKEN_NEQ
            || tok.token == TOKEN_GEQ
            || tok.token == TOKEN_LEQ
            || tok.token == TOKEN_LT
            || tok.token == TOKEN_GT ) {

        auto right = AddExpr();
        auto left  = result;

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
                // NOTE: this is actually unreachable:
                // the condition for the loop enforces that tok.token
                // is one of the six cases I check here, which can be
                // easily verified by inspection
                error( tok.linenum, "unreachable" );
        }

        result.add_child( left  );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode AndExpr() {
    ASTNode result = RelExpr();

    Tokinfo tok;
    // children are relation/comparison Expressions, OR-ed together
    while ( ( tok = lex() ).token == TOKEN_LOGIC_AND ) {
        auto right = RelExpr();
        auto left  = result;
        result     = ASTNode ( AST_LOGIC_AND, tok.linenum, tok.lexeme );

        result.add_child( left  );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode OrExpr() {
    ASTNode result = AndExpr();

    Tokinfo tok;
    // children are And Expressions, OR-ed together
    while ( ( tok = lex() ).token == TOKEN_LOGIC_OR ) {
        auto right = AndExpr();
        auto left = result;
        result = ASTNode ( AST_LOGIC_OR, tok.linenum, tok.lexeme );

        result.add_child( left  );
        result.add_child( right );
    }
    unlex( tok );

    return result;
}

ASTNode Expression() {
    return OrExpr();
}

ASTNode BreakStmt() {
    auto tok = expect( TOKEN_BREAK );

    return ASTNode ( AST_BREAK, tok.linenum, tok.lexeme );
}

ASTNode ReturnStmt() {
    auto tok = expect( TOKEN_RETURN );

    ASTNode result( AST_RETURN, tok.linenum, tok.lexeme );

    // optional expression to return
    if ( is_expression_next() ) {
        auto exp = Expression();
        result.add_child( exp );
    }

    return result;
}

ASTNode VarDecl() {
    auto tok = expect( TOKEN_VAR );

    ASTNode result ( AST_VAR, tok.linenum, tok.lexeme );

    auto newid     = newidentifier ();
    auto typeident = typeidentifier();

    result.add_child( newid     );
    result.add_child( typeident );

    return result;
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

ASTNode IfStmt() {
    auto tok = expect( TOKEN_IF );

    ASTNode result( AST_IF, tok.linenum, tok.lexeme );

    // Condition
    {
        auto expr = Expression();
        result.add_child( expr );
    }

    check_for_bad_semicolon();

    // block to execute if condition is true
    {
        auto blk = Block();
        result.add_child( blk );
    }

    // if no else, we're done
    if ( ( tok = lex() ).token != TOKEN_ELSE ) {
        unlex( tok );
        return result;
    }

    result.type    = AST_IFELSE;
    result.lexeme  = "";

    tok = lex();
    unlex( tok );

    // else -> lbrace = if/else
    // else -> if     = if/elseif i.e. another if statement
    if ( tok.token == TOKEN_LBRACE ) {
        auto blk = Block();
        result.add_child( blk );
    } else if ( tok.token == TOKEN_IF ) {
        auto ifstmt = IfStmt();
        result.add_child( ifstmt );
    } else
        error( tok.linenum, "syntax error o\"%s\"", tok.lexeme.data() );

    return result;
}

ASTNode ForStmt() {
    auto tok = expect( TOKEN_FOR );

    ASTNode result( AST_FOR, tok.linenum );

    tok = lex();
    unlex( tok );

    // [ Condition ]
    if ( tok.token != TOKEN_LBRACE ) {
        auto expr = Expression();
        result.add_child( expr );
    } else
        result.add_child( { AST_ID, -1, "$true" } );

    tok = lex();
    unlex( tok );

    check_for_bad_semicolon();

    auto blk = Block();
    result.add_child( blk );

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

            // Assignment
            if ( ( tok = lex() ).token == TOKEN_ASSIGN ) {
                auto right = Expression();

                auto left = result;

                result = ASTNode ( AST_ASSIGN, tok.linenum, tok.lexeme );
                result.add_child( left );
                result.add_child( right );

                if ( ( tok = lex() ).token == TOKEN_ASSIGN ) error( tok.linenum, "syntax error on \"=\"" );
            } else {
                auto tmp = result;
                result   = ASTNode( AST_EXPRSTMT, tmp.linenum );
                result.add_child( tmp );
            }
            unlex( tok );

            return result;
    }
}

// "formal" in the reference compiler I think
ASTNode ParameterDecl() {
    ASTNode result ( AST_FORMAL );

    auto newid     = newidentifier();
    auto typeident = typeidentifier();

    result.add_child( newid     );
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
    ASTNode sig     ( AST_SIGNATURE );
    ASTNode formals ( AST_FORMALS   );

    tok = lex();
    unlex( tok );

    // ParameterList
    if ( tok.token == TOKEN_ID ) {

        auto params = ParameterDecl();
        formals.add_child( params );

        // parameters delimited by commas
        while ( ( tok = lex() ).token == TOKEN_COMMA
             && ( tok = lex() ).token == TOKEN_ID ) {
            unlex( tok );

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

    // return type
    if ( tok.token == TOKEN_ID ) {
        auto tident = typeidentifier();
        sig.add_child( tident );
    } else {
        ASTNode inserted_void( AST_TYPEID, -1, "$void" );

        sig.add_child( inserted_void );
    }

    result.add_child( sig );

    check_for_bad_semicolon();

    // FunctionBody
    auto blk = Block();
    result.add_child( blk );

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
            result.type = AST_GLOBVAR;
            break;
        default:
            printf("current token is: %s\n", tok.lexeme.data());
            error( tok.linenum, "top level declarations must begin with 'func' or 'var'" );
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

std::string ASTNode_to_string( ASTNode n ) {
    std::string numstring;
    if ( n.linenum > 0 ) numstring = " @ line " + std::to_string( n.linenum );

    std::string lexstring;
    if ( n.lexeme.size() > 0 ) lexstring = " [" + n.lexeme + "]";

    switch ( n.type ) {
        // "program structure" or someth idk
        case AST_PROGRAM:   return "program";
        case AST_SIGNATURE: return "sig";
        case AST_FORMAL:    return "formal";
        case AST_FORMALS:   return "formals";
        case AST_BLOCK:     return "block";
        case AST_ACTUALS:   return "actuals";
        case AST_FUNCCALL:  return "funccall";

        // statements
        case AST_EXPRSTMT:  return "exprstmt" + numstring;
        case AST_EMPTYSTMT: return "emptystmt";

        // keywords
        case AST_BREAK:     return "break"    + numstring;
        case AST_FOR:       return "for"      + numstring;
        case AST_FUNC:      return "func"     + numstring;
        case AST_IF:        return "if"       + numstring;
        case AST_IFELSE:    return "ifelse"   + numstring;
        case AST_RETURN:    return "return"   + numstring;
        case AST_VAR:       return "var"      + numstring;
        case AST_GLOBVAR:   return "globvar"  + numstring;

        case AST_PLUS:      return "+"        + numstring;
        case AST_MINUS:     return "-"        + numstring;
        case AST_UMINUS:    return "u-"       + numstring;
        case AST_MUL:       return "*"        + numstring;
        case AST_DIV:       return "/"        + numstring;
        case AST_MOD:       return "%"        + numstring;
        case AST_ASSIGN:    return "="        + numstring;
        case AST_LOGIC_AND: return "&&"       + numstring;
        case AST_LOGIC_OR:  return "||"       + numstring;
        case AST_LOGIC_NOT: return "!"        + numstring;
        case AST_EQ:        return "=="       + numstring;
        case AST_LT:        return "<"        + numstring;
        case AST_GT:        return ">"        + numstring;
        case AST_NEQ:       return "!="       + numstring;
        case AST_LEQ:       return "<="       + numstring;
        case AST_GEQ:       return ">="       + numstring;

        // identifiers
        case AST_ID:        return "id"       + lexstring + numstring;
        case AST_NEWID:     return "newid"    + lexstring + numstring;
        case AST_TYPEID:    return "typeid"   + lexstring + numstring;


        // literals
        case AST_INT:       return "int"      + lexstring + numstring;
        case AST_STRING:    return "string"   + lexstring + numstring;

        // sadness :(
        case AST_UNSET: error( n.linenum, "internal error" );
    }

    // this is unrachable, each case of the switch above returns or errors
    error( -1, "unreachable in node to_string" );
    return "";
}

void printast( const ASTNode & root, int depth ) {
    // indent based on depth in tree
    std::string indent;
    for ( int i = 0; i < depth; i++ ) indent += "    ";
void preorder
( ASTNode & root, void ( *callback )( ASTNode & ) )
{
    prepost( root, callback, +[]( ASTNode & ) {} );
}

void postorder
( ASTNode & root, void ( *callback )( ASTNode & ) )
{
    for ( auto & child : root.children ) postorder( child, callback );

    callback( root );
}

void prepost
( ASTNode & root, void ( *precallback )( ASTNode & ), void ( *postcallback )( ASTNode & ) )
{
    precallback( root );

    // TODO: if we need to prune, bail here

    for ( auto & child : root.children ) prepost( child, precallback, postcallback );

    postcallback( root );
}


    printf( "%s%s\n", indent.data(), ASTNode_to_string( root ).data() );

    for ( const auto & kid : root.children )
        printast( kid, depth + 1);
}
