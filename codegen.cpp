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

// TODO: div/mod by zero check and error
// TODO: cite shankar's thing
inline void asm_prologue()
{
    output +=
R"(
.text
# RTS FUNCTIONS

.globl main
main:
        jal Lmain
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
    output += '\t' + instruction + '\n';
}

inline std::string getlabel
( std::string prefix, int num )
{
    return prefix + std::to_string( num );
}

void pass_1_pre( ASTNode & node );
void pass_1_post( ASTNode & node );

static std::vector<std::string> break_to_labels;

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
        case AST_FOR:
        {
            std::string top    = getlabel( "L", generic_labels++ );
            std::string bottom = getlabel( "L", generic_labels++ );

            break_to_labels.push_back( bottom );

            emitlabel( top );

            prepost( node[ 0 ], pass_1_pre, pass_1_post );

            emitinstruction( "beqz " + node[ 0 ].reg + ", " + bottom );

            prepost( node[ 1 ], pass_1_pre, pass_1_post );

            emitinstruction( "j " + top );

            emitlabel( bottom );

            break_to_labels.pop_back();

            throw PruneTraversalException();
        }
        default: break;
    }
}

void pass_1_post( ASTNode & node )
{
    switch ( node.type )
    {
        case AST_STRING:
        {
            node.reg         = allocreg();
            node.stringlabel = getlabel( "S", string_labels++ );

            emitinstruction( "la " + node.reg + ", " + node.stringlabel );
        }
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
                emitinstruction( "li " + node.reg + ", " + std::to_string( val )  + "# " + node.lexeme);

                break;
            }

            // not a symbol or a constant, its an identifier

            std::string location = "LOCATION UNSET";
            // FIXME: handle true/false constants
            if ( node.symbolinfo->label.empty() )
                // TODO: change this when I figure out how I know where stuff is
                // on the stack
                location = std::to_string( node.symbolinfo->frame_offset_bytes ) + "($fp)";
            else
                location = node.symbolinfo->label;

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
    }
}

void pass_2_cb( ASTNode & node )
{
    if ( node.type != AST_GLOBVAR && node.type != AST_STRING ) return;

    std::string label       = "LABEL UNSET";
    std::string instruction = "INSTRUCTION UNSET";

    if ( node.type == AST_GLOBVAR )
    {
        std::string type = node.symbolinfo->signature;
        instruction      = type == "string" ? ".word S0" : ".word 0" ;

        label = getlabel( "G", global_labels++ );
        node.symbolinfo->label = label;
    }
    else
    {
        label       = node.stringlabel;
        instruction = ".asciiz \"" + node.lexeme + "\"" ;
    }

    emitlabel( label );
    emitinstruction( instruction );
}

void gen_code( ASTNode & root )
{
    // EMIT PROLOGUE
    // - set up stuff and go to main
    asm_prologue();

    // pass 1 - code generation
    // prepost for everything else
    // funccall:
    //  - function prologue
    //  - jump and return address thing to function
    //  - function epilogue
    prepost( root, pass_1_pre, pass_1_post );

    asm_epilogue();

    // pass 2 - statically allocated things
    // preorder to emit global var declarations
    // - AST_GLOBVAR
    //    - a label plus either a word or a string constant
    // - AST_FUNC ( ????? )

    preorder( root, pass_2_cb );

    printf( "%s", output.data() );
}
