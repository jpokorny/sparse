/*
 * Example trivial client program that uses the sparse library
 * to tokenize, pre-process and parse a C file, and prints out
 * the results.
 *
 * Copyright (C) 2003 Transmeta Corp, all rights reserved.
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"
#include "token.h"
#include "parse.h"
#include "symbol.h"
#include "expression.h"

static void emit_blob(struct symbol *sym)
{
	int size = sym->bit_size;
	int alignment = sym->ctype.alignment;
	const char *name = show_ident(sym->ident);

	if (size <= 0) {
		warn(sym->pos, "emitting insized symbol");
		size = 8;
	}
	if (size & 7)
		warn(sym->pos, "emitting symbol of size %d bits\n", size);
	size = (size+7) >> 3;
	if (alignment < 1)
		alignment = 1;
	if (!(size & (alignment-1))) {
		switch (alignment) {
		case 1:
			printf("unsigned char %s[%d];\n", name, size);
			return;
		case 2:
			printf("unsigned short %s[%d];\n", name, (size+1) >> 1);
			return;
		case 4:
			printf("unsigned int %s[%d];\n", name, (size+3) >> 2);
			return;
		}
	}
	printf("unsigned char %s[%d] __attribute__((aligned(%d)));\n",
		name, size, alignment);
	return;
}

static void emit_fn(struct symbol *sym)
{
	const char *name = show_ident(sym->ident);
	printf("%s();\n", name);	
}

void emit_symbol(struct symbol *sym, void *_parent, int flags)
{
	struct symbol *ctype;

	evaluate_symbol(sym);
	if (sym->type != SYM_NODE) {
		warn(sym->pos, "I really want to emit nodes, not pure types!");
		return;
	}

	ctype = sym->ctype.base_type;
	if (!ctype)
		return;
	switch (ctype->type) {
	case SYM_NODE:
	case SYM_PTR:
	case SYM_ARRAY:
	case SYM_STRUCT:
	case SYM_UNION:
	case SYM_BASETYPE:
		emit_blob(sym);
		return;
	case SYM_FN:
		emit_fn(sym);
		return;
	default:
		warn(sym->pos, "what kind of strange node do you want me to emit again?");
		return;
	}
}

int main(int argc, char **argv)
{
	int fd;
	char *filename = argv[1];
	struct token *token;
	struct symbol_list *list = NULL;

	// Initialize symbol stream first, so that we can add defines etc
	init_symbols();

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		die("No such file: %s", argv[1]);

	// Tokenize the input stream
	token = tokenize(filename, fd, NULL);
	close(fd);

	// Pre-process the stream
	token = preprocess(token);

	// Parse the resulting C code
	translation_unit(token, &list);

	// Do type evaluation and simplify
	symbol_iterate(list, emit_symbol, NULL);

	return 0;
}