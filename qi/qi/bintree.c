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
 */

#ifndef lint
static char  RcsId[] = "@(#)$Id: bintree.c,v 1.30 1995/01/09 13:14:21 p-pomes Exp $";
#endif

#include "protos.h"

#define GRANULE 64
#define INCR(x) ((unsigned)((x+=GRANULE)*sizeof(INT32)))

QHEADER	header;			/* header from .seq file */
IDX	last_node;		/* */
NODE	*node_buf = NULL;	/* array containing all NODEs */
LEAF_DES *leaf_des_buf;		/* array containing all LEAF_DESs */
static LEAF Leaf;		/* leaf currently in memory */
static int Leaf_dirty;		/* has Leaf been modified? */
int	seq_fd;			/* fd of .seq file */

static void flush_leaf();
static void simple_insert __P((LEAF *, int, ITEM *, int));
static IDX start_point __P((char *));
static char *to_low_str __P((char *));

/*
 * convert a string to lower case
 */
static char *
to_low_str(string)
	char *string;
{
	register char *s;

	for (s = string; *s; s++)
		if (*s >= 'A' && *s <= 'A')
			*s += 'a' - 'A';
	return (string);
}

/*
 * initialize; open and read in the header of the .seq (leaf) file
 */
void 
bintree_init(fname)
	char *fname;
{
	char	buf[100];

	(void) sprintf(buf, "%s.seq", fname);

	if ((seq_fd = open(buf, 2)) < 0)
	{
		IssueMessage(LOG_ERR, "bintree_init: open(%s): %s",
			buf, strerror(errno));
		cleanup(-1);
	}
	get_tree_head();
}

/*
 * cleanup; close the .seq file
 */
void 
close_tree()
{
	(void) close(seq_fd);
}

/*
 * read the header from the .seq file
 */
void 
get_tree_head()
{
	if (lseek(seq_fd, 0L, 0) < 0)
	{
		IssueMessage(LOG_ERR, "get_tree_head: lseek(%d,0L,0): %s",
			seq_fd, strerror(errno));
		cleanup(-1);
	}
	else if (read(seq_fd, (char *) &header, HEADSIZE) != HEADSIZE)
	{
		IssueMessage(LOG_ERR, "get_tree_head: read: %s",
			strerror(errno));
		cleanup(-1);
	}
}

/*
 * make sure that the given leaf contains the leaf numbered num
 */
void 
read_leaf(num, leaf)
	IDX num;
	LEAF *leaf;
{
	INT32	offset;

	if (&Leaf == leaf)	/* are we filling Leaf? */
	{
		if (Leaf.leaf_no == num)	/* is the right leaf already there? */
		{
			return;
		}
		flush_leaf();	/* write Leaf out if it's dirty */
	}
	offset = NODE_OFFSET(num);	/* turn index into disk address */
	if (lseek(seq_fd, offset, 0) < 0)
	{
		IssueMessage(LOG_ERR, "read_leaf: lseek(%d,%ld,0): %s",
			seq_fd, offset, strerror(errno));
		cleanup(-1);
	}
	else if (read(seq_fd, (char *) leaf, LBSIZE) != LBSIZE)
	{
		IssueMessage(LOG_ERR, "read_leaf: read: %s", strerror(errno));
		IssueMessage(LOG_ERR, "leaf index was %d, offset was %d", num, offset);
		cleanup(-1);
	}
}

/*
 * write out Leaf IFF it is dirty
 */
static void 
flush_leaf()
{
	if (Leaf_dirty)
		write_leaf(&Leaf);
	Leaf_dirty = 0;
}

/*
 * write out the given leaf
 */
void 
write_leaf(leaf)
	LEAF *leaf;
{
	if (lseek(seq_fd, NODE_OFFSET(leaf->leaf_no), 0) < 0)
	{
		IssueMessage(LOG_ERR, "write_leaf: lseek(%d,%ld,0): %s",
			seq_fd, NODE_OFFSET(leaf->leaf_no), strerror(errno));
		cleanup(-1);
	}
	else if (write(seq_fd, (char *) leaf, LBSIZE) != LBSIZE)
	{
		IssueMessage(LOG_ERR, "write_leaf: write: %s", strerror(errno));
		cleanup(-1);
	}
}

/*
 * copy an ITEM.  Since an item is an IDX followed by a null-terminated
 * string, we first copy an IDX' worth, then copy until we find a null
 * or hit the ITEM size limit. Returns the number of bytes copied.
 */
int 
icopy(dest, src)
	char *dest, *src;
{
	register int i;

	/*
	 * copy the first few bytes (the index)
	 */
	for (i = 0; i < sizeof (IDX); i++)
		*dest++ = *src++;

	/*
	 * copy the string until NULL or size limit
	 */
	while ((*dest++ = *src++) && (i < sizeof (ITEM)))
		i++;
	if (i >= sizeof (ITEM))
	{
		/* uh-oh */
		IssueMessage(LOG_ERR, "icopy: failed, item full");
		cleanup(-1);
	}
	return ++i;
}

/*
 * find the first leaf that might possibly contain the given key
 */
static IDX 
start_point(key)
	char *key;
{
	IDX	idx;
	char	simp_key[256], *ptr;

	/* no node tree (.bdx), we have to start with the first leaf node */
	if (!node_buf)
		return header.seq_set;

	/* get substring of the key which doesn't include any meta characters */
	(void) strcpy(simp_key, key);
	if (ptr = strchr(simp_key, '*'))
		*ptr = '\0';
	if (ptr = strchr(simp_key, '?'))
		*ptr = '\0';
	if (ptr = strchr(simp_key, '['))
		*ptr = '\0';

	/* search NODEs: postitve ptrs go to more nodes, neg to leaves */
	idx = header.index_root;/* start at the top */
	while (idx > 0)
	{
		if (strncmp(simp_key, node_buf[idx].key, KEY_SIZE) <= 0)
			idx = node_buf[idx].l_ptr;
		else
			idx = node_buf[idx].r_ptr;
	}

	/* something wrong if null ptr, but can still search whole seq set */
	if (idx == 0)
	{
		IssueMessage(LOG_WARNING, "start_point: index search led to a NULL leaf");
		return header.seq_set;
	}
	return (-idx);
}

/*
 * find a key in the leaves (or a needle in a haystack...)
 * no metacharacters in key
 */
int 
search(key, leaf, offset)
	char *key;
	LEAF *leaf;
	int *offset;
{
	IDX	cur_idx;
	int	result;

	cur_idx = start_point(key);	/* use .bdx to find first possible leaf */
	/*
	 * loop through leaf string
	 */
	while (cur_idx)
	{

		read_leaf(cur_idx, leaf);	/* read it in */
		*offset = 0;
		/*
		 * loop through all the ITEMs in the leaf
		 */
		while (*offset < leaf -> n_bytes)
		{
			result = stricmp(&leaf -> data[*offset + IDX_SIZE], key);
			if (result < 0)
			{
				/* ITEM too small; next one will be bigger; try it */
				*offset += sizeof (IDX);
				while (leaf -> data[(*offset)++])
					;
			} else if (result == 0)
			{
				return MATCH;	/* we found it! */
			} else
			{
				return NO_MATCH;	/* not here, but belongs here */
			}
		}
		cur_idx = leaf -> next; /* on to the next leaf */
	}
	/*
	 * not found in whole leaf string; belongs on the end of the last leaf
	 * in the string
	 */
	return NO_MATCH;
}

/*
 * ``remove'' the ITEM belonging to a string; actually just sets its
 * pointer to NULL to indicate that it's dead; does not actually delete
 * the string.  the appropriate leaf will be left in Leaf.
 */
int 
qidelete(key)
	char *key;
{
	int	offset;	/* byte offset where found */
	ITEM	item;		/* scratch item */

	/* Find the key and null out its data */
	if (search(key, &Leaf, &offset) == MATCH)
	{
		(void) icopy(item.raw, &Leaf.data[offset]);
		item.i_num = 0;
		(void) icopy(&Leaf.data[offset], item.raw);
		Leaf_dirty++;
		return (1);
	} else
	{
		IssueMessage(LOG_ERR, "qidelete: non-existent key (%s)", key);
		return (0);
	}
}

/*
 * add a key to the .seq file.  MUST not be there already.
 */
void 
insert(key, data)
	char *key;
	IDX data;
{
	ITEM	new_item;
	int	offset, item_size, code;

	/* Set leaf and offset to where this key belongs */
	code = search(key, &Leaf, &offset);

	/*
	 * if we found the string, it should be because we have deleted
	 * it before, and the data should be null.  In that case, all
	 * we have to do is set the data to what it should be
	 */
	if (code == MATCH)
	{
		(void) icopy(new_item.raw, &Leaf.data[offset]);
		if (new_item.i_num)
			IssueMessage(LOG_ERR, "insert: \"%s\" already in tree", key);
		else
		{
			new_item.i_num = data;
			(void) icopy(&Leaf.data[offset], new_item.raw);
			Leaf_dirty++;
		}
		return;
	}
	/*
	 * key not in .seq file.  put it into an item to be put into a leaf
	 */
	new_item.i_num = data;
	strcpy(new_item.i_key, to_low_str(key));
	item_size = strlen(key) + 1 + sizeof (IDX);

	/*
	 * stick our pretty new ITEM into Leaf
	 */
	if (item_size + Leaf.n_bytes <= DATA_SIZE)
		/* we can just tack it onto the end */
		simple_insert(&Leaf, offset, &new_item, item_size);
	else
		/* no room--split up the leaf and then insert the item */
		expand(&Leaf, offset, &new_item);
}

/*
 * insert a new ITEM into a LEAF at the proper spot.  There is room in
 * the LEAF.
 */
static void 
simple_insert(leaf, offset, item, item_size)
	LEAF *leaf;
	ITEM *item;
	int offset, item_size;
{
	register int src, dest;

	/* move existing data over to allow room for new_item */
	src = leaf -> n_bytes - 1;
	dest = src + item_size;
	while (src >= offset)
		leaf -> data[dest--] = leaf -> data[src--];

	/* put in the new item && write out the leaf */
	(void) icopy(&leaf -> data[offset], item -> raw);
	leaf -> n_bytes += item_size;
	Leaf_dirty++;
}

/*
 * split a leaf in two and then insert the item into one of the halves
 */
void 
expand(leaf, offset, item)
	LEAF *leaf;
	int offset;
	ITEM *item;
{
	LEAF	new_leaf;	/* room for the newly split leaf */
	char	buf[2 * DATA_SIZE];	/* some space */
	int	size, buf_size;
	register int src, dest;

	/* link in the new leaf */
	new_leaf.leaf_no = allocate_leaf();
	new_leaf.next = leaf -> next;
	leaf -> next = new_leaf.leaf_no;

	/* collect all the data into buf */
	src = 0;
	dest = 0;
	while (src < offset)
		buf[dest++] = leaf -> data[src++];
	dest += icopy(&buf[dest], item -> raw);
	while (src < leaf -> n_bytes)
		buf[dest++] = leaf -> data[src++];
	buf_size = dest;

	/* put the first half of the data into original leaf */
	for (src = 0, dest = 0; dest < DATA_SIZE / 2;)
	{
		src += icopy(&leaf -> data[src], &buf[src]);
		dest = src;
	}
	leaf -> n_bytes = dest;
	Leaf_dirty++;

	/* put the second half of the data into the new leaf */
	for (dest = 0; src < buf_size;)
	{
		size = icopy(&new_leaf.data[dest], &buf[src]);
		src += size;
		dest += size;
	}
	new_leaf.n_bytes = dest;

	/* write the new leaf out, but let Leaf_dirty do the trick for Leaf */
	write_leaf(&new_leaf);
}

/*
 * find all entries that match a string with metacharacters in it
 */
INT32 *
find_all(key)
	char *key;
{
	int	offset = 0;	/* offset in leaf */
	ITEM	item;		/* space for an item */
	char	lcase_key[256];/* the key in lower case */
	INT32	*array = NULL;	/* our result */

	/*
	 * make a lower-case copy of the string
	 */
	strcpy(lcase_key, key);
	to_low_str(lcase_key);

	/*
	 * find the first leaf that might contain the key, and read it in
	 */
	read_leaf(start_point(lcase_key), &Leaf);
	for (;;)
	{
		if (offset >= Leaf.n_bytes)	/* out of data in this leaf? */
		{
			/* on to the next leaf, if there is one */
			if (Leaf.next)
			{
				read_leaf(Leaf.next, &Leaf);
				offset = 0;
			} else
				break;	/* all done */
		}
		/*
		 * grab the next ITEM
		 */
		offset += icopy(item.raw, &Leaf.data[offset]);

		/*
		 * compare
		 */
		switch (pmatch(item.i_key, lcase_key))
		{
		    case MATCH:
			if (item.i_num)
			{
				/*
				 * we found a match, and the item has not been
				 * deleted.  add the entries pointed to by the
				 * .idx file entry pointed to by i_num to our
				 * current list.
				 */
				array = merge(array, get_dir_ptrs(item.i_num));
			}
			break;
		    case NO_MATCH:
			return array;	/* all done */
		    case CONTINUE:
			break;
		}
	}
	return array;
}

/*
 * Write out the current file header and close.
 */
void 
put_tree_head()
{

	if (lseek(seq_fd, 0L, 0) < 0)
		IssueMessage(LOG_ERR, "put_tree_head: lseek(%d,0L,0): %s",
			seq_fd, strerror(errno));
	else if ((write(seq_fd, (char *) &header, HEADSIZE)) != HEADSIZE)
		IssueMessage(LOG_ERR, "put_tree_head: write: %s",
			strerror(errno));
	flush_leaf();
	close(seq_fd);
}

/*
 * add a leaf on the end of the .seq file.  The actual adding to the file
 * will happen when the leaf is written
 */
IDX 
allocate_leaf()
{
	return (++header.last_leaf);
}

/*
 * read the entire .bdx (node) file into node_buf
 */
void 
read_index(fname)
	char *fname;
{
	FILE	*fp;
	int	n_read, n_items;
	unsigned int ask = (2 * header.last_leaf * NODE_SIZE);
	char	buf[100];

	(void) sprintf(buf, "%s.bdx", fname);

	if ((fp = fopen(buf, "r")) == NULL)
	{
		IssueMessage(LOG_ERR, "read_index: fopen(%s): %s",
			buf, strerror(errno));
		cleanup(-1);
	}
	if (node_buf)
	{
		free(node_buf);
		node_buf = NULL;
	}
	if ((node_buf = (NODE *) malloc(ask)) == NULL)
	{
		IssueMessage(LOG_ERR, "read_index: malloc(%d): %s",
			ask, strerror(errno));
		cleanup(-1);
	}
	memset((void *) node_buf, (char)0, ask);
	(void) fseek(fp, 0L, 2);
	n_items = ftell(fp) / NODE_SIZE;
	if (ask < NODE_SIZE * n_items)
	{
		IssueMessage(LOG_ERR, "read_index: malloc(%d) too small (%d)",
			ask, NODE_SIZE * n_items);
		cleanup(-1);
	}
	rewind(fp);
	n_read = fread((char *) node_buf, NODE_SIZE, n_items, fp);
	if (n_read != n_items)
	{
		IssueMessage(LOG_ERR, "read_index: fread: %s",
			n_items, n_read, strerror(errno));
		cleanup(-1);
	}
	(void) fclose(fp);
}

/*
 * Merge together 2 array of INT32.  The inputs are assumed to be
 * already sorted and null terminted.  Each input array is assumed not to
 * contain dupes, although the same number may occur in both inputs.  The
 * output has dups elided.
 */
INT32 *
merge(ary1, ary2)
	INT32 *ary1, *ary2;
{
	INT32	*out, *orig_1, *orig_2;
	register int i = 0;
	unsigned size = 0;

	/* Check for null inputs, note that output may be null */
	if (ary1 == NULL)
		return ary2;
	if (ary2 == NULL)
		return ary1;

	/* Save inputs for later freeing, allocate space for output */
	orig_1 = ary1;
	orig_2 = ary2;
	out = (INT32 *) malloc(INCR(size));

	/* put min ele from either input onto output til one input exhausted */
	while (*ary1 && *ary2)
	{
		if (i >= size - 2)
			out = (INT32 *) realloc((char *) out, INCR(size));
		if (*ary1 < *ary2)
			out[i++] = *ary1++;
		else
			out[i++] = *ary2++;
		if (*ary1 == out[i - 1])
			ary1++;
	}

	/* move whatever is left of ary1 to output */
	while (*ary1)
	{
		if (i >= size - 2)
			out = (INT32 *) realloc((char *) out, INCR(size));
		out[i++] = *ary1++;
	}

	/* move whatever is left of ary2 to output */
	while (*ary2)
	{
		if (i >= size - 2)
			out = (INT32 *) realloc((char *) out, INCR(size));
		out[i++] = *ary2++;
	}

	/* null terminate output, and free inputs */
	out[i] = 0;
	free((char *) orig_1);
	free((char *) orig_2);
	orig_1 = orig_2 = 0;
	return out;
}
