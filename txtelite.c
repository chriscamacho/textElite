/* txtelite.c  1.5 */
/* Textual version of Elite trading (C implementation) */
/* Converted by Ian Bell from 6502 Elite sources.
   Original 6502 Elite by Ian Bell & David Braben. */


/* ----------------------------------------------------------------------
  The nature of basic mechanisms used to generate the Elite socio-economic
universe are now widely known. A competant games programmer should be able to
produce equivalent functionality. A competant hacker should be able to lift
the exact system from the object code base of official conversions.

  This file may be regarded as defining the Classic Elite universe.

  It contains a C implementation of the precise 6502 algorithms used in the
 original BBC Micro version of Acornsoft Elite (apart from Galctic Hyperspace
 target systems) together with a parsed textual command testbed.

  Note that this is not the universe of David Braben's 'Frontier' series.


ICGB 13/10/99	; 21/07/15
ian@ianbell.me
www.ianbellelite.com
  ---------------------------------------------------------------------- */



/* Note that this program is "quick-hack" text parser-driven version
of Elite with no combat or missions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//#include <conio.h>
//#include <graph.h>
#include <math.h>
#ifdef _WIN32
# ifndef __clang__
#  include <malloc.h>
# endif
#endif

#include <ctype.h>	// toupper / tolower

#define true (-1)
#define false (0)
#define tonnes (0)

#define maxlen (20) /* Length of strings */

typedef int boolean;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef signed short int16;
typedef signed long int32;

typedef uint16 u_int16;

typedef int planetnum;


typedef struct
{   uint8 a,b,c,d;
} fastseedtype;  /* four byte random number used for planet description */


typedef struct
{   uint16 w0;
    uint16 w1;
    uint16 w2;
} seedtype;  /* six byte random number used as seed for planets */

typedef struct
{   u_int16 x;
    u_int16 y;       /* One byte unsigned */
    u_int16 economy; /* These two are actually only 0-7  */
    u_int16 govtype;
    u_int16 techlev; /* 0-16 i think */
    u_int16 population;   /* One byte */
    u_int16 productivity; /* Two byte */
    u_int16 radius; /* Two byte (not used by game at all) */
    fastseedtype	goatsoupseed;
    char name[12];
} plansys ;

#define galsize (256)
#define AlienItems (16)
#define lasttrade AlienItems

#define numforLave 7       /* Lave is 7th generated planet in galaxy one */
#define numforZaonce 129
#define numforDiso 147
#define numforRied 46

plansys galaxy[galsize]; /* Need 0 to galsize-1 inclusive */

seedtype seed;

fastseedtype rnd_seed;

boolean nativerand;

typedef struct
{   /* In 6502 version these were: */
    u_int16 baseprice;        /* one byte */
    int16 gradient;   /* five bits plus sign */
    u_int16 basequant;        /* one byte */
    u_int16 maskbyte;         /* one byte */
    u_int16 units;            /* two bits */
    char   name[20];         /* longest="Radioactives" */
} tradegood ;


typedef struct
{   u_int16 quantity[lasttrade+1];
    u_int16 price[lasttrade+1];
} markettype ;

/* Player workspace */
u_int16     shipshold[lasttrade+1];  /* Contents of cargo bay */
planetnum  currentplanet;           /* Current planet */
u_int16     galaxynum;               /* Galaxy number (1-8) */
int32    cash;
u_int16     fuel;
markettype localmarket;
u_int16     holdspace;

int fuelcost =2; /* 0.2 CR/Light year */
int maxfuel =70; /* 7.0 LY tank */

const uint16 base0=0x5A4A;
const uint16 base1=0x0248;
const uint16 base2=0xB753;  /* Base seed for galaxy 1 */


//static const char *digrams=
//							 "ABOUSEITILETSTONLONUTHNO"
//							 "ALLEXEGEZACEBISO"
//							 "USESARMAINDIREA?"
//							 "ERATENBERALAVETI"
//							 "EDORQUANTEISRION";


#if 0 // 1.4-
char pairs0[]="ABOUSEITILETSTONLONUTHNO";
/* must continue into .. */
char pairs[] = "..LEXEGEZACEBISO"
               "USESARMAINDIREA."
               "ERATENBERALAVETI"
               "EDORQUANTEISRION"; /* Dots should be nullprint characters */


#else // 1.5 planet names fix
char pairs0[]=
    "ABOUSEITILETSTONLONUTHNOALLEXEGEZACEBISOUSESARMAINDIREA.ERATENBERALAVETIEDORQUANTEISRION";

char pairs[] = "..LEXEGEZACEBISO"
               "USESARMAINDIREA."
               "ERATENBERALAVETI"
               "EDORQUANTEISRION"; /* Dots should be nullprint characters */

#endif






char govnames[][maxlen]= {"Anarchy","Feudal","Multi-gov","Dictatorship",
                          "Communist","Confederacy","Democracy","Corporate State"
                         };

char econnames[][maxlen]= {"Rich Ind","Average Ind","Poor Ind","Mainly Ind",
                           "Mainly Agri","Rich Agri","Average Agri","Poor Agri"
                          };


char unitnames[][5] = {"t","kg","g"};

/* Data for DB's price/availability generation system */
/*                   Base  Grad Base Mask Un   Name
                     price ient quant     it              */

#define POLITICALLY_CORRECT	0
/* Set to 1 for NES-sanitised trade goods */

tradegood commodities[]=
{
    {0x13,-0x02,0x06,0x01,0,"Food        "},
    {0x14,-0x01,0x0A,0x03,0,"Textiles    "},
    {0x41,-0x03,0x02,0x07,0,"Radioactives"},
#if POLITICALLY_CORRECT
    {0x28,-0x05,0xE2,0x1F,0,"Robot Slaves"},
    {0x53,-0x05,0xFB,0x0F,0,"Beverages   "},
#else
    {0x28,-0x05,0xE2,0x1F,0,"Slaves      "},
    {0x53,-0x05,0xFB,0x0F,0,"Liquor/Wines"},
#endif
    {0xC4,+0x08,0x36,0x03,0,"Luxuries    "},
#if POLITICALLY_CORRECT
    {0xEB,+0x1D,0x08,0x78,0,"Rare Species"},
#else
    {0xEB,+0x1D,0x08,0x78,0,"Narcotics   "},
#endif
    {0x9A,+0x0E,0x38,0x03,0,"Computers   "},
    {0x75,+0x06,0x28,0x07,0,"Machinery   "},
    {0x4E,+0x01,0x11,0x1F,0,"Alloys      "},
    {0x7C,+0x0d,0x1D,0x07,0,"Firearms    "},
    {0xB0,-0x09,0xDC,0x3F,0,"Furs        "},
    {0x20,-0x01,0x35,0x03,0,"Minerals    "},
    {0x61,-0x01,0x42,0x07,1,"Gold        "},
    {0xAB,-0x02,0x37,0x1F,1,"Platinum    "},
    {0x2D,-0x01,0xFA,0x0F,2,"Gem-Strones "},
    {0x35,+0x0F,0xC0,0x07,0,"Alien Items "},
};

/**-Required data for text interface **/
char tradnames[lasttrade][maxlen]; /* Tradegood names used in text commands
                                      Set using commodities array */


void goat_soup(const char *source,plansys * psy);


#define nocomms (14)

boolean dobuy(char *);
boolean dosell(char *);
boolean dofuel(char *);
boolean dojump(char *);
boolean docash(char *);
boolean domkt(char *);
boolean dohelp(char *);
boolean dohold(char *);
boolean dosneak(char *);
boolean dolocal(char *);
boolean doinfo(char *);
boolean dogalhyp(char *);
boolean doquit(char *);
boolean dotweakrand(char *);

char commands[nocomms][maxlen]=
{   "buy",        "sell",     "fuel",     "jump",
    "cash",       "mkt",      "help",     "hold",
    "sneak",      "local",    "info",     "galhyp",
    "quit",       "rand"
};

boolean (*comfuncs[nocomms])(char *)=
{   dobuy,         dosell,       dofuel,    dojump,
    docash,        domkt,        dohelp,    dohold,
    dosneak,       dolocal,      doinfo,    dogalhyp,
    doquit,				 dotweakrand
};

/**- General functions **/


void port_srand(unsigned int);
int port_rand(void);

static unsigned int lastrand = 0;

void mysrand(unsigned int seed)
{   srand(seed);
    lastrand = seed - 1;
}

int myrand(void)
{   int r;
    if(nativerand) r=rand();
    else
    {   // As supplied by D McDonnell	from SAS Insititute C
        r = (((((((((((lastrand << 3) - lastrand) << 3)
                    + lastrand) << 1) + lastrand) << 4)
                - lastrand) << 1) - lastrand) + 0xe60)
            & 0x7fffffff;
        lastrand = r - 1;
    }
    return(r);
}

char randbyte(void)	{
    return (char)(myrand()&0xFF);
}

u_int16 mymin(u_int16 a,u_int16 b) {
    if(a<b) return(a);
    else return(b);
}

void stop(char * string)
{   printf("\n%s",string);
    exit(1);
}

/**+  ftoi **/
signed int ftoi(double value)
{   return ((signed int)floor(value+0.5));
}

/**+  ftoi2 **/
signed int ftoi2(double value)
{   return ((signed int)floor(value));
}

void tweakseed(seedtype *s)
{   uint16 temp;
    temp = ((*s).w0)+((*s).w1)+((*s).w2); /* 2 byte aritmetic */
    (*s).w0 = (*s).w1;
    (*s).w1 = (*s).w2;
    (*s).w2 = temp;
}

/**-String functions for text interface **/

void stripout(char *s,const char c) /* Remove all c's from string s */
{   size_t i=0,j=0;
    while(i<strlen(s))
    {   if(s[i]!=c) {
            s[j]=s[i];
            j++;
        }
        i++;
    }
    s[j]=0;
}

/*
int toupper(char c)
{	if((c>='a')&&(c<='z')) return(c+'A'-'a');
	return((int)c);
}

int tolower(char c)
{	if((c>='A')&&(c<='Z')) return(c+'a'-'A');
	return((int)c);
}
*/

int stringbeg(char *s,char *t)
/* Return nonzero iff string t begins with non-empty string s */
{   size_t i=0;
    size_t l=strlen(s);
    if(l>0)
    {   while((i<l)&(toupper(s[i])==toupper(t[i])))	i++;
        if(i==l) return true;
    }
    return false;
}

u_int16 stringmatch(char *s,char a[][20],u_int16 n)
/* Check string s against n options in string array a
   If matches ith element return i+1 else return 0 */
{   u_int16 i=0;
    while(i<n)
    {   if(stringbeg(s,a[i])) return i+1;
        i++;
    }
    return 0;
}

void spacesplit(char *s,char *t)
/* Split string s at first space, returning first 'word' in t & shortening s
*/
{   size_t i=0,j=0;
    size_t l=strlen(s);
    while((i<l)&(s[i]==' ')) i++;; /* Strip leading spaces */
    if(i==l) {
        s[0]=0;
        t[0]=0;
        return;
    };
    while((i<l)&(s[i]!=' ')) t[j++]=s[i++];
    t[j]=0;
    i++;
    j=0;
    while(i<l) s[j++]=s[i++];
    s[j]=0;
}

/**-Functions for stock market **/

u_int16 gamebuy(u_int16 i, u_int16 a)
/* Try to buy ammount a  of good i  Return ammount bought */
/* Cannot buy more than is availble, can afford, or will fit in hold */
{   u_int16 t;
    if(cash<0) t=0;
    else
    {   t=mymin(localmarket.quantity[i],a);
        if ((commodities[i].units)==tonnes) {
            t = mymin(holdspace,t);
        }
        t = mymin(t, (u_int16)floor((double)cash/(localmarket.price[i])));
    }
    shipshold[i]+=t;
    localmarket.quantity[i]-=t;
    cash-=t*(localmarket.price[i]);
    if ((commodities[i].units)==tonnes) {
        holdspace-=t;
    }
    return t;
}

u_int16 gamesell(u_int16 i,u_int16 a) /* As gamebuy but selling */
{   u_int16 t=mymin(shipshold[i],a);
    shipshold[i]-=t;
    localmarket.quantity[i]+=t;
    if ((commodities[i].units)==tonnes) {
        holdspace+=t;
    }
    cash+=t*(localmarket.price[i]);
    return t;
}

markettype genmarket(u_int16 fluct, plansys p)
/* Prices and availabilities are influenced by the planet's economy type
   (0-7) and a random "fluctuation" byte that was kept within the saved
   commander position to keep the market prices constant over gamesaves.
   Availabilities must be saved with the game since the player alters them
   by buying (and selling(?))

   Almost all operations are one byte only and overflow "errors" are
   extremely frequent and exploited.

   Trade Item prices are held internally in a single byte=true value/4.
   The decimal point in prices is introduced only when printing them.
   Internally, all prices are integers.
   The player's cash is held in four bytes.
 */

{   markettype market;
    unsigned short i;
    for(i=0; i<=lasttrade; i++)
    {   signed int q;
        signed int product = (p.economy)*(commodities[i].gradient);
        signed int changing = fluct & (commodities[i].maskbyte);
        q =  (commodities[i].basequant) + changing - product;
        q = q&0xFF;
        if(q&0x80) {
            q=0;
        };                       /* Clip to positive 8-bit */

        market.quantity[i] = (uint16)(q & 0x3F); /* Mask to 6 bits */

        q =  (commodities[i].baseprice) + changing + product;
        q = q & 0xFF;
        market.price[i] = (uint16) (q*4);
    }
    market.quantity[AlienItems] = 0; /* Override to force nonavailability */
    return market;
}

void displaymarket(markettype m)
{   unsigned short i;
    for(i=0; i<=lasttrade; i++)
    {   printf("\n");
        printf("%s", commodities[i].name);
        printf("   %.1f",((float)(m.price[i])/10));
        printf("   %u",m.quantity[i]);
        printf("%s ", unitnames[commodities[i].units]);
        printf("   %u",shipshold[i]);
    }
}

/**-Generate system info from seed **/

plansys makesystem(seedtype *s)
{   plansys thissys;
    u_int16 pair1,pair2,pair3,pair4;
    uint16 longnameflag=((*s).w0)&64;

    thissys.x=(((*s).w1)>>8);
    thissys.y=(((*s).w0)>>8);

    thissys.govtype =((((*s).w1)>>3)&7); /* bits 3,4 &5 of w1 */

    thissys.economy =((((*s).w0)>>8)&7); /* bits 8,9 &A of w0 */
    if (thissys.govtype <=1)
    {   thissys.economy = ((thissys.economy)|2);
    }

    thissys.techlev =((((*s).w1)>>8)&3)+((thissys.economy)^7);
    thissys.techlev +=((thissys.govtype)>>1);
    if (((thissys.govtype)&1)==1)	thissys.techlev+=1;
    /* C simulation of 6502's LSR then ADC */

    thissys.population = 4*(thissys.techlev) + (thissys.economy);
    thissys.population +=  (thissys.govtype) + 1;

    thissys.productivity = (((thissys.economy)^7)+3)*((thissys.govtype)+4);
    thissys.productivity *= (thissys.population)*8;

    thissys.radius = 256*(((((*s).w2)>>8)&15)+11) + thissys.x;

    thissys.goatsoupseed.a = (*s).w1 & 0xFF;;
    thissys.goatsoupseed.b = (*s).w1 >>8;
    thissys.goatsoupseed.c = (*s).w2 & 0xFF;
    thissys.goatsoupseed.d = (*s).w2 >> 8;

    pair1=2*((((*s).w2)>>8)&31);
    tweakseed(s);
    pair2=2*((((*s).w2)>>8)&31);
    tweakseed(s);
    pair3=2*((((*s).w2)>>8)&31);
    tweakseed(s);
    pair4=2*((((*s).w2)>>8)&31);
    tweakseed(s);
    /* Always four iterations of random number */

    (thissys.name)[0]=pairs[pair1];
    (thissys.name)[1]=pairs[pair1+1];
    (thissys.name)[2]=pairs[pair2];
    (thissys.name)[3]=pairs[pair2+1];
    (thissys.name)[4]=pairs[pair3];
    (thissys.name)[5]=pairs[pair3+1];

    if(longnameflag) /* bit 6 of ORIGINAL w0 flags a four-pair name */
    {
        (thissys.name)[6]=pairs[pair4];
        (thissys.name)[7]=pairs[pair4+1];
        (thissys.name)[8]=0;
    }
    else (thissys.name)[6]=0;
    stripout(thissys.name,'.');



    return thissys;
}


/**+Generate galaxy **/


/* Functions for galactic hyperspace */

uint16 rotatel(uint16 x) /* rotate 8 bit number leftwards */
/* (tried to use chars but too much effort persuading this braindead
   language to do bit operations on bytes!) */
{   uint16 temp = x&128;
    return (2*(x&127))+(temp>>7);
}

uint16 twist(uint16 x)
{   return (uint16)((256*rotatel(x>>8))+rotatel(x&255));
}

void nextgalaxy(seedtype *s) /* Apply to base seed; once for galaxy 2  */
{   (*s).w0 = twist((*s).w0);  /* twice for galaxy 3, etc. */
    (*s).w1 = twist((*s).w1);  /* Eighth application gives galaxy 1 again*/
    (*s).w2 = twist((*s).w2);
}

/* Original game generated from scratch each time info needed */
void buildgalaxy(u_int16 galaxynum)
{   u_int16 syscount,galcount;
    seed.w0=base0;
    seed.w1=base1;
    seed.w2=base2; /* Initialise seed for galaxy 1 */
    for(galcount=1; galcount<galaxynum; ++galcount) nextgalaxy(&seed);
    /* Put galaxy data into array of structures */
    for(syscount=0; syscount<galsize; ++syscount) galaxy[syscount]=makesystem(&seed);
}

/**-Functions for navigation **/

void gamejump(planetnum i) /* Move to system i */
{   currentplanet=i;
    localmarket = genmarket(randbyte(),galaxy[i]);
}

u_int16 distance(plansys a,plansys b)
/* Seperation between two planets (4*sqrt(X*X+Y*Y/4)) */
{   return (u_int16)ftoi(4*sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)/4));
}


planetnum matchsys(char *s)
/* Return id of the planet whose name matches passed strinmg
   closest to currentplanet - if none return currentplanet */
{   planetnum syscount;
    planetnum p=currentplanet;
    u_int16 d=9999;
    for(syscount=0; syscount<galsize; ++syscount)
    {   if (stringbeg(s,galaxy[syscount].name))
        {   if (distance(galaxy[syscount],galaxy[currentplanet])<d)
            {   d=distance(galaxy[syscount],galaxy[currentplanet]);
                p=syscount;
            }
        }
    }
    return p;
}


/**-Print data for given system **/
void prisys(plansys plsy,boolean compressed)
{   if (compressed)
    {
        //	  printf("\n ");
        printf("%10s",plsy.name);
        printf(" TL: %2i ",(plsy.techlev)+1);
        printf("%12s",econnames[plsy.economy]);
        printf(" %15s",govnames[plsy.govtype]);
    }
    else
    {   printf("\n\nSystem:  ");
        printf("%s", plsy.name);
        printf("\nPosition (%i,",plsy.x);
        printf("%i)",plsy.y);
        printf("\nEconomy: (%i) ",plsy.economy);
        printf("%s", econnames[plsy.economy]);
        printf("\nGovernment: (%i) ",plsy.govtype);
        printf("%s", govnames[plsy.govtype]);
        printf("\nTech Level: %2i",(plsy.techlev)+1);
        printf("\nTurnover: %u",(plsy.productivity));
        printf("\nRadius: %u",plsy.radius);
        printf("\nPopulation: %u Billion",(plsy.population)>>3);

        rnd_seed = plsy.goatsoupseed;
        printf("\n");
        goat_soup("\x8F is \x97.",&plsy);
    }
}

/**-Various command functions **/

boolean dotweakrand(char * s)
{   //(void *)&s;
    nativerand ^=1;
    return true;
}

boolean dolocal(char *s)
{   planetnum syscount;
    u_int16 d;
    //atoi(s);
    printf("Galaxy number %i",galaxynum);
    for(syscount=0; syscount<galsize; ++syscount)
    {   d=distance(galaxy[syscount],galaxy[currentplanet]);
        if(d<=maxfuel)
        {   if(d<=fuel)	printf("\n * ");
            else printf("\n - ");
            prisys(galaxy[syscount],true);
            printf(" (%.1f LY)",(float)d/10);
        }
    }
    return true;
}


boolean dojump(char *s) /* Jump to planet name s */
{   u_int16 d;
    planetnum dest=matchsys(s);
    if(dest==currentplanet) {
        printf("\nBad jump");
        return false;
    }
    d=distance(galaxy[dest],galaxy[currentplanet]);
    if (d>fuel) {
        printf("\nJump to far");
        return false;
    }
    fuel-=d;
    gamejump(dest);
    prisys(galaxy[currentplanet],false);
    return true;
}

boolean dosneak(char *s) /* As dojump but no fuel cost */
{   u_int16 fuelkeep=fuel;
    boolean b;
    fuel=666;
    b=dojump(s);
    fuel=fuelkeep;
    return b;
}


boolean dogalhyp(char *s) /* Jump to next galaxy */
/* Preserve planetnum (eg. if leave 7th planet
   arrive at 7th planet)
   Classic Elite always jumped to planet nearest (0x60,0x60)
   */
{   (void)(&s);     /* Discard s */
    galaxynum++;
    if(galaxynum==9) {
        galaxynum=1;
    }
    buildgalaxy(galaxynum);
    return true;
}

boolean doinfo(char *s) /* Info on planet */
{   planetnum dest=matchsys(s);
    prisys(galaxy[dest],false);
    return true;
}


boolean dohold(char *s)
{   u_int16 a=(u_int16)atoi(s),t=0,i;
    for(i=0; i<=lasttrade; ++i)
    {   if ((commodities[i].units)==tonnes) t+=shipshold[i];
    }
    if(t>a) {
        printf("\nHold too full");
        return false;
    }
    holdspace=a-t;
    return true;
}

boolean dosell(char *s) /* Sell ammount S(2) of good S(1) */
{   u_int16 i,a,t;
    char s2[maxlen];
    spacesplit(s,s2);
    a=(u_int16)atoi(s);
    if (a==0) {
        a=1;
    }
    i=stringmatch(s2,tradnames,lasttrade+1);
    if(i==0) {
        printf("\nUnknown trade good");
        return false;
    }
    i-=1;

    t=gamesell(i,a);

    if(t==0) {
        printf("Cannot sell any ");
    }
    else
    {   printf("\nSelling %i",t);
        printf("%s", unitnames[commodities[i].units]);
        printf(" of ");
    }
    printf("%s", tradnames[i]);

    return true;

}


boolean dobuy(char *s) /* Buy ammount S(2) of good S(1) */
{   u_int16 i,a,t;
    char s2[maxlen];
    spacesplit(s,s2);
    a=(u_int16)atoi(s);
    if (a==0) a=1;
    i=stringmatch(s2,tradnames,lasttrade+1);
    if(i==0) {
        printf("\nUnknown trade good");
        return false;
    }
    i-=1;

    t=gamebuy(i,a);
    if(t==0) printf("Cannot buy any ");
    else
    {   printf("\nBuying %i",t);
        printf("%s", unitnames[commodities[i].units]);
        printf(" of ");
    }
    printf("%s", tradnames[i]);
    return true;
}

u_int16 gamefuel(u_int16 f) /* Attempt to buy f tonnes of fuel */
{   if(f+fuel>maxfuel)  f=maxfuel-fuel;
    if(fuelcost>0)
    {   if((int)f*fuelcost>cash)  f=(u_int16)(cash/fuelcost);
    }
    fuel+=f;
    cash-=fuelcost*f;
    return f;
}


boolean dofuel(char *s)
/* Buy ammount S of fuel */
{   u_int16 f=gamefuel((u_int16)floor(10*atof(s)));
    if(f==0) {
        printf("\nCan't buy any fuel");
    }
    printf("\nBuying %.1fLY fuel",(float)f/10);
    return true;
}

boolean docash(char *s) /* Cheat alter cash by S */
{   int a=(int)(10*atof(s));
    cash+=(long)a;
    if(a!=0) return true;
    printf("Number not understood");
    return false;
}

boolean domkt(char *s) /* Show stock market */
{   //atoi(s);
    displaymarket(localmarket);
    printf("\nFuel :%.1f",(float)fuel/10);
    printf("      Holdspace :%it",holdspace);
    return true;
}

boolean parser(char *s) /* Obey command s */
{   u_int16 i;
    char c[maxlen];
    spacesplit(s,c);
    i=stringmatch(c,commands,nocomms);
    if(i)return (*comfuncs[i-1])(s) ;
    printf("\n Bad command (");
    printf("%s", c);
    printf(")");
    return false;
}


boolean doquit(char *s)
{   (void)(&s);
    exit(0);
    return(0);
}

boolean dohelp(char *s)
{
    (void)(&s);
    printf("\nCommands are:");
    printf("\nBuy   tradegood ammount");
    printf("\nSell  tradegood ammount");
    printf("\nFuel  ammount    (buy ammount LY of fuel)");
    printf("\nJump  planetname (limited by fuel)");
    printf("\nSneak planetname (any distance - no fuel cost)");
    printf("\nGalhyp           (jumps to next galaxy)");
    printf("\nInfo  planetname (prints info on system");
    printf("\nMkt              (shows market prices)");
    printf("\nLocal            (lists systems within 7 light years)");
    printf("\nCash number      (alters cash - cheating!)");
    printf("\nHold number      (change cargo bay)");
    printf("\nQuit or ^C       (exit)");
    printf("\nHelp             (display this text)");
    printf("\nRand             (toggle RNG)");
    printf("\n\nAbbreviations allowed eg. b fo 5 = Buy Food 5, m= Mkt");
    return true;
}

/**+main **/
int main()
{   u_int16 i;
    char getcommand[maxlen];
    nativerand=1;
    printf("\nWelcome to Text Elite 1.5.\n");

    for(i=0; i<=lasttrade; i++) strcpy(tradnames[i],commodities[i].name);

    mysrand(12345);/* Ensure repeatability */

    galaxynum=1;
    buildgalaxy(galaxynum);

    currentplanet=numforLave;                        /* Don't use jump */
    localmarket = genmarket(0x00,galaxy[numforLave]);/* Since want seed=0 */

    fuel=maxfuel;

#define PARSER(S) { char buf[0x10];strcpy(buf,S);parser(buf);}

    PARSER("hold 20");         /* Small cargo bay */
    PARSER("cash +100");       /* 100 CR */
    PARSER("help");

#undef PARSER

    for(;;)
    {   printf("\n\nCash :%.1f>",((float)cash)/10);
        fgets(getcommand, maxlen, stdin);
        if (getcommand[strlen(getcommand)-1] == '\n')
            getcommand[strlen(getcommand)-1] = '\0';
        //gets(getcommand);
        parser(getcommand);
    }

    /* Unreachable */


    /* 6502 Elite fires up at Lave with fluctuation=00
       and these prices tally with the NES ones.
       However, the availabilities reside in the saved game data.
       Availabilities are calculated (and fluctuation randomised)
       on hyperspacing
       I have checked with this code for Zaonce with fluctaution &AB
       against the SuperVision 6502 code and both prices and availabilities tally.
    */
    return(0);
}

/* "Goat Soup" planetary description string code - adapted from Christian Pinder's
  reverse engineered sources. */

struct desc_choice {
    const char *option[5];
};

static struct desc_choice desc_list[] =
{
    /* 81 */	{"fabled", "notable", "well known", "famous", "noted"},
    /* 82 */	{"very", "mildly", "most", "reasonably", ""},
    /* 83 */	{"ancient", "\x95", "great", "vast", "pink"},
    /* 84 */	{"\x9E \x9D plantations", "mountains", "\x9C", "\x94 forests", "oceans"},
    /* 85 */	{"shyness", "silliness", "mating traditions", "loathing of \x86", "love for \x86"},
    /* 86 */	{"food blenders", "tourists", "poetry", "discos", "\x8E"},
    /* 87 */	{"talking tree", "crab", "bat", "lobst", "\xB2"},
    /* 88 */	{"beset", "plagued", "ravaged", "cursed", "scourged"},
    /* 89 */	{"\x96 civil war", "\x9B \x98 \x99s", "a \x9B disease", "\x96 earthquakes", "\x96 solar activity"},
    /* 8A */	{"its \x83 \x84", "the \xB1 \x98 \x99","its inhabitants' \x9A \x85", "\xA1", "its \x8D \x8E"},
    /* 8B */	{"juice", "brandy", "water", "brew", "gargle blasters"},
    /* 8C */	{"\xB2", "\xB1 \x99", "\xB1 \xB2", "\xB1 \x9B", "\x9B \xB2"},
    /* 8D */	{"fabulous", "exotic", "hoopy", "unusual", "exciting"},
    /* 8E */	{"cuisine", "night life", "casinos", "sit coms", " \xA1 "},
    /* 8F */	{"\xB0", "The planet \xB0", "The world \xB0", "This planet", "This world"},
    /* 90 */	{"n unremarkable", " boring", " dull", " tedious", " revolting"},
    /* 91 */	{"planet", "world", "place", "little planet", "dump"},
    /* 92 */	{"wasp", "moth", "grub", "ant", "\xB2"},
    /* 93 */	{"poet", "arts graduate", "yak", "snail", "slug"},
    /* 94 */	{"tropical", "dense", "rain", "impenetrable", "exuberant"},
    /* 95 */	{"funny", "wierd", "unusual", "strange", "peculiar"},
    /* 96 */	{"frequent", "occasional", "unpredictable", "dreadful", "deadly"},
    /* 97 */	{"\x82 \x81 for \x8A", "\x82 \x81 for \x8A and \x8A", "\x88 by \x89", "\x82 \x81 for \x8A but \x88 by \x89","a\x90 \x91"},
    /* 98 */	{"\x9B", "mountain", "edible", "tree", "spotted"},
    /* 99 */	{"\x9F", "\xA0", "\x87oid", "\x93", "\x92"},
    /* 9A */	{"ancient", "exceptional", "eccentric", "ingrained", "\x95"},
    /* 9B */	{"killer", "deadly", "evil", "lethal", "vicious"},
    /* 9C */	{"parking meters", "dust clouds", "ice bergs", "rock formations", "volcanoes"},
    /* 9D */	{"plant", "tulip", "banana", "corn", "\xB2weed"},
    /* 9E */	{"\xB2", "\xB1 \xB2", "\xB1 \x9B", "inhabitant", "\xB1 \xB2"},
    /* 9F */	{"shrew", "beast", "bison", "snake", "wolf"},
    /* A0 */	{"leopard", "cat", "monkey", "goat", "fish"},
    /* A1 */	{"\x8C \x8B", "\xB1 \x9F \xA2","its \x8D \xA0 \xA2", "\xA3 \xA4", "\x8C \x8B"},
    /* A2 */	{"meat", "cutlet", "steak", "burgers", "soup"},
    /* A3 */	{"ice", "mud", "Zero-G", "vacuum", "\xB1 ultra"},
    /* A4 */	{"hockey", "cricket", "karate", "polo", "tennis"}
};

/* B0 = <planet name>
	 B1 = <planet name>ian
	 B2 = <random name>
*/

int gen_rnd_number (void)
{   int a,x;
    x = (rnd_seed.a * 2) & 0xFF;
    a = x + rnd_seed.c;
    if (rnd_seed.a > 127)	a++;
    rnd_seed.a = a & 0xFF;
    rnd_seed.c = x;

    a = a / 256;	/* a = any carry left from above */
    x = rnd_seed.b;
    a = (a + x + rnd_seed.d) & 0xFF;
    rnd_seed.b = a;
    rnd_seed.d = x;
    return a;
}


void goat_soup(const char *source,plansys * psy)
{   for(;;)
    {   int c=*(source++);
        c=c&0xFF; // mask to avoid breakage (neg expansion)
        if(c=='\0')	break;
        if(c<0x80) printf("%c",c);
        else
        {   if (c <=0xA4)
            {   int rnd = gen_rnd_number();
                goat_soup(desc_list[c-0x81].option[(rnd >= 0x33)+(rnd >= 0x66)+(rnd >= 0x99)+(rnd >= 0xCC)],psy);
            }
            else switch(c)
                {
                case 0xB0: /* planet name */
                {   int i=1;
                    printf("%c",psy->name[0]);
                    while(psy->name[i]!='\0') printf("%c",tolower(psy->name[i++]));
                }
                break;
                case 0xB1: /* <planet name>ian */
                {   int i=1;
                    printf("%c",psy->name[0]);
                    while(psy->name[i]!='\0')
                    {   if((psy->name[i+1]!='\0') || ((psy->name[i]!='E')	&& (psy->name[i]!='I')))
                            printf("%c",tolower(psy->name[i]));
                        i++;
                    }
                    printf("ian");
                }
                break;
                case 0xB2: /* random name */




#if 1 // 1.5
                {
                    int i;
                    int len = gen_rnd_number() & 3;
                    for (i = 0; i <= len; i++)
                    {
                        int x = gen_rnd_number() & 0x3e;
                        if (i == 0)
                        {
                            printf("%c",pairs0[x]);
                        }
                        else
                        {
                            printf("%c",tolower(pairs0[x]));
                        }

                        printf("%c",tolower(pairs0[x+1]));

                    } // endfor
                }
#else	// 1.4-



                {   int i;
                    int len = gen_rnd_number() & 3;
                    for(i=0; i<=len; i++)
                    {   int x = gen_rnd_number() & 0x3e;
                        if(pairs0[x]!='.') printf("%c",pairs0[x]);
                        if(i && (pairs0[x+1]!='.')) printf("%c",pairs0[x+1]);
                    }
                }
#endif

                break;
                default:
                    printf("<bad char in data [%X]>",c);
                    return;
                }	/* endswitch */
        }	/* endelse */
    }	/* endwhile */
}	/* endfunc */

/**+end **/

