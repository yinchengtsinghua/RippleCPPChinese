
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 转换
**/


static void
ge25519_p1p1_to_partial(ge25519 *r, const ge25519_p1p1 *p) {
	packed64bignum25519 ALIGN(16) xz, tt, xzout;
	curve25519_mul(r->y, p->y, p->z);
	curve25519_tangle64(xz, p->x, p->z);
	curve25519_tangleone64(tt, p->t);
	curve25519_mul_packed64(xzout, xz, tt);
	curve25519_untangle64(r->x, r->z, xzout);
}

static void 
ge25519_p1p1_to_full(ge25519 *r, const ge25519_p1p1 *p) {
	packed64bignum25519 ALIGN(16) zy, xt, xx, zz, ty;
	curve25519_tangle64(ty, p->t, p->y);
	curve25519_tangleone64(xx, p->x);
	curve25519_mul_packed64(xt, xx, ty);
	curve25519_untangle64(r->x, r->t, xt);
	curve25519_tangleone64(zz, p->z);
	curve25519_mul_packed64(zy, zz, ty);
	curve25519_untangle64(r->z, r->y, zy);
}

static void
ge25519_full_to_pniels(ge25519_pniels *p, const ge25519 *r) {
	curve25519_sub(p->ysubx, r->y, r->x);
	curve25519_add(p->xaddy, r->x, r->y);
	curve25519_copy(p->z, r->z);
	curve25519_mul(p->t2d, r->t, ge25519_ec2d);
}

/*
 添加和加倍
**/


static void
ge25519_add_p1p1(ge25519_p1p1 *r, const ge25519 *p, const ge25519 *q) {
	bignum25519 ALIGN(16) a,b,c,d;
	packed32bignum25519 ALIGN(16) xx, yy, yypxx, yymxx, bd, ac, bdmac, bdpac;
	packed64bignum25519 ALIGN(16) at, bu, atbu, ptz, qtz, cd;

	curve25519_tangle32(yy, p->y, q->y);
	curve25519_tangle32(xx, p->x, q->x);
	curve25519_add_packed32(yypxx, yy, xx);
	curve25519_sub_packed32(yymxx, yy, xx);
	curve25519_tangle64_from32(at, bu, yymxx, yypxx);
	curve25519_mul_packed64(atbu, at, bu);
	curve25519_untangle64(a, b, atbu);
	curve25519_tangle64(ptz, p->t, p->z);
	curve25519_tangle64(qtz, q->t, q->z);
	curve25519_mul_packed64(cd, ptz, qtz);
	curve25519_untangle64(c, d, cd);
	curve25519_mul(c, c, ge25519_ec2d);
	curve25519_add_reduce(d, d, d);
 /*减少，所以以后不需要“基本”后*/
	curve25519_tangle32(bd, b, d);
	curve25519_tangle32(ac, a, c);
	curve25519_sub_packed32(bdmac, bd, ac);
	curve25519_add_packed32(bdpac, bd, ac);
	curve25519_untangle32(r->x, r->t, bdmac);
	curve25519_untangle32(r->y, r->z, bdpac);
}


static void
ge25519_double_p1p1(ge25519_p1p1 *r, const ge25519 *p) {
	bignum25519 ALIGN(16) a,b,c,x;
	packed64bignum25519 ALIGN(16) xy, zx, ab, cx;
	packed32bignum25519 ALIGN(16) xc, yz, xt, yc, ac, bc;

	curve25519_add(x, p->x, p->y);
	curve25519_tangle64(xy, p->x, p->y);
	curve25519_square_packed64(ab, xy);
	curve25519_untangle64(a, b, ab);
	curve25519_tangle64(zx, p->z, x);
	curve25519_square_packed64(cx, zx);
	curve25519_untangle64(c, x, cx);
	curve25519_tangle32(bc, b, c);
	curve25519_tangle32(ac, a, c);
	curve25519_add_reduce_packed32(yc, bc, ac);
	curve25519_untangle32(r->y, c, yc);
	curve25519_sub(r->z, b, a);
	curve25519_tangle32(yz, r->y, r->z);
	curve25519_tangle32(xc, x, c);
	curve25519_sub_after_basic_packed32(xt, xc, yz);
	curve25519_untangle32(r->x, r->t, xt);
}

static void
ge25519_nielsadd2_p1p1(ge25519_p1p1 *r, const ge25519 *p, const ge25519_niels *q, unsigned char signbit) {
	const bignum25519 *qb = (const bignum25519 *)q;
	bignum25519 *rb = (bignum25519 *)r;
	bignum25519 ALIGN(16) a,b,c;
	packed64bignum25519 ALIGN(16) ab, yx, aybx;
	packed32bignum25519 ALIGN(16) bd, ac, bdac;

	curve25519_sub(a, p->y, p->x);
	curve25519_add(b, p->y, p->x);
	curve25519_tangle64(ab, a, b);
	curve25519_tangle64(yx, qb[signbit], qb[signbit^1]);
	curve25519_mul_packed64(aybx, ab, yx);
	curve25519_untangle64(a, b, aybx);
	curve25519_add(r->y, b, a);
	curve25519_add_reduce(r->t, p->z, p->z);
	curve25519_mul(c, p->t, q->t2d);
	curve25519_copy(r->z, r->t);
	curve25519_add(rb[2+signbit], rb[2+signbit], c);
	curve25519_tangle32(bd, b, rb[2+(signbit^1)]);
	curve25519_tangle32(ac, a, c);
	curve25519_sub_packed32(bdac, bd, ac);
	curve25519_untangle32(r->x, rb[2+(signbit^1)], bdac);
}

static void
ge25519_pnielsadd_p1p1(ge25519_p1p1 *r, const ge25519 *p, const ge25519_pniels *q, unsigned char signbit) {
	const bignum25519 *qb = (const bignum25519 *)q;
	bignum25519 *rb = (bignum25519 *)r;
	bignum25519 ALIGN(16) a,b,c;
	packed64bignum25519 ALIGN(16) ab, yx, aybx, zt, zt2d, tc;
	packed32bignum25519 ALIGN(16) bd, ac, bdac;

	curve25519_sub(a, p->y, p->x);
	curve25519_add(b, p->y, p->x);
	curve25519_tangle64(ab, a, b);
	curve25519_tangle64(yx, qb[signbit], qb[signbit^1]);
	curve25519_mul_packed64(aybx, ab, yx);
	curve25519_untangle64(a, b, aybx);
	curve25519_add(r->y, b, a);
	curve25519_tangle64(zt, p->z, p->t);
	curve25519_tangle64(zt2d, q->z, q->t2d);
	curve25519_mul_packed64(tc, zt, zt2d);
	curve25519_untangle64(r->t, c, tc);
	curve25519_add_reduce(r->t, r->t, r->t);
	curve25519_copy(r->z, r->t);
	curve25519_add(rb[2+signbit], rb[2+signbit], c);
	curve25519_tangle32(bd, b, rb[2+(signbit^1)]);
	curve25519_tangle32(ac, a, c);
	curve25519_sub_packed32(bdac, bd, ac);
	curve25519_untangle32(r->x, rb[2+(signbit^1)], bdac);
}

static void
ge25519_double(ge25519 *r, const ge25519 *p) {
	ge25519_p1p1 ALIGN(16) t;
	ge25519_double_p1p1(&t, p);
	ge25519_p1p1_to_full(r, &t);
}

static void
ge25519_add(ge25519 *r, const ge25519 *p, const ge25519 *q) {
	ge25519_p1p1 ALIGN(16) t;
	ge25519_add_p1p1(&t, p, q);
	ge25519_p1p1_to_full(r, &t);
}

static void
ge25519_double_partial(ge25519 *r, const ge25519 *p) {
	ge25519_p1p1 ALIGN(16) t;
	ge25519_double_p1p1(&t, p);
	ge25519_p1p1_to_partial(r, &t);
}

static void
ge25519_nielsadd2(ge25519 *r, const ge25519_niels *q) {
	packed64bignum25519 ALIGN(16) ab, yx, aybx, eg, ff, hh, xz, ty;
	packed32bignum25519 ALIGN(16) bd, ac, bdac;
	bignum25519 ALIGN(16) a,b,c,d,e,f,g,h;

	curve25519_sub(a, r->y, r->x);
	curve25519_add(b, r->y, r->x);
	curve25519_tangle64(ab, a, b);
	curve25519_tangle64(yx, q->ysubx, q->xaddy);
	curve25519_mul_packed64(aybx, ab, yx);
	curve25519_untangle64(a, b, aybx);
	curve25519_add(h, b, a);
	curve25519_add_reduce(d, r->z, r->z);
	curve25519_mul(c, r->t, q->t2d);
 /*ve25519_-add（g，d，c）；/*d减少，因此无需在_-basic*之后
 曲线25519_tangle32（b d，b，d）；
 曲线25519_tangle32（AC，A，C）；
 curve25519_sub_packed32（bd ac，bd，ac）；/*d减少，因此无需在_basic之后*/

	curve25519_untangle32(e, f, bdac);
	curve25519_tangle64(eg, e, g);
	curve25519_tangleone64(ff, f);
	curve25519_mul_packed64(xz, eg, ff);
	curve25519_untangle64(r->x, r->z, xz);
	curve25519_tangleone64(hh, h);
	curve25519_mul_packed64(ty, eg, hh);
	curve25519_untangle64(r->t, r->y, ty);
}

static void
ge25519_pnielsadd(ge25519_pniels *r, const ge25519 *p, const ge25519_pniels *q) {
	ge25519_p1p1 ALIGN(16) t;
	ge25519 ALIGN(16) f;
	ge25519_pnielsadd_p1p1(&t, p, q, 0);
	ge25519_p1p1_to_full(&f, &t);
	ge25519_full_to_pniels(r, &f);
}

/*
 包装与解包
**/


static void
ge25519_pack(unsigned char r[32], const ge25519 *p) {
	bignum25519 ALIGN(16) tx, ty, zi;
	unsigned char parity[32];
	curve25519_recip(zi, p->z);
	curve25519_mul(tx, p->x, zi);
	curve25519_mul(ty, p->y, zi);
	curve25519_contract(r, ty);
	curve25519_contract(parity, tx);
	r[31] ^= ((parity[0] & 1) << 7);
}


static int
ge25519_unpack_negative_vartime(ge25519 *r, const unsigned char p[32]) {
	static const bignum25519 ALIGN(16) one = {1};
	static const unsigned char zero[32] = {0};
	unsigned char parity = p[31] >> 7;
	unsigned char check[32];
	bignum25519 ALIGN(16) t, root, num, den, d3;

	curve25519_expand(r->y, p);
	curve25519_copy(r->z, one);
 /*ve25519_square_times（num，r->y，1）；/*x=y^2*/
 曲线25519_mul（den，num，ge25519_ecd）；/*den=dy^2*/

 /*ve25519_sub_reduce（num，num，r->z）；/*x=y^2-1*/
 曲线25519_-add（den，den，r->z）；/*den=dy^2+1*/


 /*sqrt计算（num/den）*/
 /*1.：计算num^（（p-5）/8）*den^（（7p-35）/8）=（num*den^7）^（（p-5）/8）*/
	curve25519_square_times(t, den, 1);
	curve25519_mul(d3, t, den);
	curve25519_square_times(r->x, d3, 1);
	curve25519_mul(r->x, r->x, den);
	curve25519_mul(r->x, r->x, num);
	curve25519_pow_two252m3(r->x, r->x);

 /*2。r->x=t*num*den^3的计算*/
	curve25519_mul(r->x, r->x, d3);
	curve25519_mul(r->x, r->x, num);

 /*三。检查任一根是否工作：*/
	curve25519_square_times(t, r->x, 1);
	curve25519_mul(t, t, den);
	curve25519_copy(root, t);
	curve25519_sub_reduce(root,  root, num);
	curve25519_contract(check, root);
	if (!ed25519_verify(check, zero, 32)) {
		curve25519_add_reduce(t, t, num);
		curve25519_contract(check, t);
		if (!ed25519_verify(check, zero, 32))
			return 0;
		curve25519_mul(r->x, r->x, ge25519_sqrtneg1);
	}

	curve25519_contract(check, r->x);
	if ((check[0] & 1) == parity) {
		curve25519_copy(t, r->x);
		curve25519_neg(r->x, t);
	}
	curve25519_mul(r->t, r->x, r->y);
	return 1;
}



/*
 鳞片
**/


#define S1_SWINDOWSIZE 5
#define S1_TABLE_SIZE (1<<(S1_SWINDOWSIZE-2))
#define S2_SWINDOWSIZE 7
#define S2_TABLE_SIZE (1<<(S2_SWINDOWSIZE-2))

static void
ge25519_double_scalarmult_vartime(ge25519 *r, const ge25519 *p1, const bignum256modm s1, const bignum256modm s2) {
	signed char slide1[256], slide2[256];
	ge25519_pniels ALIGN(16) pre1[S1_TABLE_SIZE];
	ge25519 ALIGN(16) d1;
	ge25519_p1p1 ALIGN(16) t;
	int32_t i;

	contract256_slidingwindow_modm(slide1, s1, S1_SWINDOWSIZE);
	contract256_slidingwindow_modm(slide2, s2, S2_SWINDOWSIZE);

	ge25519_double(&d1, p1);
	ge25519_full_to_pniels(pre1, p1);
	for (i = 0; i < S1_TABLE_SIZE - 1; i++)
		ge25519_pnielsadd(&pre1[i+1], &d1, &pre1[i]);

 /*设置中立*/
	memset(r, 0, sizeof(ge25519));
	r->y[0] = 1;
	r->z[0] = 1;

	i = 255;
	while ((i >= 0) && !(slide1[i] | slide2[i]))
		i--;

	for (; i >= 0; i--) {
		ge25519_double_p1p1(&t, r);

		if (slide1[i]) {
			ge25519_p1p1_to_full(r, &t);
			ge25519_pnielsadd_p1p1(&t, r, &pre1[abs(slide1[i]) / 2], (unsigned char)slide1[i] >> 7);
		}

		if (slide2[i]) {
			ge25519_p1p1_to_full(r, &t);
			ge25519_nielsadd2_p1p1(&t, r, &ge25519_niels_sliding_multiples[abs(slide2[i]) / 2], (unsigned char)slide2[i] >> 7);
		}

		ge25519_p1p1_to_partial(r, &t);
	}
}

#if !defined(HAVE_GE25519_SCALARMULT_BASE_CHOOSE_NIELS)

static uint32_t
ge25519_windowb_equal(uint32_t b, uint32_t c) {
	return ((b ^ c) - 1) >> 31;
}

static void
ge25519_scalarmult_base_choose_niels(ge25519_niels *t, const uint8_t table[256][96], uint32_t pos, signed char b) {
	bignum25519 ALIGN(16) neg;
	uint32_t sign = (uint32_t)((unsigned char)b >> 7);
	uint32_t mask = ~(sign - 1);
	uint32_t u = (b + mask) ^ mask;
	uint32_t i;

 /*包装形式的ysubx、xaddy、t2d。初始化为ysubx=1，xaddy=1，t2d=0*/
	uint8_t ALIGN(16) packed[96] = {0};
	packed[0] = 1;
	packed[32] = 1;

	for (i = 0; i < 8; i++)
		curve25519_move_conditional_bytes(packed, table[(pos * 8) + i], ge25519_windowb_equal(u, i + 1));

 /*扩展到T*/
	curve25519_expand(t->ysubx, packed +  0);
	curve25519_expand(t->xaddy, packed + 32);
	curve25519_expand(t->t2d  , packed + 64);

 /*调整标志*/
	curve25519_swap_conditional(t->ysubx, t->xaddy, sign);
	curve25519_neg(neg, t->t2d);
	curve25519_swap_conditional(t->t2d, neg, sign);
}

/*DIF/*让25519_scalarmult_base_choose_niels*/

静态空隙
GE25519诳scalarmult_base诳（GE25519*R，const uint8_t table[256][96]，const bignum256modm s）
 有符号字符B[64]；
 UIT32 32Ⅰ；
 GE25519对齐（16）T；

 合同256_window4_modm（b，s）；

 GE25519_scalarmult_base_choose_niels（&t，table，0，b[1]）；
 曲线25519_sub_reduce（r->x，t.xaddy，t.ysubx）；
 曲线25519_加_减（r->y，t.xaddy，t.ysubx）；
 memset（r->z，0，sizeof（bignum25519））；
 R＞Z〔0〕＝2；
 曲线25519_副本（R->T，T.t2d）；
 对于（i=3；i<64；i+=2）
  GE25519_scalarmult_base_choose_niels（&t，table，i/2，b[i]）；
  GE25519《尼尔沙dd2》（R&T）；
 }
 GE25519_双_部分（R，R）；
 GE25519_双_部分（R，R）；
 GE25519_双_部分（R，R）；
 GE25519_双精度（R，R）；
 GE25519_scalarmult_base_choose_niels（&t，table，0，b[0]）；
 曲线25519_-mul（t.t2d，t.t2d，ge25519_-ecd）；
 GE25519《尼尔沙dd2》（R&T）；
 对于（i=2；i<64；i+=2）
  GE25519_scalarmult_base_choose_niels（&t，table，i/2，b[i]）；
  GE25519《尼尔沙dd2》（R&T）；
 }
}
