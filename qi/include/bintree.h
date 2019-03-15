/*
 * Copyright (c) 1985 Corporation for Research and Educational Networking
 * Copyright (c) 1988 University of Illinois Board of Trustees, Steven
 *		Dorner, and Paul Pomes
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Corporation for
 *	Research and Educational Networking (CREN), the University of
 *	Illinois at Urbana, and their contributors.
 * 4. Neither the name of CREN, the University nor the names of their
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE TRUSTEES AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TRUSTEES OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)$Id: bintree.h,v 1.14 1995/02/28 19:33:42 p-pomes Exp $
 */

#ifndef BINTREE_H
#include "conf.h"
#define BINTREE_H
#define LBSIZE 1024		/* the size of a leaf */
#define MATCH 0
#define NO_MATCH 1
#define CONTINUE -1
#define KEY_SIZE	4	/* the number of characters kept in NODEs */
#define DATA_SIZE (LBSIZE-2*sizeof(IDX)-sizeof(INT32))	/* free space in leaf */
#define NODE_SIZE sizeof(NODE)
#define IDX_SIZE sizeof(IDX)

typedef INT32 IDX;		/* index pointer size */

/*
 * Bintree maintains (along with maket) a four-tiered structure consisting
 * of a header, nodes, leaves, and items.
 *
 * ITEMS contain the entries from the hash table (.idx); the string and
 * the hash index are kept in each ITEM.
 *
 * LEAVES contain one or more items, in sorted order, and are linked
 * together.  All the keys in the hash table may be examined in sorted
 * order by traversing the leaves in order and the items in order within
 * the leaves.
 *
 * NODES contain the first four characters of a key, and left and right
 * pointers.  These pointers point to other nodes when positive, or leaves
 * when negative.
 *
 * The QHEADER has some statistics, as well as pointers to the head node
 * and first and last leaves.
 *
 * All pointers are stored as indices, and must be multiplied by the
 * appropriate size to get disk addresses.
 *
 * Leaves (and hence items) and the header are kept in the .seq file.
 * Nodes are kept in the .bdx file.
 *
 * Steve Dorner, Computing Services Office, University of Illinois, 4/88
 */

/*
 * NODEs are used for quick indexing into the leaf list.  They are
 * created in maket.c, and reside in the .bdx file.
 */
struct node
{
	IDX	l_ptr;		/* if your name is <= key */
	char	key[KEY_SIZE];	/* greatest key in l_ptr subtree */
	IDX	r_ptr;		/* if your name is > key */
};
typedef struct node NODE;

/*
 * a sorted set of keys from the index; linked together to form a sorted
 * list of entire index. kept in the .seq file.
 */
struct leaf
{
	IDX	leaf_no;	/* this leaf's index */
	IDX	next;		/* pointer to next leaf */
	int	n_bytes;	/* number of bytes in data */
	char	data[DATA_SIZE]; /* data--zero or more ITEMs */
};
typedef struct leaf LEAF;

/*
 * this structure is used when building the .bdx (node) file
 */
struct leaf_des
{
	IDX	leaf_no;	/* start of leaf string */
	char	max_key[KEY_SIZE]; /* biggest key in leaf string */
};
typedef struct leaf_des LEAF_DES;

/*
 * the basic unit.  contains a single key from the index.  multiple ITEMs
 * are kept in each leaf.
 */
union item
{
	struct
	{
		IDX	p_number;
		char	p_key[256];
	}	pair;
	char	raw[260];
};
typedef union item ITEM;

#define i_num pair.p_number
#define i_key pair.p_key

/*
 * file header; kept in the .seq file
 */
struct header
{
	IDX	seq_set;	/* pointer to first leaf */
	IDX	freelist;	/* unused */
	IDX	last_leaf;	/* pointer to last leaf */
	IDX	index_root;	/* pointer to first node */
	int	reads;		/* statistics... */
	int	writes;		/* statistics... */
	int	lookups;	/* statistics... */
	int	inserts;	/* statistics... */
	int	deletes;	/* statistics... */
};
typedef struct header QHEADER;

/*
 * miscellaneous
 */

/* size of the file header not including padding */
#define HEADSIZE sizeof( struct header)

/* size of null pad needed to bring header up to integral LBSIZE blks */
#define PADSIZE ((HEADSIZE / LBSIZE) * LBSIZE + LBSIZE - HEADSIZE)

/* blks taken up by header incl null padding */
#define HEADBLKS ( (HEADSIZE + LBSIZE -1) / LBSIZE )

/* byte offset of a node in the file */
#define NODE_OFFSET(n) (int)(HEADSIZE+PADSIZE+(n-1)*LBSIZE)

#endif
