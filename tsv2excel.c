/* This is a simple tsv to excel format convertor. prerequisite: libxlsxwriter
 *
 * Copyright  shiquan.cn@gmail.com
 *
 * Free to distribute, use, modify, share for any nonprofit or profit purpose.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <zlib.h>
#include <getopt.h>
#include "kseq.h"
#include "kstring.h"
#include "xlsxwriter.h" // libxlsxwriter, please see https://github.com/jmcnamara/libxlsxwriter

KSTREAM_INIT(gzFile, gzread, 8192)

#define INIT_KSTRING {0, 0, 0}

lxw_doc_properties properties = {
    .comments = "Created by tsv2excel.",
};

struct worksheet_theme_choice {
    lxw_format *head;
    lxw_format *odd;
    lxw_format *even;
};

struct worksheet_compact {
    char *name; // sheet name
    char *file; // tsv name
    struct worksheet_theme_choice theme;
    lxw_worksheet *worksheet;    
};

struct opts {
    int no_header;
    int col_limits;
    int n_sheet;
    uint8_t comment_char;
    struct worksheet_compact *sheets;
    char *out;
    char *in; // temp parameter for input file,
};

struct opts args = {
    .no_header = 0,
    .col_limits = 0,
    .n_sheet = 0,
    .comment_char = 0,
    .sheets = NULL,
    .out = NULL,
    .in = NULL,
};

int32_t color_names[] = {
    0xCDFECE,
    0x9BCDFD,
    0xCB9CFC,
};

void usage()
{
    fprintf(stderr, "tsv2excel -o out.xlsx in.tsv\n");
    fprintf(stderr, "A lightwight tool to convert tsv files to a excel file.\n");
    fprintf(stderr, "Author: shiquan@genomics.cn\n");
    fprintf(stderr, "https://github.com/shiquan/tsv2excel\n");
    exit(1);
}
void default_themes_init(lxw_workbook *wb, struct worksheet_theme_choice *theme, int n)
{
    assert(theme != NULL);
    n = n < 0 ? 0 : n%3;
    theme->head = workbook_add_format(wb);
    theme->even = workbook_add_format(wb);
    format_set_bold(theme->head);
    format_set_bg_color(theme->even, color_names[n]);
}

static struct option lopt[] = {
    {"out", required_argument, NULL,'o'},
    {"no_header", no_argument, NULL, 'H'},
    {NULL,0,NULL,0}
//    {"in", required_argument, }
};

/*only accept one input tsv for now*/
void init_args(int argc, char *argv[])
{
    int c;
    int has_defined_out = 0;
    while ((c=getopt_long(argc, argv, "o:H", lopt, NULL))>=0) {
	switch(c) {
	    case 'o':
		args.out = strdup(optarg);
		has_defined_out = 1;
		break;
	    case 'H':
		args.no_header = 1;
		break;
	    default:
		fprintf(stderr,"unknown argument: %s", optarg);
		exit(1);
	}
    }
    char *fname = NULL;
    if (optind == argc) {
	if (!isatty(fileno((FILE*)stdin))) fname = "-";
	else usage();	    

	if (has_defined_out == 0)
	    args.out = strdup("temp.xlsx");
    } else {
	fname = argv[optind];
	char *ss = fname;
	kstring_t str = INIT_KSTRING;
	int length = strlen(fname);
	length --;
	for (;length>0 && ss[length] != '.'; length --) {
	    if (ss[length] == '/') {
		length = 0;
		break;
	    }
	}
	if (length >0) {
	    kputsn(fname, length, &str);
	} else {
	    kputs(fname, &str);
	}
	kputs(".xlsx", &str);
	args.out = strdup(str.s);
	free(str.s);
    }
    args.in = strdup(fname);
    //  only accept one input tsv file for now
    args.n_sheet = 1;
    args.sheets = (struct worksheet_compact*)calloc(1, sizeof(struct worksheet_compact));
    struct worksheet_compact *ws = &args.sheets[0];
    ws->name = NULL;
    ws->file = strdup(args.in);

    ws->worksheet = NULL;
    
}

void prase_tsv_files(lxw_workbook *wb)
{
    int i;
    for (i=0; i<args.n_sheet; ++i) {
	struct worksheet_compact *wc = &args.sheets[i];
	if (wc==NULL) continue;	
	wc->worksheet = workbook_add_worksheet(wb, wc->name);
	default_themes_init(wb, &wc->theme, i);
	gzFile fp = strcmp(wc->file,"-") ? gzopen(wc->file, "r") : gzdopen(fileno(stdin),"r") ;
	kstream_t *ks;
	kstring_t str = INIT_KSTRING;
	int dret;
	int line = 0;
	ks = ks_init(fp);
	while(ks_getuntil(ks, 2, &str, &dret) >= 0) {
	    int *splits;
	    int j;
	    int nfields = 0;
	    if (args.comment_char != 0 && str.s[0] == args.comment_char) {
		str.l=0;
		continue;
	    }
	    splits = ksplit(&str, '\t', &nfields);
#ifdef _DEBUG_MODE
	    fprintf(stderr,"[debug] [%s] %d: %s\n",__func__, nfields,str.s);
#endif
	    if (splits == NULL) {
		str.l = 0;
		continue;
	    }
#ifdef _DEBUG_MODE
	    fprintf(stderr,"[debug] [%s] splits[0]: %d\n",__func__,splits[0]);
#endif
	    
	    nfields = args.col_limits > 0 && args.col_limits < nfields + splits[0] ? args.col_limits : nfields;
	    lxw_format *format = NULL;
	    if (args.no_header == 0 && line==0) 
		format = wc->theme.head;
	    else
		format = line&1 ? wc->theme.even : wc->theme.odd;

	    for (j=0; j<splits[0]; ++j)
		worksheet_write_string(wc->worksheet, line, j, NULL, format);
	    
	    for (j=0; j<nfields; ++j) {		
		worksheet_write_string(wc->worksheet, line, j+splits[0], str.s+splits[j], format);
	    }

	    line++;
	    free(splits);
	    str.l = 0;
	}
	if (str.m) free(str.s);
	gzclose(fp);
    }

}
void clean_args()
{
    int i;
    for (i=0; i<args.n_sheet; ++i) {
	struct worksheet_compact *wc = &args.sheets[i];
	if (wc->name) free(wc->name);
	if (wc->file) free(wc->file);	
    }
    if (args.sheets) free(args.sheets);
    if (args.out) free(args.out);
    if (args.in) free(args.in);
}
int main(int argc, char **argv)
{
    init_args(argc, argv);
    lxw_workbook  *workbook  = workbook_new(args.out);
    prase_tsv_files(workbook);
    clean_args();
    return workbook_close(workbook);    
}
