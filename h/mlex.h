/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  mlex.c interfaces
*
****************************************************************************/


#ifndef _MLEX_H
#define _MLEX_H     1
#include "watcom.h"
#include "mtypes.h"
#include "mstream.h"

extern char *targ_path;   /* Current sufsuf target path    */
extern char *dep_path;    /* Current sufsuf dependent path */

/*
 *  The masks for MS-macro modifier
 */

#define MOD_D   0x01
#define MOD_B   0x02
#define MOD_F   0x04
#define MOD_R   0x08

/*
 * These are used as place holders while doing macro expansion, they are
 * assumed to not be in the input stream.
 */
#define TMP_DOL_C           '\x01'  /* replace $ with this temporarily */
#define TMP_DOL_S           "\x01"
#define TMP_COMMENT_C       '\x02'  /* replace $# with this temporarily */
#define TMP_COMMENT_S       "\x02"
/* only good for microsoft option when doing partial deMacros     */
/* for special macros            */
#define SPECIAL_TMP_DOL_C   '\x03' /* replace $ with this temporarily */
#define SPECIAL_TMP_DOL_S   "\x03"

/*
 * Is this a special microsoft character in a macro
 * $@
 * $$@
 * $*
 * $**
 * $?
 * $<
 * as well as modifiers D,B,F,R
 */
#define ismsspecial(__c)   ((__c) == '@'\
                         || (__c) == '*'\
                         || (__c) == '?'\
                         || (__c) == '<')
#define ismsmodifier(__c)  ((__c) == 'D'\
                         || (__c) == 'd'\
                         || (__c) == 'B'\
                         || (__c) == 'b'\
                         || (__c) == 'F'\
                         || (__c) == 'f'\
                         || (__c) == 'R'\
                         || (__c) == 'r' )

/*
 * The tokens which Scan() will use - it may not return each and every one
 * of these (notably the preprocessor tokens - they are handled internally)
 */

#define TOK_MIN     400
#define MAC_MIN     500

enum Tokens {

    /*
     * dependency & rule parser tokens
     */
    TOK_SCOLON = TOK_MIN,           /* ":"                  */
    TOK_DCOLON,                     /* "::"                 */
    TOK_FILENAME,                   /* "{filec}+"           */
    TOK_DOTNAME,                    /* special dot name     */
    TOK_CMD,                        /* {ws}+cmd             */
    TOK_SUF,                        /* ".{extc}+"           */
    TOK_SUFSUF,                     /* .{extc}+.{extc}+     */
    TOK_PATH,                       /* {filec}+(;{filec}*)+ */

    TOK_MAX,

    /*
     * macro parser tokens
     */
    MAC_START = MAC_MIN,            /* "$"              */
    MAC_DOLLAR,                     /* "$$"             */
    MAC_COMMENT,                    /* "$#"             */
    MAC_OPEN,                       /* "$("             */
    MAC_CLOSE,                      /* ")"              */
    MAC_EXPAND_ON,                  /* "$+"             */
    MAC_EXPAND_OFF,                 /* "$-"             */
    MAC_CUR,                        /* "$^"             */
    MAC_FIRST,                      /* "$["             */
    MAC_LAST,                       /* "$]"             */
    MAC_ALL_DEP,                    /* "$<"             */
    MAC_YOUNG_DEP,                  /* "$?"             */
    MAC_NAME,                       /* {macc}+  used in LEX_MAC_SUBST */
    MAC_PUNC,                       /* {~macc}+ used in LEX_MAC_SUBST */
    MAC_WS,                         /* {ws}+    used in LEX_MAC_DEF */
    MAC_TEXT,                       /* {~ws}+   used in LEX_MAC_DEF */
    MAC_INF_DEP,                    /* This is needed in MS since in
                                       inference rules there are two types
                                       of dependent files*/

    MAC_MAX,

    // These token types are for MS Compatability which allows the
    // use of if (constantExpression) in its preprocessing
    OP_INTEGER,                    /* operands that are integers    */
    OP_STRING,                     /* operands that are strings     */
    OP_COMPLEMENT,                 /* "~"                */
    OP_LOG_NEGATION,               /* "!"                */
    OP_ADD,                        /* "+"  can be unary  */
    OP_SUBTRACT,                   /* "-"  can be unary  */
    OP_MULTIPLY,                   /* "*"                */
    OP_DIVIDE,                     /* "/"                */
    OP_MODULUS,                    /* "%"                */
    OP_BIT_AND,                    /* "&"                */
    OP_BIT_OR,                     /* "|"                */
    OP_BIT_XOR,                    /* "^"                */
    OP_LOG_AND,                    /* "&&"               */
    OP_LOG_OR,                     /* "||"               */
    OP_SHIFT_LEFT,                 /* "<<"               */
    OP_SHIFT_RIGHT,                /* ">>"               */
    OP_EQUAL,                      /* "=="               */
    OP_INEQU,                      /* "!="               */
    OP_LESSTHAN,                   /* "<"                */
    OP_GREATERTHAN,                /* ">"                */
    OP_LESSEQU,                    /* "<="               */
    OP_GREATEREQU,                 /* ">="               */
    OP_PAREN_LEFT,                 /* "("                */
    OP_PAREN_RIGHT,                /* ")"                */
    OP_DEFINED,                    /* DEFINED(MACRONAME) */
    OP_EXIST,                      /* EXIST[S](FILEPATH) */
    OP_SHELLCMD,                   /* "[ shellcmd ]"     */
    OP_ENDOFSTRING,                /* End of string Character */
    OP_ERROR,                      /* Token returned has error */

    OP_MAX,

    TOKENS_MAX
};

#define COMPLEMENT      '~'
#define LOG_NEGATION    '!'
#define ADD             '+'
#define SUBTRACT        '-'
#define MULTIPLY        '*'
#define DIVIDE          '/'
#define MODULUS         '%'
#define BIT_AND         '&'
#define BIT_OR          '|'
#define BIT_XOR         '^'
#define EQUAL           '='
#define LESSTHAN        '<'
#define GREATERTHAN     '>'
#define PAREN_LEFT      '('
#define PAREN_RIGHT     ')'
#define BRACKET_LEFT    '['
#define BRACKET_RIGHT   ']'
#define DOUBLEQUOTE     '\"'
#define BACKSLASH       '\\'
#define MAX_STRING      256
#define EXIST           "EXIST"
#define EXISTS          "EXISTS"
#define DEFINED         "DEFINED"

// Node Definition for the tokens
typedef struct  TOKEN_OP {
    enum Tokens type;    // Type of Token
    union {
        int_32  number;        // string value
        char    string[MAX_STRING];
    }   data;
}   TOKEN_TYPE;

typedef TOKEN_TYPE DATAVALUE;

enum DotNames {                 /* must be in alpha order! */
    DOT_MIN = -1,
    DOT_AFTER,                      /* ".AFTER"         */
    DOT_ALWAYS,                     /* ".ALWAYS"        */
    DOT_AUTO_DEPEND,                /* ".AUTODEPEND"    */
    DOT_BEFORE,                     /* ".BEFORE"        */
    DOT_BLOCK,                      /* ".BLOCK"         */
    DOT_CONTINUE,                   /* ".CONTINUE"      */
    DOT_DEFAULT,                    /* ".DEFAULT"       */
    DOT_ERASE,                      /* ".ERASE"         */
    DOT_ERROR,                      /* ".ERROR"         */
    DOT_EXISTSONLY,                 /* ".EXISTSONLY"    */
    DOT_EXPLICIT,                   /* ".EXPLICIT"      */
    DOT_EXTENSIONS,                 /* ".EXTENSIONS"    */
    DOT_FUZZY,                      /* ".FUZZY"         */
    DOT_HOLD,                       /* ".HOLD"          */
    DOT_IGNORE,                     /* ".IGNORE"        */
    DOT_RCS_MAKE,                   /* ".JUST_ENOUGH"   */
    DOT_MULTIPLE,                   /* ".MULTIPLE"      */
    DOT_NOCHECK,                    /* ".NOCHECK"       */
    DOT_OPTIMIZE,                   /* ".OPTIMIZE"      */
    DOT_PRECIOUS,                   /* ".PRECIOUS"      */
    DOT_PROCEDURE,                  /* ".PROCEDURE"     */
    DOT_RECHECK,                    /* ".RECHECK"       */
    DOT_SILENT,                     /* ".SILENT"        */
    DOT_SUFFIXES,                   /* ".SUFFIXES"      */
    DOT_SYMBOLIC,                   /* ".SYMBOLIC"      */
    DOT_MAX                         /* must always be last */
};

#define MAX_DOT_NAME    16      /* maximum characters needed for dot-name */

#if defined( DEVELOPMENT ) || defined( INTERNAL_VERSION )
#include "msysdep.h"
#if MAX_DOT_NAME > MAX_SUFFIX
#error "MAX_DOT_NAME must be at smaller than or equal to MAX_SUFFIX"
#endif
#endif

#define IsDotWithCmds(i)   \
    ( (i) == DOT_AFTER      \
    || (i) == DOT_BEFORE    \
    || (i) == DOT_DEFAULT   \
    || (i) == DOT_ERROR )

typedef STRM_T  TOKEN_T;

/*
 * These are the values returned in CurAttr.num with the MAC_CUR,
 * MAC_FIRST, and MAC_LAST tokens.
 */
enum FormQualifiers {
    FORM_MIN = 0,
    FORM_FULL,                  /* '@' */
    FORM_NOEXT,                 /* '*' */
    FORM_NOEXT_NOPATH,          /* '&' */
    FORM_NOPATH,                /* '.' */
    FORM_PATH,                  /* ':' */
    FORM_EXT,                   /* '!' */
    FORM_MAX
};


/*
 * global data
 */
extern const char * const   DotNames[];
extern union CurAttrUnion {
    char    *ptr;
    INT16   num;
} CurAttr;                          /* attribute of last return value */

/* NOTE: If you get a pointer in CurAttr.ptr, it is yours to play with. */
/* ie: When done, you MUST do a FreeSafe( CurAttr.ptr )                 */


/*
 * Different modes of the LexToken() function.  Note that pre-processing
 * is done silently during each mode.
 */
enum LexMode {
    LEX_MAC_DEF,    /* send back MAC_EXPAND_ON, otherwise MAC_TEXT          */
    LEX_MAC_SUBST,  /* send back all MAC tokens                             */
    LEX_PARSER,     /* send back only TOK tokens - silently do MAC stuff    */
    LEX_PATH,       /* send back only TOK_PATH/EOL/STRM_END                 */
    LEX_MS_MAC      /* sned back tokens for microsoft demacro               */
};

extern void     LexInit( void );
extern void     LexFini( void );
extern TOKEN_T  LexToken( enum LexMode mode );
extern void     LexMaybeFree( TOKEN_T tok );

/* never call these directly! only call through LexToken() */
extern TOKEN_T  LexParser( STRM_T );
extern TOKEN_T  LexPath( STRM_T );
extern TOKEN_T  LexMacSubst( STRM_T );
extern TOKEN_T  LexMacDef( STRM_T );
extern TOKEN_T  LexMSDollar ( STRM_T );

extern void     GetModifier ( void );

#endif /* !_MLEX_H */
