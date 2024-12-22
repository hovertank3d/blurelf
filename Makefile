PROG=	blurelf
MK_MAN= no
SRCS=	blurelf.c

LDADD= -lm

CFLAGS+= -Wall -Werror -Wextra -Wpedantic -std=gnu99 -O2 -g

.include <bsd.prog.mk>