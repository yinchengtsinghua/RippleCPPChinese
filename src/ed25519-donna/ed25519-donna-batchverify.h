
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 ED25519批量验证
**/


#define max_batch_size 64
#define heap_batch_size ((max_batch_size * 2) + 1)

/*第128位在哪边？*/
static const size_t limb128bits = (128 + bignum256modm_bits_per_limb - 1) / bignum256modm_bits_per_limb;

typedef size_t heap_index_t;

typedef struct batch_heap_t {
 /*igned char r[heap_batch_size][16]；/*128位随机值*/
 GE25519点[堆批量大小]；
 bignum256Modm scalars[堆批处理大小]；
 堆索引堆[堆批大小]；
 尺寸大小；
BtChuthHAP；

/*交换堆中的两个值*/

static void
heap_swap(heap_index_t *heap, size_t a, size_t b) {
	heap_index_t temp;
	temp = heap[a];
	heap[a] = heap[b];
	heap[b] = temp;
}

/*将列表末尾的标量添加到堆中*/
static void
heap_insert_next(batch_heap *heap) {
	size_t node = heap->size, parent;
	heap_index_t *pheap = heap->heap;
	bignum256modm *scalars = heap->scalars;

 /*在底部插入*/
	pheap[node] = (heap_index_t)node;

 /*将节点筛选到其排序点*/
	parent = (node - 1) / 2;
	while (node && lt256_modm_batch(scalars[pheap[parent]], scalars[pheap[node]], bignum256modm_limb_size - 1)) {
		heap_swap(pheap, parent, node);
		node = parent;
		parent = (node - 1) / 2;
	}
	heap->size++;
}

/*更新根元素时更新堆*/
static void
heap_updated_root(batch_heap *heap, size_t limbsize) {
	size_t node, parent, childr, childl;
	heap_index_t *pheap = heap->heap;
	bignum256modm *scalars = heap->scalars;

 /*把根筛到底*/
	parent = 0;
	node = 1;
	childl = 1;
	childr = 2;
	while ((childr < heap->size)) {
		node = lt256_modm_batch(scalars[pheap[childl]], scalars[pheap[childr]], limbsize) ? childr : childl;
		heap_swap(pheap, parent, node);
		parent = node;
		childl = (parent * 2) + 1;
		childr = childl + 1;
	}

 /*把根筛回到它的分类点*/
	parent = (node - 1) / 2;
	while (node && lte256_modm_batch(scalars[pheap[parent]], scalars[pheap[node]], limbsize)) {
		heap_swap(pheap, parent, node);
		node = parent;
		parent = (node - 1) / 2;
	}
}

/*使用count元素构建堆，count必须大于等于3*/
static void
heap_build(batch_heap *heap, size_t count) {
	heap->heap[0] = 0;
	heap->size = 0;
	while (heap->size < count)
		heap_insert_next(heap);
}

/*扩展堆以包含新的\u count元素*/
static void
heap_extend(batch_heap *heap, size_t new_count) {
	while (heap->size < new_count)
		heap_insert_next(heap);
}

/*获取堆的前2个元素*/
static void
heap_get_top2(batch_heap *heap, heap_index_t *max1, heap_index_t *max2, size_t limbsize) {
	heap_index_t h0 = heap->heap[0], h1 = heap->heap[1], h2 = heap->heap[2];
	if (lt256_modm_batch(heap->scalars[h1], heap->scalars[h2], limbsize))
		h1 = h2;
	*max1 = h0;
	*max2 = h1;
}

/**/
static void
ge25519_multi_scalarmult_vartime_final(ge25519 *r, ge25519 *point, bignum256modm scalar) {
	const bignum256modm_element_t topbit = ((bignum256modm_element_t)1 << (bignum256modm_bits_per_limb - 1));
	size_t limb = limb128bits;
	bignum256modm_element_t flag;

	if (isone256_modm_batch(scalar)) {
  /*这在卡特之后的大部分时间都会发生*/
		*r = *point;
		return;
	} else if (iszero256_modm_batch(scalar)) {
  /*只有当所有scalars==0时才会发生这种情况。*/
		memset(r, 0, sizeof(*r));
		r->y[0] = 1;
		r->z[0] = 1;
		return;
	}

	*r = *point;

 /*找到设置第一个位的肢体*/
	while (!scalar[limb])
		limb--;

 /*找到第一个位*/
	flag = topbit;
	while ((scalar[limb] & flag) == 0)
		flag >>= 1;

 /*指数*/
	for (;;) {
		ge25519_double(r, r);
		if (scalar[limb] & flag)
			ge25519_add(r, r, point);

		flag >>= 1;
		if (!flag) {
			if (!limb--)
				break;
			flag = topbit;
		}
	}
}

/*计数必须大于等于5*/
static void
ge25519_multi_scalarmult_vartime(ge25519 *r, batch_heap *heap, size_t count) {
	heap_index_t max1, max2;

 /*从完整的肢体尺寸开始*/
	size_t limbsize = bignum256modm_limb_size - 1;

 /*堆是否已扩展为包含128位标量*/
	int extended = 0;

 /*抓取奇数个scalar来构建堆，未知的分支大小*/
	heap_build(heap, ((count + 1) / 2) | 1);

	for (;;) {
		heap_get_top2(heap, &max1, &max2, limbsize);

  /*只剩下一个标量，我们完成了*/
		if (iszero256_modm_batch(heap->scalars[max2]))
			break;

  /*又一条腿筋疲力尽了？*/
		if (!heap->scalars[max1][limbsize])
			limbsize -= 1;

  /*我们可以扩展到128位标量吗？*/
		if (!extended && isatmost128bits256_modm_batch(heap->scalars[max1])) {
			heap_extend(heap, count);
			heap_get_top2(heap, &max1, &max2, limbsize);
			extended = 1;
		}

		sub256_modm_batch(heap->scalars[max1], heap->scalars[max1], heap->scalars[max2], limbsize);
		ge25519_add(&heap->points[max2], &heap->points[max2], &heap->points[max1]);
		heap_updated_root(heap, limbsize);
	}

	ge25519_multi_scalarmult_vartime_final(r, &heap->points[max1], heap->scalars[max1]);
}

/*除了测试之外，没有实际使用*/
unsigned char batch_point_buffer[3][32];

static int
ge25519_is_neutral_vartime(const ge25519 *p) {
	static const unsigned char zero[32] = {0};
	unsigned char point_buffer[3][32];
	curve25519_contract(point_buffer[0], p->x);
	curve25519_contract(point_buffer[1], p->y);
	curve25519_contract(point_buffer[2], p->z);
	memcpy(batch_point_buffer[1], point_buffer[1], 32);
	return (memcmp(point_buffer[0], zero, 32) == 0) && (memcmp(point_buffer[1], point_buffer[2], 32) == 0);
}

int
ED25519_FN(ed25519_sign_open_batch) (const unsigned char **m, size_t *mlen, const unsigned char **pk, const unsigned char **RS, size_t num, int *valid) {
	batch_heap ALIGN(16) batch;
	ge25519 ALIGN(16) p;
	bignum256modm *r_scalars;
	size_t i, batchsize;
	unsigned char hram[64];
	int ret = 0;

	for (i = 0; i < num; i++)
		valid[i] = 1;

	while (num > 3) {
		batchsize = (num > max_batch_size) ? max_batch_size : num;

  /*生成r（scalars[batchsize+1]。.scalars[2*batchsize]*/
		ED25519_FN(ed25519_randombytes_unsafe) (batch.r, batchsize * 16);
		r_scalars = &batch.scalars[batchsize + 1];
		for (i = 0; i < batchsize; i++)
			expand256_modm(r_scalars[i], batch.r[i], 16);

  /*计算标量[0]=（（r1s1+r2s2+…）*/
		for (i = 0; i < batchsize; i++) {
			expand256_modm(batch.scalars[i], RS[i] + 32, 32);
			mul256_modm(batch.scalars[i], batch.scalars[i], r_scalars[i]);
		}
		for (i = 1; i < batchsize; i++)
			add256_modm(batch.scalars[0], batch.scalars[0], batch.scalars[i]);

  /*计算scalars[1]。.scalars[batchsize]为r[i]*h（r[i]，a[i]，m[i]）*/
		for (i = 0; i < batchsize; i++) {
			ed25519_hram(hram, RS[i], pk[i], m[i], mlen[i]);
			expand256_modm(batch.scalars[i+1], hram, 64);
			mul256_modm(batch.scalars[i+1], batch.scalars[i+1], r_scalars[i]);
		}

  /*计算点*/
		batch.points[0] = ge25519_basepoint;
		for (i = 0; i < batchsize; i++)
			if (!ge25519_unpack_negative_vartime(&batch.points[i+1], pk[i]))
				goto fallback;
		for (i = 0; i < batchsize; i++)
			if (!ge25519_unpack_negative_vartime(&batch.points[batchsize+i+1], RS[i]))
				goto fallback;

		ge25519_multi_scalarmult_vartime(&p, &batch, (batchsize * 2) + 1);
		if (!ge25519_is_neutral_vartime(&p)) {
			ret |= 2;

			fallback:
			for (i = 0; i < batchsize; i++) {
				valid[i] = ED25519_FN(ed25519_sign_open) (m[i], mlen[i], pk[i], RS[i]) ? 0 : 1;
				ret |= (valid[i] ^ 1);
			}
		}

		m += batchsize;
		mlen += batchsize;
		pk += batchsize;
		RS += batchsize;
		num -= batchsize;
		valid += batchsize;
	}

	for (i = 0; i < num; i++) {
		valid[i] = ED25519_FN(ed25519_sign_open) (m[i], mlen[i], pk[i], RS[i]) ? 0 : 1;
		ret |= (valid[i] ^ 1);
	}

	return ret;
}

