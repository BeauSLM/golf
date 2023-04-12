#include "codegen.hpp"
#include "error.hpp"
#include <cassert>

std::string registers[] =
{
    "$s0",
    "$s1",
    "$s2",
    "$s3",
    "$s4",
    "$s5",
    "$s6",
    "$s7",
    "$t0",
    "$t1",
    "$t2",
    "$t3",
    "$t4",
    "$t5",
    "$t6",
    "$t7",
    "$t8",
    "$t9",
};

static std::vector<std::string> register_pool(registers, registers + sizeof(registers) / sizeof(std::string));

std::string allocreg()
{
    if ( register_pool.size() == 0 ) error( -1, "Ran out of registers!" );

    std::string reg = register_pool.back();
    register_pool.pop_back();

    return reg;
}

inline void freereg
( std::string reg )
{
    // check if the register is already in the pool
    for ( auto & str : register_pool )
        assert( str != reg );

    register_pool.push_back( reg );
}

const static struct
{
    ASTNodeID operatorid;

    std::string instr,
    // if this is "r", then the corresponding argument is a register
                arg2 = "r";
} op_instr_mapping[] =
{

        { AST_PLUS,      "addu",     },
        { AST_MINUS,     "subu",     },
        { AST_MUL,       "mul",      },

        { AST_DIV,       "div",      },
        { AST_MOD,       "div",      },

        { AST_EQ,        "seq",      },
        { AST_LT,        "slt",      },
        { AST_LEQ,       "sle",      },

        { AST_LOGIC_NOT, "xori", "1" },
        { AST_UMINUS,    "negu", ""  },

        // AST_LOGIC_AND,
        // AST_LOGIC_OR,
};

static std::string output;

// label counters
static int generic_labels = 0,
           string_labels  = 1; // S0 is reserved for included null string

// TODO: cite shankar's thing
inline void asm_prologue()
{
    output +=
R"(
.text
# RTS FUNCTIONS

.globl main
main:
        jal F_main
        j   halt

getchar:
        addi $sp, $sp, -4
        sw $ra, 0($sp)

        li $v0, 8                       # System call for read_string
        la $a0, getcharbuf              # Pass in buffer (size = n+1)
        li $a1, 2                       # Size = n+1 as read_string adds null
        syscall

        lb $v0, getcharbuf
        bne $v0, $zero, ret

        li $v0, -1                      # If 0, map to -1 and return
ret:
        lw $ra 0($sp)
        addi $sp $sp, 4
        jr $ra


prints:
        addi $sp, $sp, -4
        sw $ra, 0($sp)

        # $a0 contains the address of the string to print
        li $v0, 4                       # System call for print_string
        syscall

        lw $ra 0($sp)
        addi $sp $sp, 4
        jr $ra


# No need for stack here as
# the program will halt after this
halt:
        li $v0, 10
        syscall
error:
        # a0 should contain address of error message to print
        li $v0, 4
        syscall

        # exit with error
        li $a0, 1
        li $v0, 17
        syscall

divmodcheck:
        addi $sp, $sp, -4
        sw $ra, 0($sp)

        # if divisor is zero, error
        beqz $a1, divmoderror

        lw $ra, 0($sp)
        addi $sp, $sp, 4
        jr $ra

divmoderror:

        la $a0, divbyzeromsg
        j error

# begin generated code
)";
}

inline void asm_epilogue()
{
    output +=
R"(
.data
getcharbuf:
        .space 2
        .align 2
S0:
        .byte 0
        .align 2
returnerror:
        .asciiz "error: failed to return from function that needed a return"
divbyzeromsg:
        .asciiz "error: division by zero"
)";
}

inline void emitlabel
( std::string code )
{
    output += code + ":\n";
}

inline void emitinstruction
( std::string instruction )
{
    output += "\t\t" + instruction + '\n';
}

inline std::string getlabel
( std::string prefix, int num )
{
    return prefix + std::to_string( num );
}

void pass_1_pre( ASTNode & node );
void pass_1_post( ASTNode & node );

static std::vector<std::string> break_to_labels;

// XXX: this sucks and needs to be better
int function_prologue
( std::vector<ASTNode> & args, std::vector<ASTNode *> & local_vars )
{
    // allocate 1 word for each arg, each local, and return address
    size_t stack_words =  args.size() + local_vars.size() + 1;

    emitinstruction( "addi $sp, $sp, -" + std::to_string( stack_words * 4 ) );
    emitinstruction( "sw $ra, 0($sp)" );

    for ( size_t i = 0; i < args.size(); i++ )
    {
        size_t offset_bytes = ( i + 1 ) * 4 ;
        args[ i ][ 0 ].symbolinfo->stack_offset_bytes = offset_bytes;

        emitinstruction( "sw $a" + std::to_string( i ) + ", " + std::to_string( offset_bytes ) + "($sp)" );
    }

    // local vars
    for ( size_t i = 0 ; i < local_vars.size() ; i++ )
    {
        size_t offset_bytes = 4 + 4 * args.size() + 4 * i;

        auto sym = local_vars[ i ]->symbolinfo;
        sym->stack_offset_bytes = offset_bytes;


        std::string stack_ref = std::to_string( offset_bytes ) + "($sp)" ;
        // if local var is a string, point it at empty string in runtime
        // else, set it to 0
        if ( sym->signature == "string" )
        {
            emitinstruction( "la $a0, S0" );
            emitinstruction( "sw $a0, " + stack_ref );
        }
        else
            emitinstruction( "sw $zero, " + stack_ref );
    }

    return stack_words;
}

void function_epilogue
( int stack_words )
{
    emitinstruction( "lw $ra, 0($sp)" );
    emitinstruction( "addi $sp, $sp, " + std::to_string( 4 * stack_words ) );
    emitinstruction( "jr $ra" );
}

inline std::string ident_location
( ASTNode & node )
{
    auto sym = node.symbolinfo;
    // if the identifier doesn't have a label, it's on the stack
    return sym->label.empty()
        ? std::to_string( sym->stack_offset_bytes ) + "($sp)"
        : sym->label;
}

void pass_1_pre( ASTNode & node )
{
    switch ( node.type )
    {
        case AST_IF:
        {
            std::string bottom = getlabel( "L", generic_labels++ );

            prepost( node[ 0 ], pass_1_pre, pass_1_post );

            emitinstruction( "beqz " + node[ 0 ].reg + ", " + bottom );
            freereg( node[ 0 ].reg );

            prepost( node[ 1 ], pass_1_pre, pass_1_post );

            emitlabel( bottom );

            throw PruneTraversalException();
        } break;
        // REVIEW: does if else if just fall out?
        case AST_IFELSE:
        {
            std::string bottom = getlabel( "L", generic_labels++ );
            std::string else_  = getlabel( "L", generic_labels++ );

            // guard expression
            prepost( node[ 0 ], pass_1_pre, pass_1_post );

            // if !guard expression, jump to else block
            emitinstruction( "beqz " + node[ 0 ].reg + ", " + else_ );
            freereg( node[ 0 ].reg );

            // if block, then jump to end
            prepost( node[ 1 ], pass_1_pre, pass_1_post );
            emitinstruction( "j " + bottom );

            // else block
            emitlabel( else_ );
            prepost( node[ 2 ], pass_1_pre, pass_1_post );

            emitlabel( bottom );

            throw PruneTraversalException();
        } break;
        case AST_FOR:
        {
            std::string top    = getlabel( "L", generic_labels++ );
            std::string bottom = getlabel( "L", generic_labels++ );

            break_to_labels.push_back( bottom );

            emitlabel( top );

            // TODO: make sure registers used in test aren't freed until after the loop

            prepost( node[ 0 ], pass_1_pre, pass_1_post );

            emitinstruction( "beqz " + node[ 0 ].reg + ", " + bottom );

            prepost( node[ 1 ], pass_1_pre, pass_1_post );

            emitinstruction( "j " + top );

            emitlabel( bottom );

            // NOTE: we free here because the register won't be looked at again only after the loop
            freereg( node[ 0 ].reg );

            break_to_labels.pop_back();

            throw PruneTraversalException();
        } break;
        case AST_GLOBVAR:
        {
            std::string label = "G_" + node[ 0 ].lexeme;
            node.symbolinfo->label = label;
        } break;
        case AST_FUNC:
        {
            // XXX: dear god I am sorry
            {
                static std::vector<ASTNode *> local_vars;
                local_vars.clear();

                auto find_local_vars = +[]( ASTNode & node )
                {
                    if ( node.type == AST_VAR ) local_vars.push_back( &node );
                };

                preorder( node[ 2 ], find_local_vars );

                node.symbolinfo->stack_size_words = function_prologue( node[ 1 ][ 0 ].children, local_vars );

                std::string label = "F_" + node[ 0 ].lexeme;
                node.symbolinfo->label = label;
                emitlabel( label );
            }

            // generate code for function body
            prepost( node[ 2 ], pass_1_pre, pass_1_post );

            // if this function has a return value, and we fail to return, error
            // NOTE: I'll emit the epilogue at the return statement
            auto returnsignature = node.symbolinfo->returnsignature;
            if ( returnsignature != "void" && returnsignature != "$void" )
            {
                emitinstruction( "la $a0, returnerror" );
                emitinstruction( "j error" );
            }
            // otherwise, just return when we reach the end of execution
            else
                function_epilogue( node.symbolinfo->stack_size_words );

            throw PruneTraversalException();
        } break;
        case AST_FUNCCALL:
        {
            auto &arguments = node[ 1 ].children;
            // REVIEW: should I do this here or at the function definition?
            if ( arguments.size() > 4 )
                error ( -1, 0, "error: too many arguments to function '%s'", node.lexeme.c_str() );

            for ( size_t i = 0; i < arguments.size(); i++ )
            {
                prepost( arguments[ i ], pass_1_pre, pass_1_post );

                emitinstruction( "move $a" + std::to_string( i ) + ", " + arguments[ i ].reg );

                freereg( arguments[ i ].reg );
            }


            auto sym = node[ 0 ].symbolinfo;

            emitinstruction( "jal " + sym->label );

            if ( sym->returnsignature != "void" && sym->returnsignature != "$void" )
            {
                std::string reg = allocreg();
                node.reg = reg;
                emitinstruction( "move " + reg + ", $v0" );
            }

            throw PruneTraversalException();
        } break;
        case AST_ASSIGN:
        {
            std::string location = ident_location( node[ 0 ] );

            prepost( node[ 1 ], pass_1_pre, pass_1_post );

            emitinstruction( "sw " + node[ 1 ].reg + ", " + location );

            freereg( node[ 1 ].reg );

            throw PruneTraversalException();
        } break;
        default: break;
    }
}

void pass_1_post( ASTNode & node )
{
    switch ( node.type )
    {
        case AST_EXPRSTMT:
        {
            freereg( node[ 0 ].reg );
        } break;
        case AST_STRING:
        {
            node.reg         = allocreg();
            node.stringlabel = getlabel( "S", string_labels++ );

            emitinstruction( "la " + node.reg + ", " + node.stringlabel );
        } break;
        case AST_INT:
        {
            node.reg = allocreg();

            // REVIEW: should I trim leading zeros?
            emitinstruction( "li " + node.reg + ", " + node.lexeme );
        } break;
        case AST_ID:
        {
            STabRecord * sym = node.symbolinfo;

            // REVIEW: should I just return??
            if ( sym->istype ) break;

            node.reg = allocreg();

            if ( sym->isconst )
            {
                int val = node.lexeme == "false" ? 0 : 1;
                emitinstruction( "li " + node.reg + ", " + std::to_string( val )  + "# " + node.lexeme );

                break;
            }

            // not a symbol or a constant, its an identifier

            std::string location = ident_location( node );
            emitinstruction( "lw " + node.reg + ", " + location );
        } break;
        case AST_RETURN:
        {
            if ( node.children.size() > 0 )
            {
                prepost( node[ 0 ], pass_1_pre, pass_1_post );

                emitinstruction( "move $v0, " + node[ 0 ].reg );
                freereg( node[ 0 ].reg );
            }

            // TODO: jump to epilogue which will return instead
            emitinstruction( "jr $ra" );

            throw PruneTraversalException();
        } break;
        case AST_BREAK:
        {
            emitinstruction( "j " + break_to_labels.back() );
        } break;
        case AST_LOGIC_AND:
        case AST_LOGIC_OR:
        {
            node.reg = allocreg();

            std::string bottom     = getlabel( "L", generic_labels++ ),
                        // on &&, jump to bottom if false
                        skip_instr = node.type == AST_LOGIC_AND ? "beqz" : "bnez",
                        // on || jump to bottom if true
                        oper_instr = node.type == AST_LOGIC_AND ? "and"  : "or";

            prepost( node[ 0 ], pass_1_pre, pass_1_post );

            emitinstruction( skip_instr + " " + node[ 0 ].reg + ", " + bottom );

            prepost( node[ 1 ], pass_1_pre, pass_1_post );

            emitlabel( bottom );

            emitinstruction( oper_instr + " " + node.reg + ", " + node[ 0 ].reg + ", " + node[ 1 ].reg );

            // result = result & 1
            // TODO: comment about garbage in second operand's reg if we skip evaluating second operand
            emitinstruction( "andi " + node.reg + ", " + node.reg + ", 1" );

            freereg( node[ 0 ].reg );
            freereg( node[ 1 ].reg );

            throw PruneTraversalException();
        } break;
        default:
        {
            for ( auto & mapping : op_instr_mapping )
            {
                if ( mapping.operatorid != node.type ) continue;

                std::string instruction;
                std::string reg1 = node[ 0 ].reg,
                            dest = allocreg();

                // REVIEW: is this ordering valid for all the operators??

                // instr dest, $reg1
                instruction = mapping.instr + " " + dest + ", " + reg1;
                freereg( reg1 );

                std::string maparg2 = mapping.arg2;
                if ( maparg2 == "r" )
                {
                    // instr dest, $reg1, $reg2
                    std::string reg2 = node[ 1 ].reg;
                    instruction += ", " + reg2;
                    freereg( reg2 );
                }
                else if ( !maparg2.empty() )
                    // instr dest, $reg1, hardcoded value
                    instruction += ", " + maparg2;

                // TODO: handle div/mod by zero

                emitinstruction( instruction );
                break;
            }

        } break;
    }
}

void pass_2_cb( ASTNode & node )
{
    if ( node.type != AST_GLOBVAR && node.type != AST_STRING ) return;

    if ( node.type == AST_GLOBVAR )
    {
        auto sym = node.symbolinfo;

        std::string type = sym->signature;
        std::string instruction = type == "string" ? ".word S0" : ".word 0" ;

        emitlabel( sym->label );
        emitinstruction( instruction );

        return;
    }
    else
    {
        emitlabel( node.stringlabel );
        for ( size_t i = 0; i < node.lexeme.size(); i++ )
        {
            char charcode = node.lexeme[ i ];
            if ( charcode == '\\' )
            {
                switch ( node.lexeme[ ++i ] )
                {
                    case 'b':  charcode = '\b'; break;
                    case 'f':  charcode = '\f'; break;
                    case 'n':  charcode = '\n'; break;
                    case 'r':  charcode = '\r'; break;
                    case 't':  charcode = '\t'; break;
                    case '\\': charcode = '\\'; break;
                    case '"':  charcode = '"'; break;
                }
            }

            emitinstruction( ".byte " + std::to_string( charcode ) );
        }

        emitinstruction( ".byte 0" );
        emitinstruction( ".align 2" ); // REVIEW: is it okay to do this for every string?
    }

}

void gen_code( ASTNode & root )
{
    asm_prologue();

    // pass 1 - code generation
    // prepost for everything else
    // funccall:
    //  - function prologue
    //  - jump and return address thing to function
    //  - function epilogue
    prepost( root, pass_1_pre, pass_1_post );

    asm_epilogue();

    preorder( root, pass_2_cb );

    printf( "%s", output.data() );
}
