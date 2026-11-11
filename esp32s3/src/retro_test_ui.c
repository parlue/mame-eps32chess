// Standalone ROM test frontend. No framebuffer, no search implementation.
#include "retro_test_ui.h"
#include "bauerino_retro_lcd.h"
#include "lcd_font.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { LED, BOARD, NOTATION, LOAD, OPTIONS, EDIT, PLAY, INFO };
static int page, selected_option, option_count, ply, handshake;
static bool running, owned, ready, thinking, black;
static char pending_driver[4], model[32]="Bauerino Retro", status[64]="Load: Emulation waehlen";
static char board[64], moves[128][6], last_white[6]="----", last_black[6]="----";
#ifdef RETRO_UI_EXTERNAL_TOUCH
static char (*queue)[896];
#else
static char queue[8][896];
#endif
static unsigned queue_read, queue_count;
static float rate;
typedef struct {char name[40],type[12],value[80],choices[160];int min,max;} Option;
static Option options[6];
static char edit[80];
static uint16_t *scan;
static int scan_y;
static const uint16_t cream=0xe6b3, muted=0x9c71, red=0xc188, bg=0x1082;
static const char *keys[4]={"1234567890","qwertyuiop","asdfghjkl-","zxcvbnm ./"};
extern const uint16_t piece_tiles[] asm("_binary_retro_pieces_start");

extern bool retro_touch_init(void);
extern bool retro_touch_point(int *x,int *y);
static bool touch_online, down;

static void changed(void) { bauerino_lcd_invalidate(true); }
static void message(const char *s) {snprintf(status,sizeof(status),"%s",s);changed();}
static bool push(const char *s) {
#ifdef RETRO_UI_EXTERNAL_TOUCH
    if(!queue){message("Kein PSRAM fuer UCI-Puffer");return false;}
#endif
    if(strlen(s)>=sizeof(queue[0]) || queue_count==8){message("UCI-Puffer voll");return false;}
    snprintf(queue[(queue_read+queue_count)%8],sizeof(queue[0]),"%s",s);++queue_count;return true;
}
bool retro_ui_command(char *line,unsigned capacity) {
    if(!queue_count || strlen(queue[queue_read])+1>capacity)return false;
    snprintf(line,capacity,"%s",queue[queue_read]);queue_read=(queue_read+1)%8;--queue_count;return true;
}
static void reset_board(void) {
    memcpy(board,"RNBQKBNRPPPPPPPP................................pppppppprnbqkbnr",64);
    ply=0;memcpy(last_white,"----",5);memcpy(last_black,"----",5);
}
static bool append_move(const char *m) {
    if(ply>=128 || strlen(m)<4 || m[0]<'a'||m[0]>'h'||m[2]<'a'||m[2]>'h'||m[1]<'1'||m[1]>'8'||m[3]<'1'||m[3]>'8')return false;
    int from=(m[1]-'1')*8+m[0]-'a', to=(m[3]-'1')*8+m[2]-'a';
    char piece=board[from];if(piece=='.')return false;
    if(tolower((unsigned char)piece)=='p' && (from%8!=to%8) && board[to]=='.')board[to+(piece=='P'?-8:8)]='.';
    if(tolower((unsigned char)piece)=='k' && abs(from-to)==2) {
        int rook=to>from?from+3:from-4;board[to>from?from+1:from-1]=board[rook];board[rook]='.';
    }
    board[to]=piece;board[from]='.';
    if(strlen(m)==5)board[to]=piece>='A'&&piece<='Z'?(char)toupper((unsigned char)m[4]):m[4];
    snprintf(moves[ply],6,"%.5s",m);
    snprintf(ply%2?last_black:last_white,6,"%.5s",m);++ply;changed();return true;
}
static void request_move(void) {
    if(!ready || thinking || !running){message(thinking?"Engine rechnet ...":"Engine noch nicht bereit");return;}
    if(ply>=128){message("Testpartie: 128 Halbzuege erreicht");return;}
    char position[896]="position startpos moves";
    for(int i=0;i<ply;++i){strcat(position," ");strcat(position,moves[i]);}
    if(ply && !push(position))return;
    if(push("go")){thinking=true;message("Engine rechnet ... Quit bricht ab");}
}
static void start_game(bool human_black) {
    if(!ready || thinking){message("Zuerst Engine laden / Suche beenden");return;}
    black=human_black;reset_board();
    for(int i=0;i<option_count;++i){char line[160];snprintf(line,sizeof(line),"setoption name %.39s value %.79s",options[i].name,options[i].value);if(!push(line))return;}
    push("ucinewgame");push("isready");ready=false;handshake=3;page=LED;
    message("Neues Spiel wird vorbereitet ...");
}
void retro_ui_open(const char *driver) {
    running=true;ready=false;thinking=false;handshake=0;option_count=0;
    snprintf(model,sizeof(model),"Mephisto %s",!strcmp(driver,"mm5")?"MM V":"MM IV");
    reset_board();
    if(owned){push("uci");handshake=1;message("UCI-Verbindung wird aufgebaut ...");}
    else message("USB-UCI aktiv");
}
void retro_ui_closed(void) {
    running=ready=thinking=owned=false;queue_count=0;queue_read=0;handshake=0;
    message("Emulation beendet - Load fuer Neustart");
}
bool retro_ui_driver(char name[4]) {
    if(!pending_driver[0])return false;
    memcpy(name,pending_driver,4);pending_driver[0]=0;owned=true;return true;
}
static void parse_option(const char *line) {
    const char *type=strstr(line," type ");if(!type)return;
    size_t n=(size_t)(type-(line+12));if(n>=sizeof(options[0].name))return;
    int i;for(i=0;i<option_count;++i)if(strlen(options[i].name)==n&&!strncmp(options[i].name,line+12,n))break;
    if(i==6)return;
    if(i==option_count)++option_count;
    Option *o=&options[i];memset(o,0,sizeof(*o));memcpy(o->name,line+12,n);
    sscanf(type+6,"%11s",o->type);
    const char *v=strstr(type," default ");
    if(v){v+=9;const char *end=strstr(v," min ");if(!end)end=strstr(v," var ");if(!end)end=v+strlen(v);
        size_t len=(size_t)(end-v);if(len>=sizeof(o->value))len=sizeof(o->value)-1;memcpy(o->value,v,len);}
    v=strstr(type," min ");if(v)o->min=atoi(v+5);v=strstr(type," max ");if(v)o->max=atoi(v+5);
    v=strstr(type," var ");if(v)snprintf(o->choices,sizeof(o->choices),"%s",v+5);
    // Explicit unlimited speed in this ROM test firmware; displayed/editable.
    if(!strcmp(o->name,"Speed"))snprintf(o->value,sizeof(o->value),"0");
    changed();
}
void retro_ui_reply(const char *line) {
    if(!strncmp(line,"option name ",12))parse_option(line);
    if(!strncmp(line,"bestmove ",9)) {
        thinking=false;
        if(append_move(line+9))message("ROM-Zug da - Move fuer naechsten Zug");
        else message("Kein spielbarer ROM-Zug / Partieende");
        return;
    }
    if(!owned)return;
    if(!strcmp(line,"uciok")&&handshake==1){push("isready");handshake=2;}
    else if(!strcmp(line,"readyok")){
        if(handshake==3){ready=true;handshake=0;
            if(!black)append_move("e2e4");
            request_move();
        }else if(handshake==2){ready=true;handshake=0;message("Bereit - Play startet den ROM-Test");}
    } else if(strstr(line,"error"))message(line);
}
void retro_ui_observe(const char *line) {
    if(owned)return;
    if(!strncmp(line,"position startpos moves ",24)){
        reset_board();char copy[896];snprintf(copy,sizeof(copy),"%s",line+24);
        char *save=NULL;for(char *m=strtok_r(copy," ",&save);m;m=strtok_r(NULL," ",&save))if(!append_move(m))break;
    }
}
// USB test transport uses the same bounded mailbox and emulation-task parser.
// Only take over an idle, completely handshaken UI session.
bool retro_ui_diagnostic(const char *line) {
    if(!line || !running || !ready || handshake || strlen(line)>=896 || strchr(line,'\n') || strchr(line,'\r'))return false;
    bool position=!strncmp(line,"position startpos moves ",24);
    bool control=!strcmp(line,"isready") || !strcmp(line,"stop") || !strcmp(line,"quit");
    if(!control && (thinking || !(position || !strcmp(line,"ucinewgame") || !strcmp(line,"go") || !strncmp(line,"setoption name ",15))))return false;
    if(!strcmp(line,"quit")){queue_read=queue_count=0;}
    if(!push(line))return false;
    owned=false;handshake=0;
    if(!strcmp(line,"ucinewgame"))reset_board();
    if(position)retro_ui_observe(line);
    if(!strcmp(line,"go")){thinking=true;message("Mittelspiel-Benchmark rechnet ...");}
    return true;
}
void retro_ui_rate(float percent) {rate=percent;}
void retro_ui_init(void) {
#ifdef RETRO_UI_EXTERNAL_TOUCH
    if(!queue)queue=heap_caps_calloc(8,sizeof(*queue),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#endif
    reset_board();
#ifndef RETRO_UI_EXTERNAL_TOUCH
    touch_online=retro_touch_init();
#endif
}
void retro_ui_poll(void) {
    if(!touch_online)return;
    int x,y;bool pressed=retro_touch_point(&x,&y);
    if(pressed&&!down)retro_ui_tap(x,y);
    down=pressed;
}
static bool hit(int x,int y,int left,int top,int w,int h){return x>=left&&x<left+w&&y>=top&&y<top+h;}
void retro_ui_tap(int x,int y) {
    printf("[UI TAP] %d %d page=%d\n",x,y,page);
    if(page==LOAD){
        if(y>=105&&y<295){snprintf(pending_driver,4,"%s",y<200?"mm4":"mm5");
            if(running){queue_count=0;push("quit");}
            page=LED;message("ROM wird geladen ...");return;}
    }else if(page==OPTIONS){
        if(y>=85&&y<85+option_count*48){selected_option=(y-85)/48;snprintf(edit,sizeof(edit),"%s",options[selected_option].value);page=EDIT;changed();return;}
    }else if(page==EDIT){
        if(hit(x,y,24,360,130,46)){size_t n=strlen(edit);if(n)edit[n-1]=0;changed();return;}
        if(hit(x,y,174,360,130,46)){size_t n=strlen(edit);if(n+1<sizeof(edit)){edit[n]=' ';edit[n+1]=0;}changed();return;}
        if(hit(x,y,324,360,130,46)){
            Option *o=&options[selected_option];bool valid=true;
            if(!strcmp(o->type,"spin")){char *end;long n=strtol(edit,&end,10);valid=edit[0]&&!*end&&n>=o->min&&n<=o->max;}
            if(!strcmp(o->type,"check"))valid=!strcmp(edit,"true")||!strcmp(edit,"false");
            if(!strcmp(o->name,"Level"))valid=(!strncmp(edit,"le ",3)||!strncmp(edit,"bl ",3))&&isdigit((unsigned char)edit[3])&&!edit[4];
            if(valid){snprintf(o->value,sizeof(o->value),"%s",edit);page=OPTIONS;message("Option gilt ab naechstem Play");}
            else message("Ungueltiger Wert (Level: le 1..9 / bl 0..9)");
            changed();return;
        }
        if(y>=140&&y<340){int r=(y-140)/50,c=(x-15)/45;if(c>=0&&c<10&&r<4&&c<(int)strlen(keys[r])){
            size_t n=strlen(edit);if(n+1<sizeof(edit)){edit[n]=keys[r][c];edit[n+1]=0;}changed();return;}}
    }else if(page==PLAY){
        if(hit(x,y,30,110,200,170)){start_game(false);return;}
        if(hit(x,y,250,110,200,170)){start_game(true);return;}
    }
    if(page!=LED){if(y>=425){page=LED;changed();}return;}
    if(y>=310&&y<452){int r=y>=382?1:0,c=(x-14)/114;if(x<14||c<0||c>3)return;
        int button=r*4+c;
        switch(button){case 0:page=BOARD;break;case 1:page=NOTATION;break;case 2:page=LOAD;break;
        case 3:page=OPTIONS;break;case 4:page=PLAY;break;case 5:request_move();break;
        case 6:page=INFO;break;case 7:if(running){queue_count=0;push("quit");message("Emulation wird beendet ...");}break;}
        changed();
    }
}
static void box(int x,int y,int w,int h,uint16_t color,int radius) {
    if(scan_y<y||scan_y>=y+h)return;
    int cut=0,dy=scan_y-y;
    if(radius&&(dy<radius||dy>=h-radius)){int a=dy<radius?radius-dy:radius-(h-1-dy),b=0;while((b+1)*(b+1)+a*a<=radius*radius)++b;cut=radius-b;}
    int end=x+w-cut;if(end>480)end=480;x+=cut;if(x<0)x=0;for(;x<end;++x)scan[x]=color;
}
static void label(int x,int y,const char *s,uint16_t color,int size) {
    if(scan_y<y||scan_y>=y+8*size)return;
    int bit=(scan_y-y)/size;
    for(;*s;++s,x+=6*size)for(int c=0;c<5;++c)if(font[(uint8_t)*s*5+c]&(1<<bit))box(x+c*size,y,size,8*size,color,0);
}
static void center(int y,const char *s,uint16_t color,int size){label(240-(int)strlen(s)*3*size,y,s,color,size);}
static void button(int x,int y,int w,int h,const char *s,bool accent) {
    box(x,y,w,h,0x0000,10);box(x+2,y+2,w-4,h-5,accent?0x6008:0x2945,8);
    box(x+5,y+4,w-10,2,accent?red:0x630c,0);
    label(x+(w-(int)strlen(s)*12)/2,y+(h-16)/2,s,cream,2);
}
static uint8_t mask(char c){static const uint8_t n[]={0x3f,6,0x5b,0x4f,0x66,0x6d,0x7d,7,0x7f,0x6f};
    if(c>='0'&&c<='9')return n[c-'0'];
    switch(c){case 'a':return 0x77;case 'b':return 0x7c;case 'c':return 0x58;case 'd':return 0x5e;case 'e':return 0x79;case 'f':return 0x71;case 'g':return 0x6f;case 'h':return 0x74;default:return 0x40;}}
static void led_digit(int x,int y,char c){uint8_t m=mask(c);int a[7][4]={{9,0,44,7},{54,8,7,30},{54,48,7,30},{9,79,44,7},{1,48,7,30},{1,8,7,30},{9,40,44,7}};
    for(int i=0;i<7;++i){int *r=a[i];box(x+r[0]-2,y+r[1]-2,r[2]+4,r[3]+4,m&(1<<i)?0x3800:0x1000,3);box(x+r[0],y+r[1],r[2],r[3],m&(1<<i)?0xf800:0x1800,2);}}
void retro_ui_scanline(int y,uint16_t *pixels) {
    scan=pixels;scan_y=y;
    if(page==LED){
        box(65,12,350,40,0x18c3,0);center(22,model,cream,2);
        box(76,70,340,190,0x0800,0);
        for(int i=0;i<4;++i){int x=84+i*76+(i>=2?18:0);led_digit(x,72,last_white[i]);led_digit(x,171,last_black[i]);}
        label(50,152,"W",muted,1);label(50,248,"B",muted,1);
        box(27,269,426,29,0x1082,0);center(279,status,cream,1);
        box(20,300,440,160,0x1082,0);
        const char *names[]={"Board","Notation","Load","Options","Play","Move","Info","Quit"};
        for(int i=0;i<8;++i)button(14+(i%4)*114,310+(i/4)*72,110,64,names[i],i==7);
        center(465,"MAME 0.220 - ROM Test",muted,1);return;
    }
    box(0,0,480,480,bg,0);center(24,model,cream,2);
    if(page==BOARD){
        for(int r=0;r<8;++r)for(int c=0;c<8;++c){int s=(7-r)*8+c;if(black)s=63-s;int dark=(r+c)%2;
            box(64+c*44,60+r*44,44,44,dark?0x7d19:0xef9f,0);
            const char *order="KQRBNPkqrbnp",*p=strchr(order,board[s]);
            if(board[s]!='.'&&p&&y>=60+r*44&&y<104+r*44){
                const uint16_t *tile=piece_tiles+((p-order)*2+dark)*48*48;
                int ty=(y-60-r*44)*48/44;
                for(int xx=0;xx<44;++xx)scan[64+c*44+xx]=tile[ty*48+xx*48/44];
            }}
        center(416,"Stellung aus echter UCI-Zugfolge",muted,1);
    }else if(page==NOTATION){
        int first=ply>18?ply-18:0;first-=first%2;
        for(int i=first;i<ply;i+=2){char row[48];snprintf(row,sizeof(row),"%2d.  %-6s   %s",i/2+1,moves[i],i+1<ply?moves[i+1]:"");label(88,78+(i-first)/2*34,row,cream,2);}
        if(!ply)center(180,"Noch keine Zuege",muted,2);
    }else if(page==LOAD){
        center(65,"Emulation laden",muted,2);button(30,105,420,85,"Mephisto MM IV",false);button(30,210,420,85,"Mephisto MM V",false);
        center(335,"ROM-Wechsel beendet die laufende Emulation",muted,1);
    }else if(page==OPTIONS){
        center(60,"UCI Options",cream,2);
        if(!option_count)center(180,"Zuerst eine Engine laden",muted,2);
        for(int i=0;i<option_count;++i){char row[96];snprintf(row,sizeof(row),"%.20s: %.35s",options[i].name,options[i].value);button(24,85+i*48,432,44,"",false);label(40,99+i*48,row,cream,2);}
        center(405,"Werte werden beim naechsten Play uebernommen",muted,1);
    }else if(page==EDIT){
        Option *o=&options[selected_option];center(62,o->name,cream,2);label(25,100,edit,0xffff,2);
        for(int r=0;r<4;++r)for(int c=0;c<(int)strlen(keys[r]);++c){char key[2]={keys[r][c],0};button(15+c*45,140+r*50,42,44,key,false);}
        button(24,360,130,46,"Loeschen",false);button(174,360,130,46,"Space",false);button(324,360,130,46,"OK",true);
        center(411,status,muted,1);
    }else if(page==PLAY){
        center(65,"ROM-Test starten / Neues Spiel",muted,1);
        button(30,110,200,170,"Weiss",false);button(250,110,200,170,"Schwarz",true);
        center(315,"Weiss: Testzug e2-e4, dann ROM-Antwort",cream,1);
        center(340,"Schwarz: ROM beginnt mit Weiss",cream,1);
        center(375,"Move: Engine uebernimmt die naechste Farbe",muted,1);
    }else if(page==INFO){
        char row[80];center(80,"Echte ROM-Emulation",cream,2);
        snprintf(row,sizeof(row),"Tempo: %.1f %% des Originals",(double)rate);label(35,140,row,cream,2);
        snprintf(row,sizeof(row),"PSRAM frei: %u",(unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));label(35,180,row,cream,2);
        snprintf(row,sizeof(row),"Halbzuege: %d / 128",ply);label(35,220,row,cream,2);
        label(35,270,"Kein depth/score/PV vom Plugin",muted,2);
        label(35,310,"Brett / BLE / WLAN noch separat",muted,2);
    }
    button(30,434,420,36,"Zurueck",false);
}
