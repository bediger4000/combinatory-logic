#include <stdio.h>  /* NULL */

#include <node.h>
#include <bracket_abstraction.h>

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
	case COMBINATOR:
		if (var->cn == tree->cn)
		{
			if (COMB_NONE == var->cn)
			{
				if (var->name == tree->name)
					r = 1;
			} else 
				r = 1;
		} else if (var->name == tree->name)
			r = 1;
		break;
	default:
		break;
	}
	return r;
}

/* Curry-Feys bracket abstraction
	[x]     -> I
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
	case COMBINATOR:
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
	default:
		break;
	}
	return r;
}

/* Simple Turner bracket abstraction
	[x]     -> I
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
							arena_copy_graph(tree->right)
						),
						turner_bracket_abstraction(var, tree->left)
					);
				}
			} else if (var_appears_in_graph(var, tree->right)) {
				if (COMBINATOR == tree->right->typ && var->name == tree->right->name)
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
	case COMBINATOR:
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
	default:
		break;
	}
	return r;
}
