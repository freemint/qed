/*
 * resource set indices for qed
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        874
 * Number of Bitblks:        7
 * Number of Iconblks:       1
 * Number of Color Iconblks: 0
 * Number of Color Icons:    0
 * Number of Tedinfos:       118
 * Number of Free Strings:   44
 * Number of Free Images:    0
 * Number of Objects:        682
 * Number of Trees:          32
 * Number of Userblks:       0
 * Number of Images:         9
 * Total file size:          33896
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "qed"
#endif
#undef RSC_ID
#ifdef qed
#define RSC_ID qed
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 874
#define NUM_FRSTR 44
#define NUM_UD 0
#define NUM_IMAGES 9
#define NUM_BB 7
#define NUM_FRIMG 0
#define NUM_IB 1
#define NUM_CIB 0
#define NUM_TI 118
#define NUM_OBS 682
#define NUM_TREE 32
#endif



#define VERSION            0 /* form/dialog */
#define RSC_VERSION        3 /* STRING in tree VERSION */

#define MENUTREE           1 /* menu */
#define TDESK              3 /* TITLE in tree MENUTREE */
#define TFILE              4 /* TITLE in tree MENUTREE */
#define TEDIT              5 /* TITLE in tree MENUTREE */
#define TFENSTER           6 /* TITLE in tree MENUTREE */
#define TSEARCH            7 /* TITLE in tree MENUTREE */
#define TSPEZIAL           8 /* TITLE in tree MENUTREE */
#define TOPTIONS           9 /* TITLE in tree MENUTREE */
#define TSHELL            10 /* TITLE in tree MENUTREE */
#define MABOUT            13 /* STRING in tree MENUTREE */
#define MNEWTEXT          22 /* STRING in tree MENUTREE */
#define MOPEN             23 /* STRING in tree MENUTREE */
#define MADD              24 /* STRING in tree MENUTREE */
#define MNEWPROJ          25 /* STRING in tree MENUTREE */
#define MOPENBIN          26 /* STRING in tree MENUTREE */
#define MSAVE             28 /* STRING in tree MENUTREE */
#define MSAVENEW          29 /* STRING in tree MENUTREE */
#define MABAND            30 /* STRING in tree MENUTREE */
#define MDELETE           31 /* STRING in tree MENUTREE */
#define MPRINT            33 /* STRING in tree MENUTREE */
#define MINFO             34 /* STRING in tree MENUTREE */
#define MQUIT2            36 /* STRING in tree MENUTREE */
#define MQUIT             37 /* STRING in tree MENUTREE */
#define MUNDO             39 /* STRING in tree MENUTREE */
#define MCUT              41 /* STRING in tree MENUTREE */
#define MCOPY             42 /* STRING in tree MENUTREE */
#define MPASTE            43 /* STRING in tree MENUTREE */
#define MSEL              45 /* STRING in tree MENUTREE */
#define MSELALL           46 /* STRING in tree MENUTREE */
#define MFORMAT           48 /* STRING in tree MENUTREE */
#define MSWAP             49 /* STRING in tree MENUTREE */
#define MLEFT             51 /* STRING in tree MENUTREE */
#define MRIGHT            52 /* STRING in tree MENUTREE */
#define MSORT             54 /* STRING in tree MENUTREE */
#define MSMALLBIG         56 /* STRING in tree MENUTREE */
#define MBIG              57 /* STRING in tree MENUTREE */
#define MSMALL            58 /* STRING in tree MENUTREE */
#define MCAPS             59 /* STRING in tree MENUTREE */
#define MCYCLE            61 /* STRING in tree MENUTREE */
#define MCLOSE            62 /* STRING in tree MENUTREE */
#define MTOPLINE          64 /* STRING in tree MENUTREE */
#define MHINTER           66 /* STRING in tree MENUTREE */
#define MNEBEN            67 /* STRING in tree MENUTREE */
#define MUNTER            68 /* STRING in tree MENUTREE */
#define MFIND             70 /* STRING in tree MENUTREE */
#define MFNDGOON          71 /* STRING in tree MENUTREE */
#define MFNDFILE          73 /* STRING in tree MENUTREE */
#define MGOTO             75 /* STRING in tree MENUTREE */
#define MSETMARK          77 /* STRING in tree MENUTREE */
#define MMARKE1           78 /* STRING in tree MENUTREE */
#define MMARKE2           79 /* STRING in tree MENUTREE */
#define MMARKE3           80 /* STRING in tree MENUTREE */
#define MMARKE4           81 /* STRING in tree MENUTREE */
#define MMARKE5           82 /* STRING in tree MENUTREE */
#define MSRECORD          84 /* STRING in tree MENUTREE */
#define MSPLAY            85 /* STRING in tree MENUTREE */
#define MSMAKRO           86 /* STRING in tree MENUTREE */
#define MZEICHEN          88 /* STRING in tree MENUTREE */
#define MUMLAUTE          89 /* STRING in tree MENUTREE */
#define MFEHLER           91 /* STRING in tree MENUTREE */
#define MFEHLERSPRUNG     92 /* STRING in tree MENUTREE */
#define MSSTRIP           94 /* STRING in tree MENUTREE */
#define MSTAB2LZ          95 /* STRING in tree MENUTREE */
#define MSLZ2TAB          96 /* STRING in tree MENUTREE */
#define MOEINF            98 /* STRING in tree MENUTREE */
#define MOOVERW           99 /* STRING in tree MENUTREE */
#define MOFONT           101 /* STRING in tree MENUTREE */
#define MGLOBALO         102 /* STRING in tree MENUTREE */
#define MLOCALOP         103 /* STRING in tree MENUTREE */
#define MSYNTAXOP        104 /* STRING in tree MENUTREE */
#define MAUTOSAVE        105 /* STRING in tree MENUTREE */
#define MKLAMMER         106 /* STRING in tree MENUTREE */
#define MPRINTER         107 /* STRING in tree MENUTREE */
#define MPROJEKT         109 /* STRING in tree MENUTREE */
#define MTAKEPRJ         110 /* STRING in tree MENUTREE */
#define MSOPTION         112 /* STRING in tree MENUTREE */
#define TSHELL_BOX       113 /* BOX in tree MENUTREE */
#define MSOPT            114 /* STRING in tree MENUTREE */
#define MSQUIT           115 /* STRING in tree MENUTREE */
#define MSMAKEFILE       117 /* STRING in tree MENUTREE */
#define MSCOMP           119 /* STRING in tree MENUTREE */
#define MSMAKE           120 /* STRING in tree MENUTREE */
#define MSMAKEALL        121 /* STRING in tree MENUTREE */
#define MSLINK           122 /* STRING in tree MENUTREE */
#define MSEXEC           123 /* STRING in tree MENUTREE */
#define MSMAKEEXEC       124 /* STRING in tree MENUTREE */
#define MSSHELL          125 /* STRING in tree MENUTREE */

#define ABOUT              2 /* form/dialog */
#define AVERSION          11 /* TEXT in tree ABOUT */ /* max len 10 */
#define ADATUM            12 /* TEXT in tree ABOUT */ /* max len 11 */
#define AINFO             14 /* BUTTON in tree ABOUT */
#define AOK               15 /* BUTTON in tree ABOUT */

#define ABOUT2             3 /* form/dialog */
#define AMINT              4 /* TEXT in tree ABOUT2 */ /* max len 4 */
#define AGEM               6 /* TEXT in tree ABOUT2 */ /* max len 4 */
#define ACF                8 /* TEXT in tree ABOUT2 */ /* max len 4 */

#define PRINT              4 /* form/dialog */
#define DALL               3 /* BUTTON in tree PRINT */
#define DBLOCK             4 /* BUTTON in tree PRINT */
#define DGEMDOS            6 /* BUTTON in tree PRINT */
#define DGDOS              7 /* BUTTON in tree PRINT */
#define DZNUM              9 /* BUTTON in tree PRINT */
#define DFEED             10 /* BUTTON in tree PRINT */
#define DSNUM             11 /* BUTTON in tree PRINT */
#define DCHECK            12 /* BUTTON in tree PRINT */
#define DRANDLEN          14 /* FTEXT in tree PRINT */ /* max len 3 */
#define DOK1              16 /* BUTTON in tree PRINT */
#define DPRINT            17 /* BUTTON in tree PRINT */

#define TEXTINFO           5 /* form/dialog */
#define INFNAME            4 /* TEXT in tree TEXTINFO */ /* max len 31 */
#define INFDATUM           6 /* TEXT in tree TEXTINFO */ /* max len 10 */
#define INFZEIT            8 /* TEXT in tree TEXTINFO */ /* max len 8 */
#define INFBYTES          10 /* TEXT in tree TEXTINFO */ /* max len 8 */
#define INFZEILE          12 /* TEXT in tree TEXTINFO */ /* max len 8 */
#define INFTOS            14 /* BUTTON in tree TEXTINFO */
#define INFUNIX           15 /* BUTTON in tree TEXTINFO */
#define INFMAC            16 /* BUTTON in tree TEXTINFO */
#define INFOK             17 /* BUTTON in tree TEXTINFO */
#define INFRAUS           18 /* BUTTON in tree TEXTINFO */

#define PRJINFO            6 /* form/dialog */
#define PRJNAME            3 /* TEXT in tree PRJINFO */ /* max len 31 */
#define PRJDATUM           5 /* TEXT in tree PRJINFO */ /* max len 10 */
#define PRJZEIT            7 /* TEXT in tree PRJINFO */ /* max len 8 */
#define PRJFILES           9 /* TEXT in tree PRJINFO */ /* max len 5 */
#define PRJLEN            11 /* TEXT in tree PRJINFO */ /* max len 8 */
#define PRJZEILE          13 /* TEXT in tree PRJINFO */ /* max len 8 */
#define IPRJSCAN          14 /* BUTTON in tree PRJINFO */

#define BLOCKINFO          7 /* form/dialog */
#define BLONAME            3 /* TEXT in tree BLOCKINFO */ /* max len 31 */
#define BLOBYTES           5 /* TEXT in tree BLOCKINFO */ /* max len 8 */
#define BLOZEILE           7 /* TEXT in tree BLOCKINFO */ /* max len 8 */

#define SORT               8 /* form/dialog */
#define BSZEILEN           2 /* TEXT in tree SORT */ /* max len 25 */
#define BSSPALTEN          3 /* TEXT in tree SORT */ /* max len 25 */
#define BSUP               4 /* BUTTON in tree SORT */
#define BSDOWN             5 /* BUTTON in tree SORT */
#define BSGRKL             6 /* BUTTON in tree SORT */
#define BSKRIT             7 /* BUTTON in tree SORT */
#define BSORT              8 /* BUTTON in tree SORT */
#define BSABBRUCH          9 /* BUTTON in tree SORT */

#define REPLACE            9 /* form/dialog */
#define RPSSTR             3 /* STRING in tree REPLACE */
#define RPTEXT1            4 /* FTEXT in tree REPLACE */ /* max len 50 */
#define RPSHIST            5 /* BOXCHAR in tree REPLACE */
#define RPGRKL             6 /* BUTTON in tree REPLACE */
#define RPWILD             7 /* BUTTON in tree REPLACE */
#define RPWORT             8 /* BUTTON in tree REPLACE */
#define RPVORW            10 /* BUTTON in tree REPLACE */
#define RPRUCKW           11 /* BUTTON in tree REPLACE */
#define RPGLOBAL          13 /* BUTTON in tree REPLACE */
#define RPCURSOR          14 /* BUTTON in tree REPLACE */
#define RPROUND           15 /* BUTTON in tree REPLACE */
#define RPESTR            17 /* STRING in tree REPLACE */
#define RPTEXT2           18 /* FTEXT in tree REPLACE */ /* max len 50 */
#define RPEHIST           19 /* BOXCHAR in tree REPLACE */
#define RPFIRST           21 /* BUTTON in tree REPLACE */
#define RPOPTION          22 /* BUTTON in tree REPLACE */
#define RPALL             23 /* BUTTON in tree REPLACE */
#define RPHELP            24 /* BOXTEXT in tree REPLACE */ /* max len 5 */
#define RPERSATZ          25 /* BUTTON in tree REPLACE */
#define RPOK              26 /* BUTTON in tree REPLACE */
#define RPRAUS            27 /* BUTTON in tree REPLACE */

#define FINDFILE          10 /* form/dialog */
#define FFTITLE            1 /* STRING in tree FINDFILE */
#define FFSELP             3 /* BUTTON in tree FINDFILE */
#define FFPATH             4 /* TEXT in tree FINDFILE */ /* max len 50 */
#define FFREK              6 /* BUTTON in tree FINDFILE */
#define FFMASK             7 /* FTEXT in tree FINDFILE */ /* max len 15 */
#define FFSTR              9 /* STRING in tree FINDFILE */
#define FFTEXT            10 /* FTEXT in tree FINDFILE */ /* max len 50 */
#define FFHIST            11 /* BOXCHAR in tree FINDFILE */
#define FFGRKL            12 /* BUTTON in tree FINDFILE */
#define FFWILD            13 /* BUTTON in tree FINDFILE */
#define FFWORT            14 /* BUTTON in tree FINDFILE */
#define FFHELP            15 /* BOXTEXT in tree FINDFILE */ /* max len 5 */
#define FFOK              16 /* BUTTON in tree FINDFILE */
#define FFABBRUCH         17 /* BUTTON in tree FINDFILE */

#define REPASK            11 /* form/dialog */
#define AL_QST             1 /* IMAGE in tree REPASK */
#define RAJA               3 /* BUTTON in tree REPASK */
#define RANEIN             4 /* BUTTON in tree REPASK */
#define RAALL              5 /* BUTTON in tree REPASK */
#define RAENDE             6 /* BUTTON in tree REPASK */

#define POS               12 /* form/dialog */
#define GZEILE             3 /* FTEXT in tree POS */ /* max len 6 */
#define GSPALTE            5 /* FTEXT in tree POS */ /* max len 6 */
#define GOK                6 /* BUTTON in tree POS */
#define GRAUS              7 /* BUTTON in tree POS */

#define MARKEN            13 /* form/dialog */
#define MRKOK              2 /* BUTTON in tree MARKEN */
#define MRK1               3 /* BUTTON in tree MARKEN */
#define MRK2               4 /* BUTTON in tree MARKEN */
#define MRK3               5 /* BUTTON in tree MARKEN */
#define MRK4               6 /* BUTTON in tree MARKEN */
#define MRK5               7 /* BUTTON in tree MARKEN */
#define MRKTXT1            8 /* FTEXT in tree MARKEN */ /* max len 12 */
#define MRKTXT2            9 /* FTEXT in tree MARKEN */ /* max len 12 */
#define MRKTXT3           10 /* FTEXT in tree MARKEN */ /* max len 12 */
#define MRKTXT4           11 /* FTEXT in tree MARKEN */ /* max len 12 */
#define MRKTXT5           12 /* FTEXT in tree MARKEN */ /* max len 12 */
#define MRKRAUS           13 /* BUTTON in tree MARKEN */

#define FUNKTION          14 /* form/dialog */
#define FNSTR              2 /* STRING in tree FUNKTION */
#define FNKEY              3 /* BOXTEXT in tree FUNKTION */ /* max len 4 */
#define FNNAME             5 /* FTEXT in tree FUNKTION */ /* max len 8 */
#define FNRECORD           6 /* BUTTON in tree FUNKTION */
#define FNBLOCK            7 /* BUTTON in tree FUNKTION */
#define FNDELETE           8 /* BUTTON in tree FUNKTION */
#define FNOK               9 /* BUTTON in tree FUNKTION */

#define MAKROREP          15 /* form/dialog */
#define MREPANZ            3 /* FTEXT in tree MAKROREP */ /* max len 5 */
#define MREPOK             4 /* BUTTON in tree MAKROREP */
#define MREPRAUS           5 /* BUTTON in tree MAKROREP */

#define UMLAUTE           16 /* form/dialog */
#define UVSTR              2 /* STRING in tree UMLAUTE */
#define UVON               3 /* BOXTEXT in tree UMLAUTE */ /* max len 11 */
#define UNSTR              4 /* STRING in tree UMLAUTE */
#define UNACH              5 /* BOXTEXT in tree UMLAUTE */ /* max len 11 */
#define UKSTART            6 /* BUTTON in tree UMLAUTE */

#define FEHLER            17 /* form/dialog */
#define FEHLTEXT1          2 /* FTEXT in tree FEHLER */ /* max len 39 */
#define FEHLHELP           8 /* BOXTEXT in tree FEHLER */ /* max len 5 */
#define FEHLOK             9 /* BUTTON in tree FEHLER */
#define FELSTOP           10 /* BUTTON in tree FEHLER */

#define GLOBALOP          18 /* form/dialog */
#define GOBLEN             4 /* FTEXT in tree GLOBALOP */ /* max len 4 */
#define GOBEXT1            7 /* FTEXT in tree GLOBALOP */ /* max len 5 */
#define GOASAVE           14 /* BUTTON in tree GLOBALOP */
#define GOSAVEWIN         15 /* BUTTON in tree GLOBALOP */
#define GOCLIP            16 /* BUTTON in tree GLOBALOP */
#define GOBLINK           17 /* BUTTON in tree GLOBALOP */
#define GOCTRL            18 /* BUTTON in tree GLOBALOP */
#define GOWDIAL           19 /* BUTTON in tree GLOBALOP */
#define GOOLGA            20 /* BUTTON in tree GLOBALOP */
#define GOKLAMMER         21 /* BUTTON in tree GLOBALOP */
#define GOSYNTAX          22 /* BUTTON in tree GLOBALOP */
#define GOTRANS           24 /* FTEXT in tree GLOBALOP */ /* max len 4 */
#define GOFCSTR           28 /* STRING in tree GLOBALOP */
#define GOFCOL            29 /* BOX in tree GLOBALOP */
#define GOBLOCKFCOL       30 /* BOX in tree GLOBALOP */
#define GOBCSTR           31 /* STRING in tree GLOBALOP */
#define GOBCOL            32 /* BOX in tree GLOBALOP */
#define GOBLOCKBCOL       33 /* BOX in tree GLOBALOP */
#define GOHELPSEL         35 /* BUTTON in tree GLOBALOP */
#define GOHELPNAME        36 /* STRING in tree GLOBALOP */
#define GOAVWIN           38 /* BUTTON in tree GLOBALOP */
#define GOAVKEY           39 /* BUTTON in tree GLOBALOP */
#define GOHELP            40 /* BOXTEXT in tree GLOBALOP */ /* max len 5 */
#define GOOK              41 /* BUTTON in tree GLOBALOP */
#define GOABBR            42 /* BUTTON in tree GLOBALOP */

#define LOCALOP           19 /* form/dialog */
#define OTYPESTR           2 /* STRING in tree LOCALOP */
#define OTYPE              3 /* BOXTEXT in tree LOCALOP */ /* max len 8 */
#define OBOX               4 /* IBOX in tree LOCALOP */
#define OTAB               6 /* BUTTON in tree LOCALOP */
#define OTABSIZE           9 /* FTEXT in tree LOCALOP */ /* max len 2 */
#define OBACKUP           11 /* BUTTON in tree LOCALOP */
#define OKURZEL           13 /* BUTTON in tree LOCALOP */
#define OKURZELNAME       14 /* TEXT in tree LOCALOP */ /* max len 12 */
#define OEXT              15 /* FTEXT in tree LOCALOP */ /* max len 3 */
#define OEINRUCK          18 /* BUTTON in tree LOCALOP */
#define OUMBRUCH          19 /* BUTTON in tree LOCALOP */
#define OFORMLOAD         20 /* BUTTON in tree LOCALOP */
#define OFORMPASTE        21 /* BUTTON in tree LOCALOP */
#define OSHOWEND          22 /* BUTTON in tree LOCALOP */
#define OLINEAL           25 /* FTEXT in tree LOCALOP */ /* max len 4 */
#define OUMBTEXT          26 /* FTEXT in tree LOCALOP */ /* max len 12 */
#define OWORT             27 /* FTEXT in tree LOCALOP */ /* max len 34 */
#define LCHELP            28 /* BOXTEXT in tree LOCALOP */ /* max len 5 */
#define OOK               29 /* BUTTON in tree LOCALOP */
#define ORAUS             30 /* BUTTON in tree LOCALOP */

#define MUSTER            20 /* form/dialog */
#define MFIRST             2 /* FTEXT in tree MUSTER */ /* max len 5 */
#define MLAST             19 /* FTEXT in tree MUSTER */ /* max len 5 */
#define MOK               20 /* BUTTON in tree MUSTER */
#define MRAUS             21 /* BUTTON in tree MUSTER */

#define AUTOSAVE          21 /* form/dialog */
#define ASTEXT             3 /* BUTTON in tree AUTOSAVE */
#define ASTMIN             5 /* FTEXT in tree AUTOSAVE */ /* max len 2 */
#define ASTASK             7 /* BUTTON in tree AUTOSAVE */
#define ASPROJ             9 /* BUTTON in tree AUTOSAVE */
#define ASPMIN            11 /* FTEXT in tree AUTOSAVE */ /* max len 2 */
#define ASPASK            13 /* BUTTON in tree AUTOSAVE */
#define ASOK              14 /* BUTTON in tree AUTOSAVE */

#define KLAMMER           22 /* form/dialog */
#define KPAUF              3 /* FTEXT in tree KLAMMER */ /* max len 10 */
#define KPZU               5 /* FTEXT in tree KLAMMER */ /* max len 10 */
#define KPOK               6 /* BUTTON in tree KLAMMER */
#define KPABBRUCH          7 /* BUTTON in tree KLAMMER */

#define PRINTER           23 /* form/dialog */
#define DTREIBER           3 /* BUTTON in tree PRINTER */
#define DTREIBNAME         4 /* STRING in tree PRINTER */
#define DPLENGTH           6 /* FTEXT in tree PRINTER */ /* max len 3 */
#define DNLQ               7 /* BUTTON in tree PRINTER */
#define DPWIDTH            9 /* FTEXT in tree PRINTER */ /* max len 3 */
#define DDICHTESTR        10 /* STRING in tree PRINTER */
#define DDICHTE           11 /* BOXTEXT in tree PRINTER */ /* max len 10 */
#define DDEVICE           14 /* STRING in tree PRINTER */
#define DFONT             16 /* STRING in tree PRINTER */
#define DPTS              18 /* STRING in tree PRINTER */
#define DFONTSEL          19 /* BUTTON in tree PRINTER */
#define DOK2              20 /* BUTTON in tree PRINTER */

#define PRN_SUB           24 /* form/dialog */
#define PS_GFNAME          2 /* TEXT in tree PRN_SUB */ /* max len 28 */
#define PS_GFPTS           3 /* STRING in tree PRN_SUB */
#define PS_GFSEL           5 /* BUTTON in tree PRN_SUB */
#define PS_ALL             7 /* BUTTON in tree PRN_SUB */
#define PS_BLOCK           8 /* BUTTON in tree PRN_SUB */
#define PS_ZNUM           10 /* BUTTON in tree PRN_SUB */
#define PS_SNUM           11 /* BUTTON in tree PRN_SUB */
#define PS_RANDLEN        13 /* FTEXT in tree PRN_SUB */ /* max len 3 */

#define SEOPTION          25 /* form/dialog */
#define SENAME1            3 /* FTEXT in tree SEOPTION */ /* max len 8 */
#define SESTATUS          10 /* TEXT in tree SEOPTION */ /* max len 30 */
#define SESEARCH          11 /* BUTTON in tree SEOPTION */
#define SESAVE            12 /* BUTTON in tree SEOPTION */
#define SEIGNORECLOSE     13 /* BUTTON in tree SEOPTION */
#define SESUCH            14 /* BUTTON in tree SEOPTION */
#define SEOK              15 /* BUTTON in tree SEOPTION */
#define SSABBRUCH         16 /* BUTTON in tree SEOPTION */

#define AKTION            26 /* form/dialog */
#define ATEXT              1 /* TEXT in tree AKTION */ /* max len 28 */
#define ABOX1              5 /* BOX in tree AKTION */
#define ABOX2              6 /* BOX in tree AKTION */
#define AESC               7 /* TEXT in tree AKTION */ /* max len 17 */

#define POPUPS            27 /* form/dialog */
#define COLORPOP           1 /* BOX in tree POPUPS */
#define CPWHITE           18 /* STRING in tree POPUPS */
#define FUNCPOP           34 /* BOX in tree POPUPS */
#define POPF1             35 /* STRING in tree POPUPS */
#define DICHTEPOP         55 /* BOX in tree POPUPS */
#define DICHTEPICA        56 /* STRING in tree POPUPS */
#define DICHTEELITE       57 /* STRING in tree POPUPS */
#define DICHTECOND        58 /* STRING in tree POPUPS */
#define DRAGPOP           59 /* BOX in tree POPUPS */
#define DPALL             61 /* STRING in tree POPUPS */
#define DPPATH            62 /* STRING in tree POPUPS */
#define DPNAME            63 /* STRING in tree POPUPS */
#define DPRELPATH         64 /* STRING in tree POPUPS */
#define DPINHALT          66 /* STRING in tree POPUPS */
#define UMLAUTPOP2        67 /* BOX in tree POPUPS */
#define UPTOST            68 /* STRING in tree POPUPS */
#define UMLAUTPOP1        75 /* BOX in tree POPUPS */
#define UPFROMST          76 /* STRING in tree POPUPS */

#define STRINGS           28 /* form/dialog */
#define MRECSTR            1 /* STRING in tree STRINGS */
#define FILESTR            2 /* STRING in tree STRINGS */
#define PAGESTR            3 /* STRING in tree STRINGS */
#define DATESTR            4 /* STRING in tree STRINGS */
#define NODEVSTR           5 /* STRING in tree STRINGS */
#define MSTOPSTR           6 /* STRING in tree STRINGS */
#define NAMENLOS           7 /* STRING in tree STRINGS */
#define SZEILESTR          8 /* STRING in tree STRINGS */
#define COPYSTR            9 /* STRING in tree STRINGS */
#define KURZELSTR         10 /* STRING in tree STRINGS */
#define SSPALTSTR         11 /* STRING in tree STRINGS */
#define LCOPYSTR          12 /* STRING in tree STRINGS */
#define FREESTR           13 /* STRING in tree STRINGS */
#define ZEICHSTR1         14 /* STRING in tree STRINGS */
#define DEFPRJSTR         15 /* STRING in tree STRINGS */
#define BINSTR            16 /* STRING in tree STRINGS */
#define ZEICHSTR2         17 /* STRING in tree STRINGS */
#define SELWFONTSTR       18 /* STRING in tree STRINGS */
#define LOADFILESTR       19 /* STRING in tree STRINGS */
#define HYPMARKESTR       20 /* STRING in tree STRINGS */
#define SELPFONTSTR       21 /* STRING in tree STRINGS */
#define LOADBINSTR        22 /* STRING in tree STRINGS */
#define HYPPRJSTR         23 /* STRING in tree STRINGS */
#define MERGESTR          24 /* STRING in tree STRINGS */
#define HYPANORDSTR       25 /* STRING in tree STRINGS */
#define LOADSTR           26 /* STRING in tree STRINGS */
#define INSNAMESTR        27 /* STRING in tree STRINGS */
#define CHAGESTR          28 /* STRING in tree STRINGS */
#define SAVESTR           29 /* STRING in tree STRINGS */
#define DEFPRJ2STR        30 /* STRING in tree STRINGS */
#define PRINTSTR          31 /* STRING in tree STRINGS */
#define FINDHPSTR         32 /* STRING in tree STRINGS */
#define UMLAUTSTR         33 /* STRING in tree STRINGS */
#define FINDMKSTR         34 /* STRING in tree STRINGS */
#define SEARCHGDOS        35 /* STRING in tree STRINGS */
#define TREIBERSTR        36 /* STRING in tree STRINGS */
#define ERRORSTR          37 /* STRING in tree STRINGS */
#define FINDKURZELSTR     38 /* STRING in tree STRINGS */
#define REPLACESTR        39 /* STRING in tree STRINGS */
#define FINDGDSTR         40 /* STRING in tree STRINGS */
#define UMBRUCHSTR        41 /* STRING in tree STRINGS */
#define SAVEBLKSTR        42 /* STRING in tree STRINGS */
#define SKRITSTR          43 /* STRING in tree STRINGS */
#define SAVEASSTR         44 /* STRING in tree STRINGS */
#define NULLBYTESTR       45 /* STRING in tree STRINGS */
#define SAVEPRJSTR        46 /* STRING in tree STRINGS */
#define BHEADSTR          47 /* STRING in tree STRINGS */
#define SELPATHSTR        48 /* STRING in tree STRINGS */
#define HEADSTR           49 /* STRING in tree STRINGS */
#define FFSTR1            50 /* STRING in tree STRINGS */
#define FFSTR2            51 /* STRING in tree STRINGS */

#define WINICON           29 /* form/dialog */
#define WIBOX              0 /* BOX in tree WINICON */
#define WIBILD             4 /* IMAGE in tree WINICON */

#define PRN_ICON          30 /* form/dialog */

#define SYNTAXOP          31 /* form/dialog */
#define SYOTXT             2 /* BOXTEXT in tree SYNTAXOP */ /* max len 40 */
#define SYOACTIVE          3 /* BUTTON in tree SYNTAXOP */
#define SYORULEFRAME       4 /* BUTTON in tree SYNTAXOP */
#define SYORULE            5 /* BOXTEXT in tree SYNTAXOP */ /* max len 40 */
#define SYOBOLD            6 /* BUTTON in tree SYNTAXOP */
#define SYOCOLORTXT        7 /* STRING in tree SYNTAXOP */
#define SYOCOLOR           8 /* BOX in tree SYNTAXOP */
#define SYOITALIC          9 /* BUTTON in tree SYNTAXOP */
#define SYOSELCOLORTXT    10 /* STRING in tree SYNTAXOP */
#define SYOSELCOLOR       11 /* BOX in tree SYNTAXOP */
#define SYOLIGHT          12 /* BUTTON in tree SYNTAXOP */
#define SYOSETSAMENAME    13 /* BUTTON in tree SYNTAXOP */
#define SYOHELP           14 /* BOXTEXT in tree SYNTAXOP */ /* max len 5 */
#define SYOOK             15 /* BUTTON in tree SYNTAXOP */
#define SYOCANCEL         16 /* BUTTON in tree SYNTAXOP */

#define NOWINDOW           0 /* Alert string */
/* [3][Es stehen keine weiteren|Fenster zur VerfÅgung!][OK] */

#define NOMEMORY           1 /* Alert string */
/* [3][Nicht genÅgend Speicher|vorhanden, um Funktion|auszufÅhren!][OK] */

#define PRNOTRDY           2 /* Alert string */
/* [3][Der Drucker ist|nicht bereit!][Nochmal|Abbruch] */

#define NOSCRAP            3 /* Alert string */
/* [1][Klemmbrett-Ordner konnte|nicht angelegt werden!][OK] */

#define READERR            4 /* Alert string */
/* [1][Fehler beim Lesen der Datei:|%s][OK] */

#define WRITEERR           5 /* Alert string */
/* [1][Fehler beim Schreiben|der Datei!][OK] */

#define EXIST              6 /* Alert string */
/* [2][Die Datei existiert schon! |Soll sie Åberschrieben |werden?][OK|Abbruch] */

#define GETNAME            7 /* Alert string */
/* [2][Soll der neue Name|Åbernommen werden?][Ja|Nein] */

#define NOTEXT             8 /* Alert string */
/* [3][Es kann kein Text mehr|geladen werden!][OK] */

#define ABANDON            9 /* Alert string */
/* [2][Sollen die énderungen in|%s|verworfen werden?][OK|Abbruch] */

#define NOTEXIST          10 /* Alert string */
/* [1][Die Datei|%s|wurde nicht gefunden! ][OK] */

#define TOOLONG           11 /* Alert string */
/* [0][Die Zeile wird zu lang!|Maximal %d Zeichen.][OK] */

#define FATALERR          12 /* Alert string */
/* [3][Interner Fehler Nr: %d|=====================|Bitte Programmautor|benachrichtigen!][OK] */

#define WRONGINF          13 /* Alert string */
/* [0][Fehler in der Parameterdatei!|UngÅltige Variable:|%s][Weiter] */

#define MKNAME            14 /* Alert string */
/* [1][Das Makro braucht|einen Namen.][OK] */

#define BREAK             15 /* Alert string */
/* [2][Soll die Operation|abgebrochen werden?][Ja|Nein] */

#define NOSPACE           16 /* Alert string */
/* [1][Laufwerk ist voll!|Datei %s|wurde gelîscht!][OK] */

#define MOVED             17 /* Alert string */
/* [2][Die Datei|%s|wurde geÑndert.][Sichern|Verwerfen|Abbruch] */

#define MOVED2            18 /* Alert string */
/* [2][Die Datei|%s|wurde durch ein anderes|Programm verÑndert.][öberschreiben|Abbruch] */

#define MOVED3            19 /* Alert string */
/* [2][Die Datei|%s|wurde durch ein anderes|Programm verÑndert.][Laden|Abbruch] */

#define NEWTEXT           20 /* Alert string */
/* [2][Die Datei|%s|existiert nicht.|Anlegen?][Ja|Nein] */

#define READONLY          21 /* Alert string */
/* [1][Die Datei|%s|ist schreibgeschÅtzt!][OK] */

#define NOFSL             22 /* Alert string */
/* [1][Keine Fontauswahl (xFSL,|MagiC) installiert!][OK] */

#define SENOANS           23 /* Alert string */
/* [2][Shell antwortet nicht!|Kommunikation beenden?][Ja|Nein] */

#define SENOTFOUND        24 /* Alert string */
/* [0][Keine Shell gefunden!][OK] */

#define SEONLINE          25 /* Alert string */
/* [0][Es besteht schon eine|SE-Verbindung!][Trotzdem|Abbruch] */

#define NOKONV            26 /* Alert string */
/* [1][Eine Konvertierung|wÅrde nichts Ñndern!][OK] */

#define STGNOTF           27 /* Alert string */
/* [3][ST-Guide oder HypView|ist nicht installiert!][Abbruch] */

#define NOSTG             28 /* Alert string */
/* [3][Die Online-Hilfe geht nur wenn |ST-Guide oder HypView als|Hilfe-Programm angemeldet ist!][Abbruch] */

#define NOHELP            29 /* Alert string */
/* [3][Kein Hilfeprogramm angemeldet!][Abbruch] */

#define HELPNOTF          30 /* Alert string */
/* [3][Das Hilfeprogramm|%s|ist nicht installiert!][Abbruch] */

#define HELPSTART         31 /* Alert string */
/* [3][Das Hilfeprogramm kann nur|unter N.AES oder MagiC|nachgestartet werden!][Abbruch] */

#define ASAVEASK          32 /* Alert string */
/* [2][Autosave!|Soll die Datei|%s|gesichert werden?][Aus|Ja|Nein] */

#define RPSAME            33 /* Alert string */
/* [0][Such- und Ersetztext|sind gleich!][OK] */

#define FATALMEM          34 /* Alert string */
/* [3][Fehler in der Speicher-|verwaltung.|Absturz mîglich!][Oh ha] */

#define SORTERR           35 /* Alert string */
/* [0][Zum Sortieren mÅssen|mindestens zwei Zeilen|markiert sein.][OK] */

#define KLAMMERERR        36 /* Alert string */
/* [3][Klammerpaare nicht voll-|stÑndig definiert!][Abbruch] */

#define WFOWNERR          37 /* Alert string */
/* [3][Ziel-Applikation kann nicht|ermittelt werden.|D&D daher nicht mîglich!][Abbruch] */

#define SYN_TOFROM        38 /* Alert string */
/* [1][Fehler in Syntaxdatei:|%s|"To" oder "While"|vor "From"|in Zeilennummer %i][OK] */

#define SYN_MIXED         39 /* Alert string */
/* [1][Fehler in Syntaxdatei:|%s|"From", "To" oder "While"|gemischt mit "Keyword"|in Zeilennummer %i][OK] */

#define SYN_SYNTAX        40 /* Alert string */
/* [1][Fehler in Syntaxdatei:|%s|Syntaxfehler in Zeile %i][OK] */

#define SYN_WRONGVAL      41 /* Alert string */
/* [1][Fehler in Syntaxdatei:|%s|Falscher numerischer Wert|in Zeile %i][OK] */

#define SYN_MEM           42 /* Alert string */
/* [1][Speichermangel!|Syntax-Highlighting wird ab-|geschaltet. Sichern Sie Ihre|Texte und starten Sie das|System neu!][OK] */

#define SYN_DUPTEXT       43 /* Alert string */
/* [1][Fehler in Syntaxdatei:|%s|Bereits vergebene Text-|definition in Zeile %i][OK] */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD qed_rsc_load(_WORD wchar, _WORD hchar);
extern _WORD qed_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD qed_rsc_free(void);
#endif
