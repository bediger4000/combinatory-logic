/*
	Copyright (C) 2007-2011, Bruce Ediger

    This file is part of cl.

    cl is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    cl is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cl; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



#include <stdio.h>  /* NULL */
#include <string.h> /* strcmp() */

#include <buffer.h>
#include <node.h>
#include <graph.h>
#include <bracket_abstraction.h>

bracket_abstraction_function default_bracket_abstraction = curry_bracket_abstraction;
int var_appears_in_graph(struct node *var, struct node *tree);
int free_var_appears_in_graph(struct node *tree);

/* returns 1 if a given variable (argument "var") appears in an
   expression, returns 0 if the given variable does not appear.
 */
int
var_appears_in_graph(struct node *var, struct node *tree)
{
	int r = 0;
	switch (tree->typ)
	{
	case APPLICATION:
		r = var_appears_in_graph(var, tree->left)
			|| var_appears_in_graph(var, tree->right);
		break;
	case ATOM:
		if (var->cn == tree->cn)
		{
			if (var->name == tree->name)
				r = 1;
		}
		break;
	}
	return r;
}

/* returns 1 if any free variable appears in an expression,
   returns 0 if no free variable appears in an expression.
 */
int
free_var_appears_in_graph(struct node *tree)
{
	int r = 0;

	switch (tree->typ)
	{
	case APPLICATION:
		r = free_var_appears_in_graph(tree->left);
		if (!r)
			r = free_var_appears_in_graph(tree->right);
		break;
	case ATOM:
		if (COMB_NONE == tree->cn)
			r = 1;
		break;
	}
	return r;
}

/* Curry-Feys bracket abstraction
	[x] x   -> I
	[x] N   -> K N,   x not appearing in N
	[x] M N -> S ([x]M) ([x]N)
 */
struct node *
curry_bracket_abstraction(struct node *var, struct node *tree)
{
	struct node *r = NULL;
	switch (tree->typ)
	{
	case APPLICATION:
		if (!var_appears_in_graph(var, tree))
			/* [x] A -> K A, x not appearing in A */
			r = new_application(new_combinator(COMB_K), arena_copy_graph(tree));
		else {
			r = new_application(
				new_application(
					new_combinator(COMB_S),
					curry_bracket_abstraction(var, tree->left)
				),
				curry_bracket_abstraction(var, tree->right)
			);
		}
		break;
	case ATOM:
		if (var->cn == tree->cn && var->name == tree->name)
			/* [x] x -> I */
			r = new_combinator(COMB_I);
		else
			/* [x] N -> K N */
			r = new_application(
				new_combinator(COMB_K),
				COMB_NONE == tree->cn? new_term(tree->name): new_combinator(tree->cn)
			);
		break;
	}
	return r;
}

/* Curry-Feys bracket abstraction, version 2
	[x] x   -> I
	[x] N x -> N      x not appearing in N <-- New rule, vs version above.
	[x] N   -> K N,   x not appearing in N
	[x] M N -> S ([x]M) ([x]N)
 */
struct node *
curry2_bracket_abstraction(struct node *var, struct node *tree)
{
	struct node *r = NULL;

	switch (tree->typ)
	{
	case APPLICATION:
		if (!var_appears_in_graph(var, tree))
			/* [x] A -> K A, x not appearing in A */
			r = new_application(new_combinator(COMB_K), arena_copy_graph(tree));
		else {
			if ((tree->right->typ == ATOM && tree->right->name == var->name) && !var_appears_in_graph(var, tree->left))
			{
				/* [x] N x -> N      x not appearing in N */
				r = arena_copy_graph(tree->left);
			} else {
				/* M N -> S ([x]M) ([x]N) */
				r = new_application(
					new_application(
						new_combinator(COMB_S),
						curry2_bracket_abstraction(var, tree->left)
					),
					curry2_bracket_abstraction(var, tree->right)
				);
			}
		}
		break;
	case ATOM:
		if (var->cn == tree->cn && var->name == tree->name)
			/* [x] x -> I */
			r = new_combinator(COMB_I);
		else
			/* [x] N -> K N */
			r = new_application(
				new_combinator(COMB_K),
				COMB_NONE == tree->cn? new_term(tree->name): new_combinator(tree->cn)
			);
		break;
	}

	return r;
}

/* Simple Turner bracket abstraction
	[x] x   -> I
	[x] N x -> N                x not appearing in N
	[x] N   -> K N              x not appearing in N
	[x] M N -> C ([x]M) N       x appears only in M, not in N
	[x] M N -> B M ([x] N)      x appears only in N, not in M
	[x] M N -> S ([x]M) ([x]N)  x appears in both M and N
 */
struct node *
turner_bracket_abstraction(struct node *var, struct node *tree)
{
	struct node *r = NULL;
	switch (tree->typ)
	{
	case APPLICATION:
		if (!var_appears_in_graph(var, tree))
			/* [x] N   -> K N   x not appearing in N */
			r = new_application(new_combinator(COMB_K), arena_copy_graph(tree));
		else {
			/* variable getting abstracted out appears somewhere */
			if (var_appears_in_graph(var, tree->left))
			{
				if (var_appears_in_graph(var, tree->right))
				{
					/* [x] M N -> S ([x]M) ([x]N)  x appears in both M and N */
					r = new_application(
						new_application(
							new_combinator(COMB_S),
							turner_bracket_abstraction(var, tree->left)
						),
						turner_bracket_abstraction(var, tree->right)
					);
				} else {
					/* [x] M N -> C ([x]M) N       x appears only in M, not in N */
					r = new_application(
						new_application(
							new_combinator(COMB_C),
							turner_bracket_abstraction(var, tree->left)
						),
						arena_copy_graph(tree->right)
					);
				}
			} else if (var_appears_in_graph(var, tree->right)) {
				if (ATOM == tree->right->typ && var->name == tree->right->name)
				{
					/* [x] N x -> N                x not appearing in N */
					r = arena_copy_graph(tree->left);
				} else {
					/* [x] M N -> B M ([x] N)      x appears only in N, not in M */
					r = new_application(
						new_application(
							new_combinator(COMB_B),
							arena_copy_graph(tree->left)
						),
						turner_bracket_abstraction(var, tree->right)
					);
				}
			}
		}
		break;
	case ATOM:
		if (var->cn == tree->cn && var->name == tree->name)
			/* [x] x -> I */
			r = new_combinator(COMB_I);
		else
			/* [x] N -> K N */
			r = new_application(
				new_combinator(COMB_K),
				COMB_NONE == tree->cn? new_term(tree->name): new_combinator(tree->cn)
			);
		break;
	}
	return r;
}

/* "A 'new' abstraction algorithm", M.A. Price, H.Simmons
   This actually implements "The cooked G-Algorithm".
   "Grzegorgczyk" algorithm.
	[x] x   -> I
	[x] Z   -> K Z                   x not appearing in Z
	[x] Q x -> Q                     x not appearing in Q
	[x] Q P -> B Q ([x] P)           x appears only in P, not in Q
	[x] Q P -> C ([x]Q) P            x appears only in Q, not in P
	[x] Q P -> W((B(C([x]Q)))([x]P)) x appears in both P and Q

   Note that the last transformation could just as well have a
   different form.
 */
struct node *
grzegorczyk_bracket_abstraction(struct node *var, struct node *tree)
{
	struct node *r = NULL;
	switch (tree->typ)
	{
	case APPLICATION:
		if (!var_appears_in_graph(var, tree))
			/* [x] Z   -> K Z    x not appearing in Z */
			r = new_application(new_combinator(COMB_K), arena_copy_graph(tree));
		else {
			/* variable getting abstracted out appears somewhere */
			if (var_appears_in_graph(var, tree->left))
			{
				if (var_appears_in_graph(var, tree->right))
				{
					/* [x] Q P -> W( (B(C([x]Q))) ([x]P)) x appears in both Q and P */
					r = new_application(
							new_combinator(COMB_W),
							new_application(
								new_application(
									new_combinator(COMB_B),
									new_application(
										new_combinator(COMB_C),
										grzegorczyk_bracket_abstraction(var, tree->left)
									)
								),
								grzegorczyk_bracket_abstraction(var, tree->right)
							)
						);
				} else {
					/* [x] M N -> C ([x]M) N       x appears only in M, not in N */
					r = new_application(
						new_application(
							new_combinator(COMB_C),
							grzegorczyk_bracket_abstraction(var, tree->left)
						),
						arena_copy_graph(tree->right)
					);
				}
			} else if (var_appears_in_graph(var, tree->right)) {
				if (ATOM == tree->right->typ && var->name == tree->right->name)
				{
					/* [x] N x -> N                x not appearing in N */
					r = arena_copy_graph(tree->left);
				} else {
					/* [x] M N -> B M ([x] N)      x appears only in N, not in M */
					r = new_application(
						new_application(
							new_combinator(COMB_B),
							arena_copy_graph(tree->left)
						),
						grzegorczyk_bracket_abstraction(var, tree->right)
					);
				}
			}
		}
		break;
	case ATOM:
		if (var->cn == tree->cn && var->name == tree->name)
			/* [x] x -> I */
			r = new_combinator(COMB_I);
		else
			/* [x] N -> K N */
			r = new_application(
				new_combinator(COMB_K),
				COMB_NONE == tree->cn? new_term(tree->name): new_combinator(tree->cn)
			);
		break;
	}
	return r;
}

/*
    [x] x   -> B M K
    [x] Z   -> K Z                   x not appearing in Z
    [x] Q x -> Q                     x not appearing in Q
    [x] P Q -> B P ([x] Q)           x appears only in Q, not in P
    [x] P Q -> B (T Q) ([x]P)        x appears only in P, not in Q
    [x] P Q -> B (T (B (T [x]Q) (B B [x]P))) (B M (B B T))   x appears in both P and Q
    alternatively,
	[x] Q P -> W(B (B (T ([x]P)) B) ([x]Q))    x appears in both Q and P
    but W = B(T(B M (B B T)))(B B T) or W = B(T(B(B M B)T))(B B T)
    Either gives a 16-combinator expression, more than the 13 in the one used.
 */

struct node *
btmk_bracket_abstraction(struct node *var, struct node *tree)
{
	struct node *r = NULL;
	switch (tree->typ)
	{
	case APPLICATION:
		if (!var_appears_in_graph(var, tree))
			/* [x] Z   -> K Z    x not appearing in Z */
			r = new_application(new_combinator(COMB_K), arena_copy_graph(tree));
		else {
			/* variable getting abstracted out appears somewhere */
			if (var_appears_in_graph(var, tree->left))
			{
				if (var_appears_in_graph(var, tree->right))
				{
    				/* [x] P Q -> B (T (B (T [x]Q) (B B [x]P))) (B M (B B T))   x appears in both P and Q */
					r = new_application(
						new_application(
							new_combinator(COMB_B),
							new_application(
								new_combinator(COMB_T),
								new_application(
									new_application(
										new_combinator(COMB_B),
										new_application(
											new_combinator(COMB_T),
											btmk_bracket_abstraction(var, tree->right) /* [x]Q */
										)
									),
									new_application(
										new_application(
											new_combinator(COMB_B),
											new_combinator(COMB_B)
										),
										btmk_bracket_abstraction(var, tree->left) /* [x]P */
									)
								)
							)
						),
						new_application(
							new_application(
								new_combinator(COMB_B),
								new_combinator(COMB_M)
							),
							new_application(
								new_application(
									new_combinator(COMB_B),
									new_combinator(COMB_B)
								),
								new_combinator(COMB_T)
							)
						)
					);
				} else {
					/* [x] Q P -> B (T P) ([x]Q)       x appears only in Q, not in P */
					r = new_application(
							new_application(
								new_combinator(COMB_B),
								new_application(
									new_combinator(COMB_T),
									arena_copy_graph(tree->right)
								)
							),
							btmk_bracket_abstraction(var, tree->left)
					);
				}
			} else if (var_appears_in_graph(var, tree->right)) {
				if (ATOM == tree->right->typ && var->name == tree->right->name)
				{
					/* [x] N x -> N                x not appearing in N */
					r = arena_copy_graph(tree->left);
				} else {
					/* [x] M N -> B M ([x] N)      x appears only in N, not in M */
					r = new_application(
						new_application(
							new_combinator(COMB_B),
							arena_copy_graph(tree->left)
						),
						btmk_bracket_abstraction(var, tree->right)
					);
				}
			}
		}
		break;
	case ATOM:
		if (var->cn == tree->cn && var->name == tree->name)
			/* [x] x -> B M K */
			r = new_application(
				new_application(
					new_combinator(COMB_B),
					new_combinator(COMB_M)
				),
				new_combinator(COMB_K)
			);
		else
			/* [x] N -> K N */
			r = new_application(
				new_combinator(COMB_K),
				COMB_NONE == tree->cn? new_term(tree->name): new_combinator(tree->cn)
			);
		break;
	}
	return r;
}

/* return 1 if two graphs "equate", and 0 if they don't.
 * "Equate" means same tree structure (application-type nodes
 * in the same places, leaf (combinator) nodes in the same places),
 * and that combinator-type nodes in the same places have the same name.
 * Used only to determine whether to apply this rule:
 * [x] ((M L) (N L)) -> [x](S M N L)         (M, N combinators)
 * in the Tromp Bracket Abstraction algorithm, below.
 */
int
equivalent_graphs(struct node *g1, struct node *g2)
{
	int r = 0;

	if (g1->typ == g2->typ)
	{
		switch (g1->typ)
		{
		case APPLICATION:
			r = equivalent_graphs(g1->left, g2->left)
				&& equivalent_graphs(g1->right, g2->right);
			break;
		case ATOM:
			if (g1->cn == g2->cn && g1->name == g2->name)
				r = 1;
			break;
		}

	}

	return r;
}


/* Apply the following set of 9 rules in decreasing order
 * of applicability:
 *
 * [x](S K M)        -> S K                  (For all M)
 * [x] M             -> K M                  (x not appearing in M)
 * [x] x             -> I
 * [x] M x           -> M                    (x not appearing in M)
 * [x] x M x         -> [x] (S S K x M)
 * [x] (M (N L))     -> [x] (S ([x] M) N L)  (M, N combinators)
 * [x] ((M N) L)     -> [x] (S M ([x] L) N)  (M, L combinators)
 * [x] ((M L) (N L)) -> [x](S M N L)         (M, N combinators)
 * [x] M N           -> S ([x] M) ([x] N)
 *
 * Phrases like "M, N combinators" mean that no variables, abstracted out
 * or not, appear in terms M and N.  This restriction helps in cases
 * like [x][y] M.  Abstraction y out of a term M that contains x will
 * cause the [x] to create a far larger final term.
 *
 * tromp_bracket_abstraction() examines the parse tree argument,
 * and returns a parse tree with the combinator named by var argument
 * abstracted out of the original parse tree.  The returned parse tree
 * has no references to the original, and the original parse tree does
 * not get modified in the process.
 * Hey, only S, K and I end up in the final abstracted expression.
 * How could you incorporate B and C in it? If they helped Turner, maybe
 * they would help here.
 */
struct node *
tromp_bracket_abstraction(struct node *var, struct node *tree)
{
	if (APPLICATION == tree->typ
		&& APPLICATION == tree->left->typ
		&& COMB_S == tree->left->left->cn
		&& COMB_K == tree->left->right->cn
	)
	{
		/* [x] (S K M) -> S K */
		return new_application(new_combinator(COMB_S), new_combinator(COMB_K));
	}

	

	if (!var_appears_in_graph(var, tree))
	{
		/* [x] M -> K M    when x doesn't appear in M */
		return new_application(new_combinator(COMB_K),
			arena_copy_graph(tree));
	}

	if (ATOM == tree->typ && var->name == tree->name)
	{
		/* [x] x -> I */
		return new_combinator(COMB_I);
	}

	if (APPLICATION == tree->typ
		&& ATOM == tree->right->typ
		&& var->name == tree->right->name
		&& !var_appears_in_graph(var, tree->left)
	)
	{
		/* [x] M x -> M   x not in M */
		return arena_copy_graph(tree->left);
	}

	if (APPLICATION == tree->typ
		&& APPLICATION == tree->left->typ
		&& ATOM == tree->right->typ
		&& var->name == tree->right->name
		&& ATOM == tree->left->left->typ
		&& var->name == tree->left->left->name
	)
	{
		/* [x] (x M x) -> [x] (S S K x M) */
		struct node *r;
		struct node *disposable = new_application(
				new_application(
					new_application(
						new_application(
							new_combinator(COMB_S),
							new_combinator(COMB_S)
						),
						new_combinator(COMB_K)
					),
					new_term(var->name)
				),
				arena_copy_graph(tree->left->right)
		);
		++disposable->refcnt;
		r = tromp_bracket_abstraction(var, disposable);
		free_node(disposable);
		return r;
	}

	if (APPLICATION == tree->typ
		&& !free_var_appears_in_graph(tree->left)
		&& APPLICATION == tree->right->typ
		&& !free_var_appears_in_graph(tree->right->left)
	)
	{
 		/* [x] (M (N L))     -> [x] (S ([x] M) N L)  (M, N combinators) */
		struct node *r;
		struct node *disposable = new_application(
			new_application(
				new_application(
					new_combinator(COMB_S),
					tromp_bracket_abstraction(var, tree->left)
				),
				arena_copy_graph(tree->right->left)
			),
			arena_copy_graph(tree->right->right)
		);
		r = tromp_bracket_abstraction(var, disposable);
		++disposable->refcnt;
		free_node(disposable);
		return r;
	}

	if (APPLICATION == tree->typ
		&& APPLICATION == tree->left->typ
		&& !free_var_appears_in_graph(tree->left->left)
		&& !free_var_appears_in_graph(tree->right)
	)
	{
		/* [x] ((M N) L)     -> [x] (S M ([x] L) N)  (M, L combinators) */
		struct node *r;
		struct node *disposable = new_application(
			new_application(
				new_application(
					new_combinator(COMB_S),
					arena_copy_graph(tree->left->left)
				),
				tromp_bracket_abstraction(var, tree->right)
			),
			arena_copy_graph(tree->left->right)
		);
		r = tromp_bracket_abstraction(var, disposable);
		++disposable->refcnt;
		free_node(disposable);
		return r;
	}

	if (APPLICATION == tree->typ
		&& APPLICATION == tree->left->typ
		&& APPLICATION == tree->right->typ
		&& !free_var_appears_in_graph(tree->left->left)
		&& !free_var_appears_in_graph(tree->right->left)
		&& equivalent_graphs(tree->left->right, tree->right->right)

	)
	{
 		/* [x] ((M L) (N L)) -> [x](S M N L)         (M, N combinators) */
		struct node *r;
		struct node *disposable = new_application(
			new_application(
				new_application(
					new_combinator(COMB_S),
					arena_copy_graph(tree->left->left)
				),
				arena_copy_graph(tree->right->left)
			),
			arena_copy_graph(tree->right->right)
		);
		r = tromp_bracket_abstraction(var, disposable);
		++disposable->refcnt;
		free_node(disposable);
		return r;
	}

	/* XXX - does it really make sense for the final "rule" to not
	 * have an if() block around it? */

	/* [x] M N           -> S ([x] M) ([x] N) */
	return new_application(
		new_application(
			new_combinator(COMB_S),
			tromp_bracket_abstraction(var, tree->left)
		),
		tromp_bracket_abstraction(var, tree->right)
	);
}

/*
 * Abstraction for I,J basis from A. Church, "A Proof of Freedom From Contraction",
 * Proceedings of National Academy of Sciences, Vol 21, 1935, pg 275
 * [x] x  -> I
 * [x] U V  -> J (J I I) ([x]V) (U I L)  where x appears only in V
 * [x] U V  -> J (J I I) V [x]U  where x appears only in U
 * [x] R L  -> J (J I I) (J I I) (J I (J (J I I) (J I I) (J (J I I) [x]R (J (J I I) [x]L J))))
 *  where x appears in R and L.
 */
struct node *
ij_bracket_abstraction(struct node *var, struct node *tree)
{
	struct node *r = NULL;
	switch (tree->typ)
	{
	case APPLICATION:
		if (!var_appears_in_graph(var, tree))
		{
			/* Lambda-I basis: not valid expression gets here */
		} else {
			/* variable getting abstracted out appears somewhere */
			int appears_right = 0;
			int appears_left = var_appears_in_graph(var, tree->left);
			if (!appears_left)
				appears_right = 1;
			else
				appears_right = var_appears_in_graph(var, tree->right);

			if (appears_left)
			{
				if (appears_right)
				{
					/* abstraction var appears in both function and arg
					 * parts of an application
	`				 * [x] R L  -> J (J I I) (J I I) (J I (J (J I I) (J I I) (J (J I I) [x]R (J (J I I) [x]L J)))) */
					struct node *tmp, *T = new_application(
						new_application(
							new_combinator(COMB_J),
							new_combinator(COMB_I)
						),
						new_combinator(COMB_I)
					);
					r = new_application(new_combinator(COMB_J), T);
					r = new_application(r, ij_bracket_abstraction(var, tree->left));
					r = new_application(r, new_combinator(COMB_J));
					tmp = new_application(new_combinator(COMB_J), T);
					tmp = new_application(tmp, ij_bracket_abstraction(var, tree->right));
					r = new_application(tmp, r);
					tmp = new_application(new_combinator(COMB_J), T);
					tmp = new_application(tmp, T);
					r = new_application(tmp, r);
					tmp = new_application(new_combinator(COMB_J), new_combinator(COMB_I));
					r = new_application(tmp, r);
					tmp = new_application(new_combinator(COMB_J), T);
					tmp = new_application(tmp, T);
					r = new_application(tmp, r);
				} else {
					/* abstraction var appears only in function
					 * part of an application 
				 	 *  [x] U V  -> J (J I I) V [x]U  where x appears only in U */
					r = new_application(
						new_application(
							new_application(
								new_combinator(COMB_J),
								new_application(
									new_application(
										new_combinator(COMB_J),
										new_combinator(COMB_I)
									),
									new_combinator(COMB_I)
								)
							),
							arena_copy_graph(tree->right)
						),
						ij_bracket_abstraction(var, tree->left)
					);
				}
			} else if (appears_right) {
				struct node *t;
				/* abstraction var appears only in arg
				 * part of an application 
 				 * [x] U V  -> J (J I I) ([x]V) (J I U)  where x appears only in V */
				t = new_application(new_combinator(COMB_J), new_combinator(COMB_I));
				t = new_application(t, arena_copy_graph(tree->left));
				r = new_application(new_combinator(COMB_J), new_combinator(COMB_I));
				r = new_application(r, new_combinator(COMB_I));
				r = new_application(new_combinator(COMB_J), r);
				r = new_application(r, ij_bracket_abstraction(var, tree->right));
				r = new_application(r, t);
			}
		}
		break;
	case ATOM:
		if (var->cn == tree->cn && var->name == tree->name)
			/* [x] x -> I */
			r = new_combinator(COMB_I);
		else {
			/* another invalid expression for Lambda-I calculus. */
		}
		break;
	}
	return r;
}

bracket_abstraction_function
determine_bracket_abstraction(const char *algorithm_name)
{
	struct {
		const char *algorithm_name;  /* values match TK_ALGORITHM_NAME in lex.l */
		bracket_abstraction_function f;
	} afmap[] = {
		{"curry",  curry_bracket_abstraction},
		{"curry2", curry2_bracket_abstraction},
		{"church", ij_bracket_abstraction},
		{"grz",    grzegorczyk_bracket_abstraction},
		{"tromp",  tromp_bracket_abstraction},
		{"turner", turner_bracket_abstraction},
		{"btmk",   btmk_bracket_abstraction}
	};
	int i;
	bracket_abstraction_function func = (bracket_abstraction_function)NULL;

	for (i = 0; i < sizeof(afmap)/sizeof(afmap[0]); ++i)
	{
		if (!strcmp(afmap[i].algorithm_name, algorithm_name))
		{
			func = afmap[i].f;
			break;
		}
	}

	return func;
}

