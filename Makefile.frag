$(srcdir)/typical-lexer.c: $(srcdir)/typical-lexer.re
	@(cd $(srcdir); $(RE2C) -b -o typical-lexer.c typical-lexer.re)

$(srcdir)/typical-parser.c: $(srcdir)/typical-parser.lemon
	@(cd $(srcdir); lemon typical-parser.lemon)
