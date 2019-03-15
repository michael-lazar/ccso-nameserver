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
static char  RcsId[] = "@(#)$Id: maket.c,v 1.8 1994/09/09 20:17:46 p-pomes Exp $";
#endif

#include <stdio.h>
#include "protos.h"

extern QHEADER header;
extern IDX last_node;
extern NODE *node_buf;
extern LEAF_DES *leaf_des_buf;
extern int seq_fd;
static char *max_key __P((LEAF *));


IDX
allocate_node()
{
	if (!last_node)
		node_buf = (NODE *) malloc((unsigned) (header.last_leaf + 1) * 2 * NODE_SIZE);
	return (++last_node);
}

IDX
build_tree(first_des, last_des)
	IDX	first_des;
	IDX	last_des;
{
	IDX	root;
	IDX	mid_des;

	root = allocate_node();
	switch (last_des - first_des)
	{
	case 0:
		strncpy(node_buf[root].key, leaf_des_buf[first_des].max_key,
			KEY_SIZE);
		node_buf[root].l_ptr = -leaf_des_buf[first_des].leaf_no;
		node_buf[root].r_ptr = -leaf_des_buf[first_des].leaf_no;
		free(malloc(100));
		return (root);
	case 1:
		strncpy(node_buf[root].key, leaf_des_buf[first_des].max_key,
			KEY_SIZE);
		node_buf[root].l_ptr = -leaf_des_buf[first_des].leaf_no;
		node_buf[root].r_ptr = -leaf_des_buf[last_des].leaf_no;
		free(malloc(100));
		return (root);
	default:
		mid_des = (first_des + last_des) / 2;
		strncpy(node_buf[root].key, leaf_des_buf[mid_des].max_key,
			KEY_SIZE);
		node_buf[root].l_ptr = build_tree(first_des, mid_des);
		node_buf[root].r_ptr = build_tree(mid_des + 1, last_des);
		free(malloc(100));
		return (root);
	}
}

build_leaf_descriptors()
{
	IDX	leaf_index;
	LEAF	leaf;
	int	max_des;

	leaf_des_buf =
	    (LEAF_DES *) malloc((unsigned) sizeof (LEAF_DES) * (header.last_leaf + 1));
	max_des = 0;
	for (leaf_index = header.seq_set; leaf_index; leaf_index = leaf.next)
	{
		max_des++;
		read_leaf(leaf_index, &leaf);
		leaf_des_buf[max_des].leaf_no = leaf.leaf_no;
		strncpy(leaf_des_buf[max_des].max_key, max_key(&leaf), KEY_SIZE);
	}
	free(malloc(100));
	return max_des;
}

static char *
max_key(leaf)
	LEAF *leaf;
{
	int	i;
	static ITEM item;

	for (i = 0; i < leaf -> n_bytes; i += icopy(item.raw, &leaf -> data[i]))
		;
	if (strlen(item.i_key) > 4)
		item.i_key[3]++;
	item.i_key[4] = '\0';
	return item.i_key;
}

write_index(filename)
	char	*filename;
{
	FILE	*fp;
	int	n_items;

	free(malloc(100));
	if (close(creat(filename, 0600)) < 0)
	{
		printf("create() failed in write_index()\n");
		exit(1);
	}
	if ((fp = fopen(filename, "w")) == NULL)
	{
		printf("fopen() failed in write_index()\n");
		exit(1);
	}
	n_items = fwrite((char *) node_buf, NODE_SIZE, last_node + 1, fp);
	if (n_items != last_node + 1)
	{
		printf("fwrite() failed in write_index()\n");
		exit(1);
	}
	printf("%d items (%d bytes each) written on file %s\n",
	       n_items, NODE_SIZE, filename);

}

/*
 * * Create a new sequence set file and intiialize its header.
 */
new_file(filename)
	char	*filename;
{
	LEAF	leaf;

	if (close(creat(filename, 0660)) < 0)
	{
		perror("Creat() failed in new_file()");
		exit(1);
	}
	if ((seq_fd = open(filename, 2)) < 0)
	{
		perror("Open() failed in new_file()");
		exit(1);
	}
	leaf.leaf_no = allocate_leaf();
	leaf.next = 0;
	leaf.n_bytes = 0;

	header.seq_set = leaf.leaf_no;
	header.freelist = 0;
	header.last_leaf = leaf.leaf_no;
	header.index_root = 0;
	header.reads = 0;
	header.writes = 0;
	header.lookups = 0;
	header.inserts = 0;
	header.deletes = 0;

	if (write(seq_fd, (char *) &header, HEADSIZE) < HEADSIZE)
	{
		printf("write() of header failed in new_file()\n");
		exit(1);
	}
	if (lseek(seq_fd, (INT32) (HEADBLKS * LBSIZE - 1), 0) < 0)
	{
		printf("lseek() failed in new_file()\n");
		exit(1);
	}
	if (write(seq_fd, "", 1) < 0)
	{
		printf("write() of null byte failed in new_file()\n");
		exit(1);
	}
	write_leaf(&leaf);

}
