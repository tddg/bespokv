/*
 * Copyright (c) 2014-2015 Mindaugas Rasiukevicius <rmind at netbsd org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_MASSTREE_H_
#define	_MASSTREE_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#if defined(_KERNEL) || defined(_STANDALONE)
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#endif

#include "utils.h"

__BEGIN_DECLS

/*
 * Version number layout: flags and two counters.
 */

#define	NODE_LOCKED		(1U << 0)	// lock (for the writers)
#define	NODE_INSERTING		(1U << 1)	// "dirty": for inserting
#define	NODE_SPLITTING		(1U << 2)	// "dirty": for splitting
#define	NODE_DELETED		(1U << 3)	// indicate node deletion
#define	NODE_ISROOT		(1U << 4)	// indicate root of B+ tree
#define	NODE_ISBORDER		(1U << 5)	// indicate border node
#define	NODE_DELAYER		(1U << 31)	// layer deletion

/*
 * Note: insert and split counter bit fields are adjacent such that
 * the inserts may overflow into the split.  That is, 7 + 18 bits in
 * total, thus making 2^25 the real overflow.
 */

#define	NODE_VINSERT		0x00001fc0	// insert counter (bits 6-13)
#define	NODE_VINSERT_SHIFT	6

#define	NODE_VSPLIT		0x7fffe000	// split counter (bits 13-31)
#define	NODE_VSPLIT_SHIFT	13

typedef struct mtree_inode mtree_inode_t;
typedef struct mtree_leaf mtree_leaf_t;

/*
 * Poor man's "polymorphism": a structure to access the version field.
 * NODE_ISBORDER determines whether it is interior or border (leaf) node.
 */
typedef struct {
	uint32_t	version;
	unsigned	_pad;
} mtree_node_t;

#define	NODE_MAX	15
#define	NODE_PIVOT	7

struct mtree_inode {
	uint32_t	version;
	uint8_t		nkeys;
	uint64_t	keyslice[NODE_MAX];
	mtree_node_t *	child[NODE_MAX + 1];
	mtree_inode_t *	parent;
	mtree_node_t *	gc_next;
};

struct mtree_leaf {
	uint32_t	version;
	uint16_t	removed;
	uint8_t		keyinfo[NODE_MAX];
	uint64_t	permutation;
	uint64_t	keyslice[NODE_MAX];
	void *		lv[NODE_MAX];
	mtree_leaf_t *	next;

	/*
	 * The following pointers are protected by the lock of the
	 * nodes they are pointing to.  Manipulated only during the
	 * creation, splits and node removal.
	 */
	mtree_leaf_t *	prev;
	mtree_inode_t *	parent;
	mtree_node_t *	gc_next;
};

/*
 * 16 four-bit fields in the 'permutation':
 * - The lower 4 bits hold the number of keys.
 * - The other bits hold a 15-element array which stores key indexes.
 * - The permutation from keyindex[0] to keyindex[nkeys - 1].
 */

#define	PERM_NKEYS(p)		((p) & 0xf)
#define	PERM_KEYIDX(p, i)	(((p) >> (((i) * 4) + 4)) & 0xf)

/*
 * Sequential permutation i.e. PERM_KEYIDX(p, i) == i.
 */
#define	PERM_SEQUENTIAL		(0xedcba98765432100ULL)

/*
 * Two upper bits of the key info store its type, the rest store the key
 * slice length (the maximum is 8).  The MTREE_LAYER flag is included in
 * KEY_LLEN() to distinguish MTREE_VALUE from MTREE_LAYER.
 *
 * Note: MTREE_NOTFOUND is just a dummy value.
 */
#define	KEY_LLEN(l)		((l) & 0x7f)
#define	KEY_TYPE(l)		((l) & 0xc0)

#define	MTREE_VALUE		0x00
#define	MTREE_LAYER		0x40
#define	MTREE_UNSTABLE		0x80
#define	MTREE_NOTFOUND		0xff

typedef struct {
	void *	(*alloc)(size_t);
	void	(*free)(void *, size_t);
} masstree_ops_t;


struct masstree {
	mtree_node_t *		root;
	mtree_node_t *		gc_nodes;
	const masstree_ops_t *	ops;
	mtree_leaf_t		initleaf;
};

struct masstree;
typedef struct masstree masstree_t;
masstree_t *	masstree_create(const masstree_ops_t *);
void		masstree_destroy(masstree_t *);
void *		masstree_gc_prepare(masstree_t *);
void		masstree_gc(masstree_t *, void *);
size_t		masstree_maxheight(void);


static uint32_t stable_version(mtree_node_t *node);

static inline bool node_locked_p(const mtree_node_t *node);

static void lock_node(mtree_node_t *node);

static void unlock_node(mtree_node_t *node);

static mtree_leaf_t * leaf_create(const masstree_t *tree);

static bool leaf_insert_key(mtree_node_t *node, uint64_t key, unsigned kinfo, void *val);

static inline unsigned leaf_find_lv(const mtree_leaf_t *leaf, uint64_t key,    unsigned kinfo, unsigned *type);





void *		masstree_get(masstree_t *, const void *, size_t);
bool		masstree_put(masstree_t *, const void *, size_t, void *);
bool		masstree_del(masstree_t *, const void *, size_t);

__END_DECLS

#endif
