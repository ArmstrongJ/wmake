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
* Description:  Suffix (extension) management.
*
****************************************************************************/


#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "make.h"
#include "massert.h"
#include "mcache.h"
#include "mmemory.h"
#include "mhash.h"
#include "mmisc.h"
#include "pathgrp.h"
#include "mrcmsg.h"
#include "msg.h"
#include "msuffix.h"
#include "mtarget.h"


#define HASH_PRIME  13
#define CASESENSITIVE FALSE // Is suffix name case insensitive
STATIC HASHTAB  *sufTab;
STATIC UINT16   nextId;
STATIC UINT16   prevId;     // Has to be one less than nextId


STATIC void freePathRing( PATHRING *pring )
/*****************************************/
{
    PATHRING    *walk;
    PATHRING    *cur;

    if( pring == NULL ) {
        return;
    }

    walk = pring;
    do {
        cur = walk;
        walk = walk->next;
        FreeSafe( cur->name );
        FreeSafe( cur );
    } while( walk != pring );
}


STATIC BOOLEAN freeSuffix( void *node, void *ptr )
/************************************************/
{
    SUFFIX  *suf = node;
    CREATOR *ccur;
    CREATOR *cwalk;

    (void)ptr; // Unused
    FreeSafe( suf->node.name );
    freePathRing( suf->first );

    cwalk = suf->creator;
    while( cwalk != NULL ) {
        ccur = cwalk;
        cwalk = cwalk->next;
        KillTarget( ccur->cretarg->node.name );
        FreeSafe( ccur );
    }

    FreeSafe( suf );

    return( FALSE );
}


void ClearSuffixes( void )
/********************************
 * clear all suffix definitions
 */
{
    WalkHashTab( sufTab, freeSuffix, NULL );
    FreeHashTab( sufTab );
    sufTab = NewHashTab( HASH_PRIME );
    nextId = 32768U;
    prevId = nextId - 1;
}


#ifdef __WATCOMC__
#pragma on (check_stack);
#endif
STATIC SUFFIX *findSuffixNode( const char *name, const char **p )
/****************************************************************
 * returns: pointer to SUFFIX named name, or NULL.  If p != NULL, then
 *          *p will be the pointer to the first dot not in the first
 *          position of name.  ie:
 *          .src.dest   returns SUFFIX src, and p = ".dest"
 *          src.dest    same
 *          .src or src returns SUFFIX src, and p = NULL
 */
{
    char        sufname[MAX_SUFFIX];
    const char  *s;
    char        *d;

    assert( name != NULL );

    if( name[0] == DOT ) {
        ++name;
    }

    d = sufname;
    s = name;
    while( *s != NULLCHAR && *s != DOT ) {
        *d++ = *s++;
    }
    *d = NULLCHAR;

    if( p != NULL ) {
        if( *s == DOT ) {
            *p = s;
        } else {
            *p = NULL;
        }
    }

    FixName( sufname );

    return( (SUFFIX *)FindHashNode( sufTab, sufname, CASESENSITIVE ) );
}
#ifdef __WATCOMC__
#pragma off(check_stack);
#endif


SUFFIX *FindSuffix( const char *name )
/*******************************************/
{
    return( findSuffixNode( name, NULL ) );
}


BOOLEAN SufExists( const char *name )    /* with . */
/******************************************/
{
    assert( name != NULL && name[0] == DOT );

    return( FindSuffix( name ) != NULL );
}


STATIC void AddFrontSuffix( char const *name )
/*********************************************
 * pass name with leading .; adds name to suffix table and assigns id
 * retains use of name after call
 * adds the suffix to the front of the extensions list by giving an id
 * that is decremented instead of incremented for microsoft option only
 */
{
    SUFFIX  *new;

    assert( (name + 1) != NULL && name[0] == DOT && !SufExists( name ) );

    new = CallocSafe( sizeof( *new ) );
    new->node.name = FixName( StrDupSafe( name + 1 ) );
    new->id = prevId;
    --prevId;

    AddHashNode( sufTab, (HASHNODE *)new );
}


BOOLEAN SufBothExist( const char *sufsuf )   /* .src.dest */
/************************************************
 *  for MS-Option it only checks if the dependent suffix is defined
 *  so no need for checking the target suffix if it exists
 */
{
    char const  *ptr;

    assert( sufsuf != NULL && sufsuf[0] == DOT &&
        strchr( sufsuf + 1, DOT ) != NULL );

    if( findSuffixNode( sufsuf, &ptr ) == NULL ) {
        return( FALSE );
    }

    if( FindSuffix( ptr ) == NULL ) {
        if( Glob.microsoft == TRUE ) {
            AddFrontSuffix( ptr );
            return( TRUE );
        } else {
            return( FALSE );
        }

    } else {
        return( TRUE );
    }

}


void AddSuffix( char *name )
/**********************************
 * pass name with leading .; adds name to suffix table and assigns id
 * retains use of name after call
 */
{
    SUFFIX  *new;
    char    *d;
    char    *s;

    assert( ( name != NULL && name[0] == DOT && !SufExists( name ) ) ||
            ( name != NULL && name[0] == DOT && SufExists( name ) &&
            Glob.microsoft ) );

    d = name;                   /* shift left by 1 place */
    s = name + 1;
    while( *s != NULLCHAR ) {
        *d++ = *s++;
    }
    *d = NULLCHAR;

    new = CallocSafe( sizeof( *new ) );
    new->node.name = FixName( name );
    new->id = nextId;
    ++nextId;

    AddHashNode( sufTab, (HASHNODE *)new );
}


STATIC void ringPath( PATHRING **pring, const char *path )
/********************************************************/
{
    PATHRING    **tail;
    PATHRING    *new;
    const char  *p;
    size_t      len;

    assert( pring != NULL && path != NULL );

    tail = pring;            /* find "tail" of ring */
    if( *tail != NULL ) {
        do {
            tail = &(*tail)->next;
        } while( *tail != *pring );
    }

    p = path;
    while( *p != NULLCHAR ) {
            /* find end of path in string */
        while( *p != NULLCHAR && *p != PATH_SPLIT && *p != ';' ) {
            ++p;
        }

        new = MallocSafe( sizeof( *new ) );     /* get a new node */
        len = p - path;                         /* get length of sub-path */
        new->name = MallocSafe( len + 1 );      /* make copy of sub-path */
        memcpy( new->name, path, len );
        new->name[len] = NULLCHAR;
        FixName( new->name );

        *tail = new;        /* link into ring - but don't close ring yet */
        tail = &new->next;

        if( *p != NULLCHAR ) {
            ++p;
            path = p;        /* advance to next path in string */
        }
    }
    *tail = *pring;         /* now we finally close the ring up */
}


void SetSufPath( const char *name, const char *path )
/**********************************************************/
/* name with . */
{
    SUFFIX      *suf;

    assert( name != NULL && name[0] == DOT );

    suf = FindSuffix( name );

    assert( suf != NULL );

    if( path == NULL ) {
        freePathRing( suf->first );
        suf->first = NULL;
        suf->pathring = NULL;
        return;
    }

    ringPath( &suf->first, path );
    if( suf->pathring == NULL ) {
        suf->pathring = suf->first;
    }
}


STATIC CREATOR *newCreator( void )
/********************************/
{
    return( (CREATOR *)CallocSafe( sizeof( CREATOR ) ) );
}


void AddCreator( const char *sufsuf )
/*******************************************
 * add the creation .src.dest
 */
{
    SUFFIX      *src;
    SUFFIX      *dest;
    char const  *ptr;
    CREATOR     *new;
    CREATOR     **cur;

    assert( sufsuf != NULL && sufsuf[0] == DOT &&
        strchr( sufsuf + 1, DOT ) != NULL );

    src = findSuffixNode( sufsuf, &ptr );
    dest = FindSuffix( ptr );

    assert( src != NULL && dest != NULL );

    if( !Glob.microsoft && !Glob.posix && src->id < dest->id ) {
        PrtMsg( ERR | LOC | EXTENSIONS_REVERSED );
    }
    cur = &dest->creator;
    while( *cur != NULL ) {
        if( src->id <= (*cur)->suffix->id ) {
            break;
        }
        cur = &(*cur)->next;
    }

    if( *cur != NULL && src->id == (*cur)->suffix->id ) {
        return;
    }

    new = newCreator();
    new->suffix = src;
    new->cretarg = NewTarget( sufsuf );
    new->cretarg->special = TRUE;
    new->cretarg->sufsuf  = TRUE;

    new->next = *cur;
    *cur = new;

    return;
}


STATIC BOOLEAN printSuf( void *node, void *ptr )
/**********************************************/
{
    SUFFIX      *suf = node;
    CREATOR     *cur;
    PATHRING    *pring;
    TARGET      *targ;
    CLIST       *cmds;
    BOOLEAN     printed;

    (void)ptr; // Unused
    PrtMsg( INF | PSUF_SUFFIX, suf->node.name );
    if( suf->pathring != NULL ) {
        pring = suf->pathring;
        do {
            PrtMsg( INF | PSUF_FOUND_IN, pring->name );
            pring = pring->next;
        } while( pring != suf->pathring );
        printed = TRUE;
    } else {
        printed = FALSE;
    }
    cur = suf->creator;
    if( cur != NULL && printed ) {
        PrtMsg( INF | NEWLINE );
    }
    while( cur != NULL ) {
        PrtMsg( INF | NEOL | PSUF_MADE_FROM, cur->suffix->node.name );
        targ = cur->cretarg;
        PrintTargFlags( targ );
        PrtMsg( INF | NEWLINE );
        cmds = targ->depend->clist;
        if( cmds != NULL ) {
            PrtMsg( INF | PSUF_USING_CMDS );
            PrintCList( cmds );
        }
        cur = cur->next;
        if( cur != NULL ) {
            PrtMsg( INF | NEWLINE );
        }
    }
    PrtMsg( INF | NEWLINE );

    return( FALSE );
}


void PrintSuffixes( void )
/*******************************/
{
    WalkHashTab( sufTab, printSuf, NULL );
}


STATIC RET_T chkOneName( char *buffer, TARGET **chktarg )
/*******************************************************/
{
    TARGET  *tmp;

    FixName( buffer );

    if( chktarg != NULL ) {
        tmp = FindTarget( buffer );
        if( tmp != NULL ) {
            *chktarg = tmp;
            return( RET_SUCCESS );
        }
    }
    if( CacheExists( buffer ) ) {
        return( RET_SUCCESS );
    }
    return( RET_ERROR );
}


STATIC RET_T tryPathRing( PATHRING **pring, char *buffer,
    const char *dir, const char *fname, const char *ext, TARGET **chktarg )
/**************************************************************************
 * walk a path ring, and attempt to find fname.ext using different paths
 */
{
    PATHRING    *cur;
    char        fake_name[_MAX_PATH];

    assert( pring != NULL );
    if( *pring == NULL ) {
        return( RET_ERROR );
    }
    if( dir[0] == '\0' ) {
        dir = NULL;
    }
    _makepath( fake_name, NULL, dir, fname, ext );
    cur = *pring;
    do {
        _makepath( buffer, NULL, cur->name, fake_name, NULL );
        if( chkOneName( buffer, chktarg ) == RET_SUCCESS ) {
            if( Glob.optimize ) {       /* nail down pathring here */
                *pring = cur;
            }
            return( RET_SUCCESS );
        }
        cur = cur->next;
    } while( cur != *pring );

    return( RET_ERROR );
}


RET_T TrySufPath( char *buffer, const char *filename, TARGET **chktarg,
    BOOLEAN tryenv )
/*****************************************************************************
 * it is NOT necessary that filename != buffer
 * the contents of buffer may be destroyed even if RET_ERROR is returned
 * first checks current directory, then any in suffix path
 * assumes buffer is at least _MAX_PATH, and that if buffer != filename
 * then buffer, and filename do not overlap
 */
{
    PGROUP      pg;
    SUFFIX      *suffix;
    char        *env;
    PATHRING    *envpath;
    RET_T       ret;

    if( chktarg != NULL ) { /* always NULL the chktarg before working */
        *chktarg = NULL;
    }

    /* check if filename given exists */
    if( filename != buffer ) {
        strcpy( buffer, filename );
    }
    if( chkOneName( buffer, chktarg ) == RET_SUCCESS ) {
        return( RET_SUCCESS );
    }

    /* split up filename */
    _splitpath2( filename, pg.buffer, &pg.drive, &pg.dir, &pg.fname, &pg.ext );

    if( pg.drive[0] != NULLCHAR || isdirc( pg.dir[0] ) ) {
        /* is an absolute path name */
        return( RET_ERROR );
    }

    suffix = FindSuffix( pg.ext );

    ret = RET_ERROR;

    if( suffix == NULL || suffix->pathring == NULL ) {
            /* no suffix info - use %PATH */
        if( tryenv ) {
            env = getenv( "PATH" );
        } else {
            /*
             *  This is an incredible kludge so that Brian could build
             *  the header project from one makefile without making
             *  many changes to the makefile.   Some people are soooo
             *  lazy :)  DJG
             */
            env = getenv( "__SEARCH_PATH__" );
        }
        if( env != NULL ) {
            envpath = NULL;
            ringPath( &envpath, env );

            Glob.cachedir = FALSE;      /* never cache %path */
            ret = tryPathRing( &envpath, buffer, pg.dir, pg.fname, pg.ext,
                chktarg );
            Glob.cachedir = TRUE;

            freePathRing( envpath );
        }
    } else {
        ret = tryPathRing( &suffix->pathring, buffer, pg.dir, pg.fname,
            pg.ext, chktarg );
    }

    return( ret );
}


void SuffixInit( void )
/****************************/
{
    sufTab = NewHashTab( HASH_PRIME );
    nextId = 32768U;
    prevId = nextId - 1;
}


void SuffixFini( void )
/****************************/
{
#ifdef DEVELOPMENT
    ClearSuffixes();
    FreeHashTab( sufTab );
    sufTab = NULL;
#else
    WalkHashTab( sufTab, freeSuffix, NULL );
    FreeHashTab( sufTab );
#endif
}
