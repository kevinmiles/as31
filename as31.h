/*
*/

#include <stdio.h>

struct opcode {
	char *name;
	int type;
	unsigned char *bytes;
};

struct symbol {
	char *name;
	char predefined;
	char type;
	long value;
	struct symbol *next;
};

#define UNDEF	0
#define LABEL	1

#define BIT	2
#define IMEM	3
#define EMEM	3
#define CONST	4
#define REG	5

struct mode {
	unsigned char mode;	
	unsigned char size;	
	unsigned char orval;
	unsigned char byte1;
	unsigned char byte2;
};

#define set_md(m,a)	((m).mode=(a))
#define set_sz(m,a)	((m).size=(a))
#define set_ov(m,a)	((m).orval=(a))
#define set_b1(m,a)	((m).byte1=(a))
#define set_b2(m,a)	((m).byte2=(a))

#define get_md(m)	((m).mode)
#define get_sz(m)	((m).size)
#define get_ov(m)	((m).orval)
#define get_b1(m)	((m).byte1)
#define get_b2(m)	((m).byte2)

struct value {
	long v;
	unsigned char d;
};

union ystack {
	long value;
	struct value val;
	struct opcode *op;
	struct symbol *sym;
	struct mode mode;
	char *str;
};

#define YYSTYPE union ystack

#define isbmram(a)	(((a)&0xf0)==0x20)

#define isbmsfr(a)	(((a)&0x80) && !((a) & 0x07))

#define size8		(~0x00ff)
#define size11		(~0x07ff)
#define size13		(~0x1fff)
#define size16		(~0xffff)

#define size10		(~0x03ff)
#define size12		(~0x0fff)
#define size15		(~0x7fff)

#define isbit8(v)	( !( ((v)>=0) ? (v)&size8 : -(v)>=128) )
#define isbit11(v)	( !( ((v)>=0) ? (v)&size11 : (-(v))&size10 ) )
#define isbit13(v)	( !( ((v)>=0) ? (v)&size13 : (-(v))&size12 ) )
#define isbit16(v)	( !( ((v)>=0) ? (v)&size16 : (-(v))&size15 ) )

#define HASHTABSIZE		4999

#define pass1			(!pass)
#define pass2			(pass)

extern int yylex(void);
extern int seek_eol(void);
extern const char *get_last_token(void);
extern int lineno;

extern int yyparse(void);
extern void clear_location_counter(void);

extern void emitusage(void);
extern const char *emit_extension(const char *ftype);
extern const char *emit_desc_lookup(int num);
extern const char *emit_desc_to_name_lookup(const char *desc);
extern int emitopen(const char *file, const char *ftype, const char *arg);
extern void emitclose(void);
extern void emitaddr(unsigned long a);
extern void emitbyte(int b);

extern struct opcode *lookop(const char *s);
extern struct symbol *looksym(const char *s);
extern void syminit(void);
extern void freesym(void);

extern int run_as31(const char *infile, int lst, int use_stdout,
        const char *fmt, const char *arg);
extern void error(const char *fmt, ...);
extern void warn(const char *fmt, ...);
extern void mesg_f(const char *fmt, ...);
extern int dashl;
extern int pass;
extern int abort_asap;
extern unsigned long lc;
extern FILE *listing;

extern void mesg(const char *str);
