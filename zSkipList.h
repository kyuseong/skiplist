#ifndef ZSKIPLIST_H
#define ZSKIPLIST_H

#include <assert.h>
#include <stdlib.h>

#define ZSKIPLIST_MAXLEVEL 64 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/4 */

/* Struct to hold a inclusive/exclusive range spec by score comparison. */
struct Range {
	double min, max;
	int minex, maxex; /* are min or max exclusive? */
};

template<class USERDATA, class COMP>
class ZSkipList
{
	COMP Comp;
	struct NODE
	{
		USERDATA ele;
		double score;
		struct NODE* backward;
		struct LEVEL
		{
			NODE* forward;
			size_t span;
		};

		LEVEL* level;

		NODE(int lv)
		{
			level = new LEVEL[lv];
		}

		NODE()
		{
			level = nullptr;
		}

		~NODE() {
			if (level)
				delete[] level;
		}
		NODE* Forward()
		{
			if (level)
			{
				return level[0].forward;
			}
			return nullptr;
		}
		NODE* Backward()
		{	
			return backward;
		}
	};
	NODE* header;
	NODE* tail;
	size_t length;
	int level;
public:
	ZSkipList()
	{
		this->level = 1;
		this->length = 0;
		this->header = CreateNode(ZSKIPLIST_MAXLEVEL, 0, {});
		for (int j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
			this->header->level[j].forward = nullptr;
			this->header->level[j].span = 0;
		}
		this->header->backward = nullptr;
		this->tail = nullptr;
	}

	~ZSkipList()
	{
		NODE* node = this->header->level[0].forward;
		NODE* next;
		delete this->header;
		while (node) {
			next = node->level[0].forward;
			FreeNode(node);
			node = next;
		}
	}


	// 새 점수 추가	
	NODE* Insert(double score, USERDATA ele)
	{
		NODE* update[ZSKIPLIST_MAXLEVEL], * x;
		size_t rank[ZSKIPLIST_MAXLEVEL];
		int i, level;

		assert(!isnan(score));

		x = this->header;
		for (i = this->level - 1; i >= 0; i--) {
			
			rank[i] = i == (this->level - 1) ? 0 : rank[i + 1];
			while (x->level[i].forward &&
				(x->level[i].forward->score < score ||
					(x->level[i].forward->score == score &&
						Comp(x->level[i].forward->ele, ele) < 0)))
			{
				rank[i] += x->level[i].span;
				x = x->level[i].forward;
			}
			update[i] = x;
		}
		
		level = RandomLevel();
		if (level > this->level) {
			for (i = this->level; i < level; i++) {
				rank[i] = 0;
				update[i] = this->header;
				update[i]->level[i].span = this->length;
			}
			this->level = level;
		}
		x = CreateNode(level, score, ele);
		for (i = 0; i < level; i++) {
			x->level[i].forward = update[i]->level[i].forward;
			update[i]->level[i].forward = x;

			
			x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
			update[i]->level[i].span = (rank[0] - rank[i]) + 1;
		}

		for (i = level; i < this->level; i++) {
			update[i]->level[i].span++;
		}

		x->backward = (update[0] == this->header) ? nullptr : update[0];
		if (x->level[0].forward)
			x->level[0].forward->backward = x;
		else
			this->tail = x;
		this->length++;
		return x;
	}

	// 점수 삭제
	int Delete(double score, USERDATA ele, NODE** node)
	{
		NODE* update[ZSKIPLIST_MAXLEVEL];

		NODE* x = this->header;
		for (int i = this->level - 1; i >= 0; i--) {
			while (x->level[i].forward &&
				(x->level[i].forward->score < score ||
					(x->level[i].forward->score == score &&
						Comp(x->level[i].forward->ele, ele) < 0)))
			{
				x = x->level[i].forward;
			}
			update[i] = x;
		}
		
		x = x->level[0].forward;
		if (x && score == x->score && Comp(x->ele, ele) == 0) {
			DeleteNode(x, update);
			if (!node)
				FreeNode(x);
			else
				*node = x;
			return 1;
		}
		return 0; /* not found */
	}

	// 점수 갱신
	NODE* UpdateScore(double curscore, USERDATA ele, double newscore)
	{
		NODE* update[ZSKIPLIST_MAXLEVEL];

		NODE* x = this->header;
		for (int i = this->level - 1; i >= 0; i--) {
			while (x->level[i].forward &&
				(x->level[i].forward->score < curscore ||
					(x->level[i].forward->score == curscore &&
						Comp(x->level[i].forward->ele, ele) < 0)))
			{
				x = x->level[i].forward;
			}
			update[i] = x;
		}

		x = x->level[0].forward;
		assert(x && curscore == x->score && Comp(x->ele, ele) == 0);

		if ((x->backward == nullptr || x->backward->score < newscore) &&
			(x->level[0].forward == nullptr || x->level[0].forward->score > newscore))
		{
			x->score = newscore;
			return x;
		}

		DeleteNode(x, update);
		NODE* newnode = Insert(newscore, x->ele);
		
		x->ele = nullptr;
		FreeNode(x);
		return newnode;
	}

	size_t DeleteRangeByScore(Range* range)
	{
		NODE* update[ZSKIPLIST_MAXLEVEL], * x;
		size_t removed = 0;
		int i;

		x = this->header;
		for (i = this->level - 1; i >= 0; i--) {
			while (x->level[i].forward && (range->minex ?
				x->level[i].forward->score <= range->min :
				x->level[i].forward->score < range->min))
				x = x->level[i].forward;
			update[i] = x;
		}

		/* Current node is the last with score < or <= min. */
		x = x->level[0].forward;

		/* Delete nodes while in range. */
		while (x &&
			(range->maxex ? x->score < range->max : x->score <= range->max))
		{
			NODE* next = x->level[0].forward;
			DeleteNode(x, update);
			FreeNode(x); /* Here is where x->ele is actually released. */
			removed++;
			x = next;
		}
		return removed;
	}
	size_t DeleteRangeByRank(unsigned int start, unsigned int end)
	{
		NODE* update[ZSKIPLIST_MAXLEVEL], * x;
		size_t traversed = 0, removed = 0;
		int i;

		x = this->header;
		for (i = this->level - 1; i >= 0; i--) {
			while (x->level[i].forward && (traversed + x->level[i].span) < start) {
				traversed += x->level[i].span;
				x = x->level[i].forward;
			}
			update[i] = x;
		}

		traversed++;
		x = x->level[0].forward;
		while (x && traversed <= end) {
			NODE* next = x->level[0].forward;
			DeleteNode(x, update);
			FreeNode(x);
			removed++;
			traversed++;
			x = next;
		}
		return removed;
	}

	size_t GetRank(double score, USERDATA ele)
	{
		NODE* x;
		size_t rank = 0;
		int i;

		x = this->header;
		for (i = this->level - 1; i >= 0; i--) {
			while (x->level[i].forward &&
				(x->level[i].forward->score < score ||
					(x->level[i].forward->score == score &&
						Comp(x->level[i].forward->ele, ele) <= 0))) {
				rank += x->level[i].span;
				x = x->level[i].forward;
			}

			/* x might be equal to zsl->header, so test if obj is non-nullptr */
			if (x->ele && Comp(x->ele, ele) == 0) {
				return rank;
			}
		}
		return 0;
	}

	NODE* Head()
	{
		if(this->header->level)
			return this->header->level[0].forward;

		return nullptr;
	}

	NODE* Tail()
	{
		return this->tail;
	}

	NODE* GetElementByRank(size_t rank)
	{
		NODE* x;
		size_t traversed = 0;
		int i;

		x = this->header;
		for (i = this->level - 1; i >= 0; i--) {
			while (x->level[i].forward && (traversed + x->level[i].span) <= rank)
			{
				traversed += x->level[i].span;
				x = x->level[i].forward;
			}
			if (traversed == rank) {
				return x;
			}
		}
		return nullptr;
	}

	size_t GetSize()
	{
		return this->length;
	}

private:

	int RandomLevel(void) {
		int level = 1;
		while ((rand() & 0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
			level += 1;
		return (level < ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
	}

	NODE* CreateNode(int level, double score, USERDATA userdata)
	{
		NODE* zn = new NODE(level);
		zn->score = score;
		zn->ele = userdata;
		return zn;
	}

	void FreeNode(NODE* node) {
		delete node;
	}

	void DeleteNode(NODE* x, NODE** update)
	{
		int i;
		for (i = 0; i < this->level; i++) {
			if (update[i]->level[i].forward == x) {
				update[i]->level[i].span += x->level[i].span - 1;
				update[i]->level[i].forward = x->level[i].forward;
			}
			else {
				update[i]->level[i].span -= 1;
			}
		}
		if (x->level[0].forward) {
			x->level[0].forward->backward = x->backward;
		}
		else {
			this->tail = x->backward;
		}
		while (this->level > 1 && this->header->level[this->level - 1].forward == nullptr)
			this->level--;
		this->length--;
	}
};



#endif // 


/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2009-2012, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
