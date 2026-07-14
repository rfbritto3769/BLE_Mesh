/** Input slots for operations
 *
 * @file
 */
/*
 * Copyright (c) 2018-2020 Silex Insight sa
 * Copyright (c) 2018-2020 Beerten Engineering
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INPUTSLOTS_HEADER_FILE
#define INPUTSLOTS_HEADER_FILE


/** Input slots for ::SX_PK_CMD_ODD_MOD_INV & ::SX_PK_CMD_ODD_MOD_REDUCE
 * & ::SX_PK_CMD_MOD_SQRT & ::SX_PK_CMD_EVEN_MOD_INV &
 * ::SX_PK_CMD_ODD_MOD_REDUCE
 */
struct sx_pk_inops_mod_single_op_cmd {
   struct sx_pk_slot n; /**< Modulus **/
   struct sx_pk_slot b; /**< Value **/
};

/** Input slots for ::SX_PK_CMD_MOD_ADD & ::SX_PK_CMD_MOD_SUB
 * & ::SX_PK_CMD_ODD_MOD_MULT & ::SX_PK_CMD_ODD_MOD_DIV
 */
struct sx_pk_inops_mod_cmd {
   struct sx_pk_slot n; /**< Modulus **/
   struct sx_pk_slot a; /**< Operand A **/
   struct sx_pk_slot b; /**< Operand B **/
};

/** Input slots for ::SX_PK_CMD_MULT */
struct sx_pk_inops_mult {
   struct sx_pk_slot a; /**< First scalar value **/
   struct sx_pk_slot b; /**< Second scalar value **/
};


/** Input slots for ::SX_PK_CMD_MOD_EXP */
struct sx_pk_inops_mod_exp {
   struct sx_pk_slot m; /**< Modulus **/
   struct sx_pk_slot input; /**< Base **/
   struct sx_pk_slot exp; /**< Exponent **/
};


/** Input slots for ::SX_PK_CMD_RSA_MOD_EXP_CM */
struct sx_pk_inops_rsa_mod_exp_cm {
   struct sx_pk_slot m; /**< Modulus **/
   struct sx_pk_slot lambda_n; /**< Lambda_n **/
   struct sx_pk_slot input; /**< Base **/
   struct sx_pk_slot exp; /**< Exponent **/
   struct sx_pk_slot blind; /**< Blinding factor **/
};


/** Input slots for ::SX_PK_CMD_MOD_EXP_CRT */
struct sx_pk_inops_crt_mod_exp {
   struct sx_pk_slot p; /**< Prime number p **/
   struct sx_pk_slot q; /**< Prime number q **/
   struct sx_pk_slot in; /**< Input **/
   struct sx_pk_slot dp; /**< d mod (p-1), with d the private key **/
   struct sx_pk_slot dq; /**< d mod (q-1), with d the private key **/
   struct sx_pk_slot qinv; /**< q^(-1) mod p **/
};

/** Input slots for ::SX_PK_CMD_RSA_KEYGEN */
struct sx_pk_inops_rsa_keygen {
   struct sx_pk_slot p; /**< Prime number p **/
   struct sx_pk_slot q; /**< Prime number q **/
   struct sx_pk_slot e; /**< Public exponent **/
};

/** Input slots for ::SX_PK_CMD_RSA_CRT_KEYPARAMS */
struct sx_pk_inops_rsa_crt_keyparams {
   struct sx_pk_slot p; /**< Prime number p **/
   struct sx_pk_slot q; /**< Prime number q **/
   struct sx_pk_slot privkey; /**< Private key **/
};

/** Input slots for ::SX_PK_CMD_SRP_USER_KEY_GEN */
struct sx_pk_inops_srp_user_keyparams {
   struct sx_pk_slot n; /**< Safe prime number **/
   struct sx_pk_slot g; /**< Generator of the multiplicative group **/
   struct sx_pk_slot a; /**< Random value **/
   struct sx_pk_slot b; /**< k * g^x + g^t with t random salt, k value derived by both sides (for example k = H(n, g)) **/
   struct sx_pk_slot x; /**< Hash of (s, p) with s a random salt and p the user password **/
   struct sx_pk_slot k; /**< Hash of (n, g) **/
   struct sx_pk_slot u; /**< Hash of (g^a, b) **/
};

/** Input slots for ::SX_PK_CMD_ECKCDSA_SIGN */
struct sx_pk_inops_eckcdsa_sign {
   struct sx_pk_slot d; /**< Private key **/
   struct sx_pk_slot k; /**< Random value **/
   struct sx_pk_slot r; /**< First part of signature **/
   struct sx_pk_slot h; /**< Hash digest **/
};


/** Input slots for ::SX_PK_CMD_MONTGOMERY_PTMUL */
struct sx_pk_inops_montgomery_mult {
   struct sx_pk_slot p; /**< Point P **/
   struct sx_pk_slot k; /**< Scalar **/
};


/** Input slots for ::SX_PK_CMD_ECC_PT_ADD */
struct sx_pk_inops_ecp_add {
   struct sx_pk_slot p1x; /**< x-coordinate of point P1 **/
   struct sx_pk_slot p1y; /**< y-coordinate of point P1 **/
   struct sx_pk_slot p2x; /**< x-coordinate of point P2 **/
   struct sx_pk_slot p2y; /**< y-coordinate of point P2 **/
};


/** Input slots for ::SX_PK_CMD_ECC_PTMUL */
struct sx_pk_inops_ecp_mult {
   struct sx_pk_slot k; /**< Scalar **/
   struct sx_pk_slot px; /**< x-coordinate of point P **/
   struct sx_pk_slot py; /**< y-coordinate of point P **/
};

/** Input slots for ::SX_PK_CMD_ECC_PT_DOUBLE */
struct sx_pk_inops_ecp_double {
   struct sx_pk_slot px; /**< x-coordinate of point P **/
   struct sx_pk_slot py; /**< y-coordinate of point P **/
};


/** Input slots for ::SX_PK_CMD_ECC_PTONCURVE */
struct sx_pk_inops_ec_ptoncurve {
   struct sx_pk_slot px; /**< x-coordinate of point P **/
   struct sx_pk_slot py; /**< y-coordinate of point P **/
};

/** Input slots for ::SX_PK_CMD_ECC_PT_DECOMP */
struct sx_pk_inops_ec_pt_decompression {
   struct sx_pk_slot x; /**< x-coordinate of compressed point **/
};


/** Input slots for ::SX_PK_CMD_ECDSA_VER &
 * ::SX_PK_CMD_ECKCDSA_VER & ::SX_PK_CMD_SM2_VER
 */
struct sx_pk_inops_ecdsa_verify {
   struct sx_pk_slot qx; /**< x-coordinate of public key **/
   struct sx_pk_slot qy; /**< y-coordinate of public key **/
   struct sx_pk_slot r; /**< First part of signature **/
   struct sx_pk_slot s; /**< Second part of signature **/
   struct sx_pk_slot h; /**< Hash digest **/
};


/** Input slots for ::SX_PK_CMD_ECDSA_GEN & ::SX_PK_CMD_SM2_GEN */
struct sx_pk_inops_ecdsa_generate {
   struct sx_pk_slot d; /**< Private key **/
   struct sx_pk_slot k; /**< Random value **/
   struct sx_pk_slot h; /**< Hash digest **/
};

/** Input slots for ::SX_PK_CMD_SM2_EXCH */
struct sx_pk_inops_sm2_exchange {
   struct sx_pk_slot d; /**< Private key **/
   struct sx_pk_slot k; /**< Random value **/
   struct sx_pk_slot qx; /**< x-coordinate of public key **/
   struct sx_pk_slot qy; /**< y-coordinate of public key **/
   struct sx_pk_slot rbx; /**< x-coordinate of random value from B **/
   struct sx_pk_slot rby; /**< y-coordinate of random value from B **/
   struct sx_pk_slot cof; /**< Cofactor **/
   struct sx_pk_slot rax; /**< x-coordinate of random value from A **/
   struct sx_pk_slot w; /**< (log2(n)/2)-1, with n the curve order **/
};

/** Input slots for ::SX_PK_CMD_ECKCDSA_PUBKEY_GEN */
struct sx_pk_inops_eckcdsa_generate {
   struct sx_pk_slot d; /**< Private key **/
};


/** Input slots for ::SX_PK_CMD_EDDSA_PTMUL */
struct sx_pk_inops_eddsa_ptmult {
   struct sx_pk_dblslot r; /**< Scalar **/
};


/** Input slots for ::SX_PK_CMD_EDDSA_SIGN */
struct sx_pk_inops_eddsa_sign {
   struct sx_pk_dblslot k; /**< Scalar with a size double of other operands **/
   struct sx_pk_dblslot r; /**< Signature part 1 **/
   struct sx_pk_slot s; /**< Signature part 2 **/
};



/** Input slots for ::SX_PK_CMD_EDDSA_VER */
struct sx_pk_inops_eddsa_ver {
   struct sx_pk_dblslot k; /**< Scalar with a size double of other operands **/
   struct sx_pk_slot ay; /**< Encoded public key  **/
   struct sx_pk_slot sig_s; /**< Signature part 2 **/
   struct sx_pk_slot ry; /**< y-coordinate of r **/
};


/** Input slots for ::SX_PK_CMD_MILLER_RABIN */
struct sx_pk_inops_miller_rabin {
   struct sx_pk_slot n; /**< Candidate prime value **/
   struct sx_pk_slot a; /**< Random value **/
};

/** Input slots for ::SX_PK_CMD_DSA_SIGN */
struct sx_pk_inops_dsa_sign {
   struct sx_pk_slot p; /**< Prime modulus **/
   struct sx_pk_slot q; /**< Prime divisor of p-1 **/
   struct sx_pk_slot g; /**< Generator **/
   struct sx_pk_slot k; /**< Random value **/
   struct sx_pk_slot privkey; /**< Private key **/
   struct sx_pk_slot h; /**< Hash digest **/
};

/** Input slots for ::SX_PK_CMD_DSA_VER */
struct sx_pk_inops_dsa_ver {
   struct sx_pk_slot p; /**< Prime modulus **/
   struct sx_pk_slot q; /**< Prime divisor of p-1 **/
   struct sx_pk_slot g; /**< Generator **/
   struct sx_pk_slot pubkey; /**< Public key **/
   struct sx_pk_slot r; /**< First part of signature **/
   struct sx_pk_slot s; /**< Second part of signature **/
   struct sx_pk_slot h; /**< Hash digest **/
};

/** Input slots for ::SX_PK_CMD_SM9_EXP */
struct sx_pk_inops_sm9_exp {
   struct sx_pk_slot h; /**< exponent */
   struct sx_pk_slot t; /**< polynomial base */
   struct sx_pk_slot g[12]; /**< g in GT */
};

/** Input slots for ::SX_PK_CMD_SM9_PMULG1 */
struct sx_pk_inops_sm9_pmulg1 {
   struct sx_pk_slot p1x0; /**< x-coordinate */
   struct sx_pk_slot p1y0; /**< y-coordinate */
   struct sx_pk_slot ke; /**< scalar */
   struct sx_pk_slot t; /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_PMULG2 */
struct sx_pk_inops_sm9_pmulg2 {
   struct sx_pk_slot p2x0; /**< x-coordinate 0 */
   struct sx_pk_slot p2x1; /**< x-coordinate 1 */
   struct sx_pk_slot p2y0; /**< y-coordinate 0 */
   struct sx_pk_slot p2y1; /**< y-coordinate 1 */
   struct sx_pk_slot ke; /**< scalar */
   struct sx_pk_slot t; /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_PAIR */
struct sx_pk_inops_sm9_pair {
   struct sx_pk_slot qx0; /**< Q x-coordinate 0 */
   struct sx_pk_slot qx1; /**< Q x-coordinate 1 */
   struct sx_pk_slot qy0; /**< Q y-coordinate 0 */
   struct sx_pk_slot qy1; /**< Q y-coordinate 1 */
   struct sx_pk_slot px0; /**< P x-coordinate */
   struct sx_pk_slot py0; /**< P y-coordinate */
   struct sx_pk_slot f; /**< frobenius constant */
   struct sx_pk_slot t; /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_PRIVSIGKEYGEN */
struct sx_pk_inops_sm9_sigpkgen {
   struct sx_pk_slot p1x0; /**< x-coordinate */
   struct sx_pk_slot p1y0; /**< y-coordinate */
   struct sx_pk_slot h; /**< scalar */
   struct sx_pk_slot ks; /**< scalar */
   struct sx_pk_slot t; /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_SIGNATUREGEN */
struct sx_pk_inops_sm9_signaturegen {
   struct sx_pk_slot dsx0; /**< x-coordinate */
   struct sx_pk_slot dsy0; /**< y-coordinate */
   struct sx_pk_slot h; /**< scalar */
   struct sx_pk_slot r; /**< scalar */
   struct sx_pk_slot t; /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_SIGNATUREVERIFY */
struct sx_pk_inops_sm9_signatureverify {
   struct sx_pk_slot h1; /**< scalar */
   struct sx_pk_slot p2x0; /**< x-coordinate */
   struct sx_pk_slot p2x1; /**< x-coordinate */
   struct sx_pk_slot p2y0; /**< y-coordinate */
   struct sx_pk_slot p2y1; /**< y-coordinate */
   struct sx_pk_slot ppubsx0; /**< x-coordinate 0 */
   struct sx_pk_slot ppubsx1; /**< x-coordinate 1 */
   struct sx_pk_slot ppubsy0; /**< y-coordinate 0 */
   struct sx_pk_slot ppubsy1; /**< y-coordinate 1 */
   struct sx_pk_slot sx0; /**< x-coordinate */
   struct sx_pk_slot sy0; /**< y-coordinate */
   struct sx_pk_slot h; /**< scalar */
   struct sx_pk_slot f; /**< frobenius constant */
   struct sx_pk_slot t; /**< polynomial base */
   struct sx_pk_slot g[12]; /**< input */
};

/** Input slots for ::SX_PK_CMD_SM9_PRIVENCRKEYGEN */
struct sx_pk_inops_sm9_privencrkeygen {
   struct sx_pk_slot p2x0; /**< x-coordinate 0 */
   struct sx_pk_slot p2x1; /**< x-coordinate 1 */
   struct sx_pk_slot p2y0; /**< y-coordinate 0 */
   struct sx_pk_slot p2y1; /**< y-coordinate 1 */
   struct sx_pk_slot h; /**< scalar */
   struct sx_pk_slot ks; /**< scalar */
   struct sx_pk_slot t; /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_SENDKEY */
struct sx_pk_inops_sm9_sendkey {
   struct sx_pk_slot p1x0; /**< x-coordinate */
   struct sx_pk_slot p1y0; /**< y-coordinate */
   struct sx_pk_slot ppubex0; /**< x-coordinate */
   struct sx_pk_slot ppubey0; /**< y-coordinate */
   struct sx_pk_slot h; /**< scalar */
   struct sx_pk_slot r; /**< scalar */
   struct sx_pk_slot t; /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_SM9_REDUCEH */
struct sx_pk_inops_sm9_reduceh {
   struct sx_pk_slot h; /**< scalar */
   struct sx_pk_slot t; /**< polynomial base */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_GENERATE_ZKP */
struct sx_pk_inops_ecjpake_generate_zkp {
   struct sx_pk_slot v; /**< Random input */
   struct sx_pk_slot x; /**< Exponent */
   struct sx_pk_slot h; /**< Hash digest */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_VERIFY_ZKP */
struct sx_pk_inops_ecjpake_verify_zkp {
   struct sx_pk_slot xv; /**< Point V on the curve, x-coordinate */
   struct sx_pk_slot yv; /**< Point V on the curve, y-coordinate */
   struct sx_pk_slot xx; /**< Point X on the curve, x-coordinate */
   struct sx_pk_slot yx; /**< Point X on the curve, y-coordinate */
   struct sx_pk_slot r; /**< Proof */
   struct sx_pk_slot h; /**< Hash digest */
   struct sx_pk_slot xg2; /**< Point G on the curve, x-coordinate */
   struct sx_pk_slot yg2; /**< Point G on the curve, y-coordinate */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_3PT_ADD */
struct sx_pk_inops_ecjpake_3pt_add {
   struct sx_pk_slot x2_1; /**< Point X2 on the curve, x-coordinate */
   struct sx_pk_slot x2_2; /**< Point X2 on the curve, y-coordinate */
   struct sx_pk_slot x3_1; /**< Point X3 on the curve, x-coordinate */
   struct sx_pk_slot x3_2; /**< Point X3 on the curve, y-coordinate */
   struct sx_pk_slot x1_1; /**< Point X1 on the curve, x-coordinate */
   struct sx_pk_slot x1_2; /**< Point X1 on the curve, y-coordinate */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_GEN_STEP_2 */
struct sx_pk_inops_ecjpake_gen_step_2 {
   struct sx_pk_slot x4_1; /**< Point X4 on the curve, x-coordinate */
   struct sx_pk_slot x4_2; /**< Point X4 on the curve, y-coordinate */
   struct sx_pk_slot x3_1; /**< Point X3 on the curve, x-coordinate */
   struct sx_pk_slot x3_2; /**< Point X3 on the curve, y-coordinate */
   struct sx_pk_slot x2s;  /**< Random value * Password */
   struct sx_pk_slot x1_1; /**< Point X1 on the curve, x-coordinate */
   struct sx_pk_slot x1_2; /**< Point X1 on the curve, y-coordinate */
   struct sx_pk_slot s;    /**< Password */
};

/** Input slots for ::SX_PK_CMD_ECJPAKE_GEN_SESS_KEY */
struct sx_pk_inops_ecjpake_gen_sess_key {
   struct sx_pk_slot x4_1; /**< Point X4 on the curve, x-coordinate */
   struct sx_pk_slot x4_2; /**< Point X4 on the curve, y-coordinate */
   struct sx_pk_slot b_1; /**< Point B on the curve, x-coordinate */
   struct sx_pk_slot b_2; /**< Point B on the curve, y-coordinate */
   struct sx_pk_slot x2; /**< Random value */
   struct sx_pk_slot x2s; /**< Random value * Password */
};

/** Inputs slots for ::SX_PK_CMD_SRP_SERVER_PUBLIC_KEY_GEN */
struct sx_pk_inops_srp_server_public_key_gen {
   struct sx_pk_slot n; /**< Safe prime */
   struct sx_pk_slot g; /**< Generator */
   struct sx_pk_slot k; /**< Hash digest */
   struct sx_pk_slot v; /**< Exponentiated hash digest */
   struct sx_pk_slot b; /**< Random */
};

/** Inputs slots for ::SX_PK_CMD_SRP_SERVER_SESSION_KEY_GEN */
struct sx_pk_inops_srp_server_session_key_gen {
   struct sx_pk_slot n; /**< Safe prime */
   struct sx_pk_slot a; /**< Random */
   struct sx_pk_slot u; /**< Hash digest */
   struct sx_pk_slot v; /**< Exponentiated hash digest */
   struct sx_pk_slot b; /**< Random */
};

/** Inputs slots for ::SX_PK_CMD_CHECK_PARAM_AB */
struct sx_pk_inops_check_param_ab {
   struct sx_pk_slot p; /**< p parameter of curve */
   struct sx_pk_slot a; /**< a parameter of curve */
   struct sx_pk_slot b; /**< b parameter of curve */
};

/** Inputs slots for ::SX_PK_CMD_CHECK_PARAM_N */
struct sx_pk_inops_check_param_n {
   struct sx_pk_slot p; /**< p parameter of curve */
   struct sx_pk_slot n; /**< n parameter of curve */
};

/** Inputs slots for ::SX_PK_CMD_CHECK_XY */
struct sx_pk_inops_check_xy {
   struct sx_pk_slot p; /**< p parameter of curve */
   struct sx_pk_slot x; /**< x-coordinate */
   struct sx_pk_slot y; /**< y-coordinate */
};

#endif
