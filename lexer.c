/*
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "as31.h"
#include "parser.h"

int lineno;
int last_token_len=0;
char last_token[256], prev_token[256];

static char line[100];
static char *lineptr=line;

int get_ch(void)
{
    register int c;

    c = getchar();
    if (last_token_len >=0 && last_token_len < 254 &&
       c != EOF && (last_token_len > 0 || !isspace(c))) {
        last_token[last_token_len++] = c;
    }
    if( c != EOF && lineptr - line < sizeof(line) )
        *lineptr++ = c;
    return(c);
}

void unget_ch(int c)
{
    ungetc(c,stdin);
    if( lineptr > line )
        lineptr--;
}

int seek_eol(void)
{
    int c;

    last_token_len = -1;
    do {
        c = get_ch();
    } while (c != '\n' && c != EOF);
    if (c == '\n') {
        unget_ch(c);
        return 1;
    }
    return 0;
}

const char *get_last_token(void)
{
    return prev_token;
}

int yylex(void)
{
    static int nl_flag=0;
    register int c;
    char buf[1024];
    char *p;
    struct symbol *sym;
    struct opcode *op;

    int octal=0,hex=0,decimal=0,binary=0;
    register long value = 0;

    if (last_token_len > 0) {
        last_token[last_token_len] = '\0';
        strcpy(prev_token, last_token);
    } else {
        *prev_token = '\0';
    }
    last_token_len=0;

    if( nl_flag ) {
        nl_flag = 0;
        lineno++;
    }

    for(;;) {
        c = get_ch();
        switch(c) {
        case EOF: return(EOF);
        case ' ':
        case '\r':
        case '\t':
            break;

        case '\n':
            nl_flag = 1;
            yylval.str = line;
            *lineptr = '\0';
            lineptr = line;
            return('\n');

        case ';':
            while((c=get_ch()) != EOF && c!='\n');
            nl_flag= 1;
            yylval.str = line;
            *lineptr = '\0';
            lineptr = line;
            return(c);

        case '"':
            p = buf;
            while((c=get_ch()) != EOF && c!='"' && c!='\n') {
                if( c == '\\' ) {
                    switch(c=get_ch()) {
                    case 'n': c = '\n'; break;
                    case 'r': c = '\r'; break;
                    case 't': c = '\t'; break;
                    case 'b': c = '\b'; break;
                    case '"': c = '"'; break;
                    case '\\': c = '\\'; break;
                    default:
                      warn("Invalid escape character: \\%c",c);
                    }
                }
                if( p-buf<sizeof(buf)-1 ) {
                    *p++ = c;
                } else {
                   error("String constant longer than %d bytes",
                        sizeof(buf));
                }
            }
            *p = '\0';
            if( c == '\n' || c == EOF ) {
                warn("String terminated improperly");
                unget_ch(c);
            }

            if (pass1) {
                yylval.value = strlen(buf);
            } else {
                p = (char *)malloc(strlen(buf) + 1);
                if (p == NULL) {
                    error("Cannot allocate %d bytes",
                        strlen(buf) + 1);
                    yylval.str = "as31 ran out of memory!";
                    return(STRING);
                }
                strcpy(p, buf);
                yylval.str = p;
            }
            return(STRING);

        case '.':
            if( (c=get_ch())>='0' && c<='7' ) {
                yylval.value = c-'0';
                return(BITPOS);
            }
            unget_ch(c);
            return('.');

        case '\'':
            c = get_ch();
            if( c=='\\' ) {
                switch(c=get_ch()) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'b': c = '\b'; break;
                case '0': c = 0; break;
                case 'o': c = 0; break;
                case 'O': c = 0; break;
                case '\\': c = '\\'; break;
                case '\'': c = '\''; break;
                default:
                    warn("Invalid escape character: \\%c",c);
                }
            }
            if( get_ch() != '\'' )
                warn("Missing quote in character constant");
            yylval.value = c;
            return(VALUE);

        case '0':   /* parse a number           */
        case '1':   /* could be followed by a:      */
        case '2':   /*  'b','B' - Binary        */
        case '3':   /*  'h','H' - Hex           */
        case '4':   /*  'd','D' - Decimal       */
        case '5':   /*  'o','O' - Octal         */
        case '6':   /* *** Numbers must start with a digit  */
        case '7':   /* Numbers could be also preceeded by:  */
        case '8':   /*  0x  - Hex           */
        case '9':   /*  0b  - binary        */

            p = buf;
            do {
                if( p-buf<sizeof(buf)-1 )
                    *p++ = c;
                c = get_ch();
            } while( c=='H' || c=='h' || c=='O' || c=='o' ||
                    c=='x' || c=='X' || isxdigit(c) );
            unget_ch(c);
            *p = '\0';

            /* Check any preceeding chars */
            if( buf[0]=='0' && (buf[1]=='x' || buf[1]=='X') ) {
                    hex++;
                    buf[1] = '0';
            } else {
                if( buf[0]=='0' &&
                  (buf[1]=='b' || buf[1]=='B') &&
                  *(p-1) != 'h' && *(p-1) != 'H') {
                    binary++;
                    buf[1] = '0';
                }
            }

            /* check any trailing chars */
            c = *(p-1);
            if( !hex && (c=='b' || c=='B') )
                { binary++; *(p-1) = '\0'; }
            else if( c=='H' || c=='h' )
                { hex++; *(p-1) = '\0'; }
            else if( !hex && (c=='D' || c=='d') )
                { decimal++; *(p-1) = '\0'; }
            else if( c=='O' || c=='o' )
                { octal++; *(p-1) = '\0'; }
            else if( !hex && !octal && !binary) decimal++;

            if (binary) {
                if (hex) warn("ambiguous number, bin or hex");
                if (decimal) warn("ambiguous number, bin or dec");
                if (octal) warn("ambiguous number, bin or oct");
                for(p=buf; *p; p++ ) {
                    if( *p=='1' ) value = value * 2 + 1;
                    else if( *p=='0' ) value = value * 2;
                    else
                      warn("Invalid binary digit: %c",*p);
                }
                yylval.value = value;
                return(VALUE);
            }

            if (hex) {
                if (binary) warn("ambiguous number, hex or bin");
                if (decimal) warn("ambiguous number, hex or dec");
                if (octal) warn("ambiguous number, hex or oct");
                for(p=buf; *p; p++ ) {
                    value <<= 4;
                    if( isdigit(*p) )
                        value += *p-'0';
                    else if( *p>='a' && *p<='f' )
                        value += *p-'a'+ 10;
                    else if( *p>='A' && *p<='F' )
                        value += *p-'A'+ 10;
                    else
                      warn("Invalid hex digit: %c",*p);
                }
                yylval.value = value;
                return(VALUE);
            }

            if (decimal) {
                if (hex) warn("ambiguous number, dec or hex");
                if (binary) warn("ambiguous number, dec or bin");
                if (octal) warn("ambiguous number, dec or oct");
                for(p=buf; *p; p++ ) {
                    if( isdigit(*p) )
                        value = value*10 + *p-'0';
                    else
                       warn("Invalid decimal digit: %c",*p);
                }
                yylval.value = value;
                return(VALUE);
            }

            if (octal) {
                if (hex) warn("ambiguous number, oct or hex");
                if (binary) warn("ambiguous number, oct or bin");
                if (decimal) warn("ambiguous number, oct or dec");
                for(p=buf; *p; p++ ) {
                    if( *p>='0' && *p<='7' )
                        value = (value << 3) + (*p - '0');
                    else
                       warn("Invalid octal digit: %c",*p);
                }
                yylval.value = value;
                return(VALUE);
            }

        default:
            if( isalpha(c) || c=='_' ) {
                p = buf;
                do {
                    if( p-buf<sizeof(buf)-1 )
                        *p++ = c;
                    c = get_ch();
                } while( isalnum(c) || c=='_' );
                *p = '\0';
                unget_ch(c);
                if ( (op = lookop(buf)) != NULL ) {
                    yylval.op = op;
                    return(op->type);
                }
                sym = looksym(buf);
                yylval.sym = sym;
                return(SYMBOL);
            } else
                return(c);
        }
    }

}
