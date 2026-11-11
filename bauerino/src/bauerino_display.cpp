#ifdef BAUERINO_DISPLAY
#include "bauerino_display.h"
#include "bauerino_layout.h"
#include "bauerino_bots.h"
#include "mame_frontend.h"
#include <Arduino_GFX_Library.h>
#include <touch/TouchDrvCST92xx.h>
#include <LittleFS.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include <cstring>
#include <freertos/queue.h>
using namespace bauerino;
namespace {
Arduino_ESP32QSPI bus(12,38,4,5,6,7);
Arduino_CO5300 panel(&bus,39,0,480,480,0,0,0,0);
class PsramCanvas : public Arduino_Canvas {
public:
  PsramCanvas():Arduino_Canvas(480,480,&panel) {}
  bool begin(int32_t speed=GFX_NOT_DEFINED) override {
    _framebuffer=static_cast<uint16_t*>(heap_caps_malloc(480*480*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    return _framebuffer && Arduino_Canvas::begin(speed);
  }
} frame;
TouchDrvCST92xx touch;
uint16_t *queens[6]{},*wallpaper=nullptr,*pieces=nullptr;
uint8_t *cards[5]{};
constexpr uint16_t Bg=0x0884,Ink=0xef9f,Muted=0x8cb5,Accent=0x6ff9,Panel=0x1928;
enum class Page { Splash,Home,Game,Controls,Boards,Retro,BotGroups,BotLevels,BotStyles,BotStart };
Page page=Page::Splash;
Page boardReturn=Page::Controls;
Page controlsReturn=Page::Home;
bool newBlack=false,adaptive=true;
bool retroMode=false;
bool retroLeaving=false;
bool capturePending=false;
core2::Game retroGame;
int strengthElo=1500,expanded=1;
int botGroupIndex=0,botLevelPage=0,chosenBot=2,chosenElo=700;
Page botStartReturn=Page::BotStyles;
bool online=false,touchOnline=false,dirty=true,notation=false;
int notationTop=0;
size_t notationCount=0;
struct Gesture {int sx=0,sy=0,x=0,y=0;uint32_t began=0;bool down=false;};
Gesture liveTouch;
portMUX_TYPE touchMux=portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t touchTaskHandle=nullptr;
QueueHandle_t gestures=nullptr;
int homePage=0,drag=0;
uint32_t began=0,lastFrame=0,toastUntil=0;
std::string toast,lastSignature;
void IRAM_ATTR touchInterrupt() {
  BaseType_t wake=pdFALSE;
  if(touchTaskHandle)vTaskNotifyGiveFromISR(touchTaskHandle,&wake);
  if(wake)portYIELD_FROM_ISR();
}
void touchTask(void*) {
  Gesture g;
  for(;;) {
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    int16_t xs[5]{},ys[5]{};
    bool pressed=touch.getPoint(xs,ys,5)>0;
    if(pressed) {
      int x=std::max(0,std::min(479,int(xs[0]))),y=std::max(0,std::min(479,int(ys[0])));
      if(!g.down) {g.sx=x;g.sy=y;g.began=millis();}
      g.x=x;g.y=y;g.down=true;
    } else if(g.down) {
      g.down=false;
      xQueueSend(gestures,&g,0);
    }
    portENTER_CRITICAL(&touchMux);liveTouch=g;portEXIT_CRITICAL(&touchMux);
  }
}
void* load(const char* name,size_t count) {
  auto f=LittleFS.open((std::string("/bauerino/")+name).c_str(),"r");
  if(!f || f.size()!=count) {Serial.printf("[DISPLAY] missing/invalid %s\n",name);return nullptr;}
  auto p=static_cast<uint8_t*>(heap_caps_malloc(count,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
  if(!p)return nullptr;
  bool ok=f.read(p,count)==count;f.close();
  if(!ok) {free(p);return nullptr;}return p;
}
void text(int x,int y,const std::string& s,uint16_t color=Ink,int size=2) {
  frame.setTextSize(size);frame.setTextColor(color);frame.setCursor(x,y);frame.print(s.c_str());
}
void center(int x,int y,const std::string& s,uint16_t color=Ink,int size=2) {
  text(x-int(s.size())*6*size/2,y,s,color,size);
}
void pill(int x,int y,int w,int h,const std::string& title,uint16_t color=Panel) {
  frame.fillRoundRect(x,y,w,h,12,color);center(x+w/2,y+(h-16)/2,title,color==Accent?Bg:Ink);
}
bool in(int x,int y,int a,int b,int w,int h) {return x>=a && x<a+w && y>=b && y<b+h;}
void notice(const std::string& message) {toast=message;toastUntil=millis()+1700;dirty=true;}
void pawn(int x,int y,bool black,int scale=1) {
  uint16_t c=black?0x31aa:0xef9f,edge=black?Muted:0xbdf7;
  frame.fillCircle(x+16*scale,y+8*scale,7*scale,edge);
  frame.fillCircle(x+16*scale,y+8*scale,6*scale,c);
  frame.fillTriangle(x+16*scale,y+13*scale,x+5*scale,y+32*scale,x+27*scale,y+32*scale,edge);
  frame.fillTriangle(x+16*scale,y+14*scale,x+7*scale,y+31*scale,x+25*scale,y+31*scale,c);
  frame.fillRoundRect(x+2*scale,y+32*scale,28*scale,6*scale,2*scale,edge);
  frame.fillRoundRect(x+3*scale,y+33*scale,26*scale,4*scale,2*scale,c);
}
void screenHeader(const std::string& title) {
  frame.fillScreen(Bg);text(52,28,title,Ink,3);
  frame.fillRoundRect(388,20,48,42,12,Panel);
  frame.fillRoundRect(401,39,22,4,2,Ink);
  frame.fillTriangle(399,41,409,32,409,50,Ink);
}
void botImage(int index,int x,int y,int size=96) {
  auto pic=botPicture(index);
  if(!pic) {frame.fillRoundRect(x,y,size,size,12,0x246d);center(x+size/2,y+size/2-8,botName(index).substr(0,1));return;}
  auto dst=frame.getFramebuffer();
  for(int yy=0;yy<size;++yy)for(int xx=0;xx<size;++xx)
    dst[(y+yy)*480+x+xx]=pic[(yy*96/size)*96+xx*96/size];
}
void queen(int stage,int x,int y,int w,int h) {
  auto src=queens[std::max(0,std::min(5,stage))];if(!src)return;
  auto dst=frame.getFramebuffer();
  for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)
    if(x+xx>=0 && x+xx<480 && y+yy>=0 && y+yy<480)dst[(y+yy)*480+x+xx]=src[(yy*330/h)*224+xx*224/w];
}
uint16_t blend(uint16_t a,uint16_t b,int alpha) {
  int r=(((a>>11)&31)*(255-alpha)+((b>>11)&31)*alpha)/255;
  int g=(((a>>5)&63)*(255-alpha)+((b>>5)&63)*alpha)/255;
  int blue=((a&31)*(255-alpha)+(b&31)*alpha)/255;
  return uint16_t((r<<11)|(g<<5)|blue);
}
void card(int i,int x,int y) {
  auto dst=frame.getFramebuffer();auto src=cards[i];if(!src)return;
  for(int yy=0;yy<CardHeight;++yy) {
    int dy=y+yy;if(dy<ViewTop || dy>=ViewBottom)continue;
    for(int xx=0;xx<CardWidth;++xx) {
      int dx=x+xx;if(dx<0 || dx>=480)continue;
      auto p=src+(yy*CardWidth+xx)*4;int alpha=p[3];if(!alpha)continue;
      auto rgb=uint16_t(((p[0]>>3)<<11)|((p[1]>>2)<<5)|(p[2]>>3));
      auto& target=dst[dy*480+dx];target=alpha==255?rgb:blend(target,rgb,alpha);
    }
  }
}
void drawSplash(uint32_t elapsed) {
  frame.fillScreen(Bg);
  center(240,20,"BAUERINO",Ink,3);
  // Blend all original M5 queen stages, from frost to fire, without blocking UCI/Wi-Fi.
  int progress=std::min(1275,int(elapsed)*1275/3000),stage=std::min(4,progress/255),alpha=progress-stage*255;
  auto dst=frame.getFramebuffer();auto a=queens[stage],b=queens[stage+1];
  for(int y=0;y<330;++y)for(int x=0;x<224;++x)dst[(y+65)*480+x+128]=blend(a[y*224+x],b[y*224+x],alpha);
  center(240,414,elapsed<1500?"FROST":"FIRE",elapsed<1500?0x6f7f:0xfc68,3);
  frame.fillRoundRect(140,458,200,3,1,Panel);
  frame.fillRoundRect(140,458,std::min(200,int(elapsed)*200/3300),3,1,Accent);
}
void drawHome() {
  memcpy(frame.getFramebuffer(),wallpaper,480*480*2);
  text(24,19,"BAUERINO",Ink,2);text(342,22,"WiFi / BLE",Muted,1);
  for(int i=0;i<AppCount;++i)card(i,cardX(i)+(i/4-homePage)*480+drag,cardY(i));
  for(int i=0;i<PageCount;++i)frame.fillCircle(231+i*20,459,4,i==homePage?Ink:Muted);
  frame.fillRoundRect(195,473,90,3,2,Ink);
  if(toastUntil && int32_t(toastUntil-millis())>0) {
    frame.fillRoundRect(30,398,420,38,12,Panel);center(240,409,toast);
  }
}
void retroSegment(int x,int y,int w,int h,bool lit) {
  frame.fillRoundRect(x-2,y-2,w+4,h+4,4,lit?0x3800:0x1000);
  frame.fillRoundRect(x,y,w,h,3,lit?0xf800:0x1800);
  if(lit)frame.fillRoundRect(x+2,y+2,std::max(1,w-4),std::max(1,h-4),2,0xf980);
}
void retroDigit(int x,int y,char c) {
  const uint8_t digits[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
  uint8_t mask=c>='0' && c<='9'?digits[c-'0']:0x40;
  retroSegment(x+9,y,44,7,mask&1);retroSegment(x+54,y+8,7,30,mask&2);
  retroSegment(x+54,y+48,7,30,mask&4);retroSegment(x+9,y+79,44,7,mask&8);
  retroSegment(x+1,y+48,7,30,mask&16);retroSegment(x+1,y+8,7,30,mask&32);
  retroSegment(x+9,y+40,44,7,mask&64);
}
void retroClock(int y) {
  // No engine time source yet: dashes explicitly indicate unavailable clocks.
  for(int i=0;i<4;++i)retroDigit(84+i*76+(i>=2?18:0),y,'-');
  frame.fillCircle(240,y+27,4,0xf800);frame.fillCircle(240,y+59,4,0xf800);
}
void drawRetro() {
  mameFrontendDraw(frame.getFramebuffer());return;
  frame.fillScreen(0x0000);auto dst=frame.getFramebuffer();
  for(int y=8;y<472;++y)for(int x=8;x<472;++x) {
    int dx=x<30?30-x:x>449?x-449:0,dy=y<30?30-y:y>449?y-449:0;
    if(dx*dx+dy*dy>22*22)continue;
    unsigned noise=(unsigned(x)*73856093U)^(unsigned(y)*19349663U);
    int v=3+int((noise^(noise>>13))%3);dst[y*480+x]=uint16_t((v<<11)|((v*2)<<5)|v);
  }
  constexpr uint16_t Cream=0xe6b3,Wine=0x880a;
  frame.drawRoundRect(8,8,464,464,22,0x4208);
  frame.fillRect(28,35,70,2,Wine);frame.fillRect(382,35,70,2,Wine);
  center(240,22,"bauerino retrochess",Cream,2);
  frame.fillTriangle(22,19,29,14,29,24,Cream);
  frame.fillRoundRect(28,56,424,219,20,0x0000);
  frame.drawRoundRect(28,56,424,219,20,0x630c);
  frame.drawRoundRect(32,60,416,211,16,0x2104);
  frame.fillRoundRect(36,64,408,203,13,0x0800);
  retroClock(72);retroClock(171);
  frame.fillRect(28,288,424,2,Wine);
  const char* labels[]={"Board","Notation","---","---","---","---"};
  for(int i=0;i<6;++i) {
    int x=30+(i%3)*145,y=306+(i/3)*80;
    frame.fillRoundRect(x-2,y-2,134,68,10,0x0000);
    frame.fillRoundRect(x,y,130,64,9,0x4208);
    for(int row=3;row<61;++row) {
      int v=row<12?8+row/3:12-(row-12)*8/48;
      int edge=row<9?9-row/2:row>55?3+(row-55)/2:3;
      uint16_t shade=i==5?uint16_t(((v+8)<<11)|((v/3)<<5)|(v/4)):uint16_t((v<<11)|((v*2)<<5)|v);
      frame.drawFastHLine(x+edge,y+row,130-edge*2,shade);
    }
    frame.drawFastHLine(x+12,y+4,106,i==5?0xa98d:0x8c51);
    frame.drawFastHLine(x+12,y+60,106,0x0000);
    center(x+65,y+24,labels[i],Cream,2);
  }
  frame.fillRect(28,460,424,2,Wine);
}
void drawGame(const core2::Game& game,const BoardView& link,bool active,bool input,const std::string& error,const SearchView& search,bool retro=false) {
  int playing=retro?-1:displayCurrentBot();
  frame.fillScreen(Bg);text(56,17,retro?"Retrochess":playing>=0?botName(playing)+" "+std::to_string(700+playing/5*50):"Virginia",Ink,2);
  frame.fillRoundRect(388,14,48,42,12,Panel);
  for(int i=0;i<3;++i)frame.fillRoundRect(399,24+i*9,26,4,2,Ink);
  text(56,40,(!error.empty()?error:game.message).substr(0,26),error.empty()?Accent:0xfc68,2);
  auto board=game.displayBoard();uint64_t diff=game.differences();
  Move mark=game.pending?game.pending:game.last;
  for(int row=0;row<8;++row)for(int col=0;col<8;++col) {
    int sq=(7-row)*8+col;if(game.human==BLACK)sq=63-sq;
    int x=BoardX+col*Cell,y=BoardY+row*Cell,dark=(row+col)&1;
    const char* order="KQRBNPkqrbnp";const char* p=strchr(order,board[sq]);
    if(p && board[sq]!='.') {
      auto tile=pieces+(int(p-order)*2+dark)*48*48;auto dst=frame.getFramebuffer();
      for(int yy=0;yy<Cell;++yy)for(int xx=0;xx<Cell;++xx)dst[(y+yy)*480+x+xx]=tile[(yy*48/Cell)*48+xx*48/Cell];
    } else frame.fillRect(x,y,Cell,Cell,dark?0x7d19:0xef9f);
    uint16_t outline=0;
    if((diff&(1ULL<<sq)) && !game.setup)outline=0xfc68;
    if(mark && (sq==from_sq(mark) || sq==to_sq(mark)))outline=Accent;
    if(outline)for(int j=0;j<3;++j)frame.drawRect(x+j,y+j,Cell-2*j,Cell-2*j,outline);
    if(col==0)text(x+2,y+2,std::string(1,char('1'+sq/8)),dark?Bg:0x3a6a,1);
    if(row==7)text(x+Cell-9,y+Cell-9,std::string(1,char('a'+sq%8)),dark?Bg:0x3a6a,1);
  }
  frame.fillRoundRect(338,64,134,362,14,Panel);
  frame.fillCircle(350,79,4,(input || link.connected)?Accent:0xfc68);
  text(362,74,retro?"RETRO":link.kind==BoardKind::ChessLink?"CHESSLINK":"CHESSNUT",Muted,1);
  text(348,99,game.thinking?"RECHNET":game.pending?"NACHZIEHEN":active?"DU AM ZUG":game.ended?"ENDE":"VORSCHAU",Accent,1);
  text(348,125,"SUCHTIEFE",Muted,1);text(348,141,std::to_string(search.depth),Ink,3);
  if(playing>=0)botImage(playing,404,121,60);
  else if(!retro)queen(core2::QueenMood::stage(game.mood.playerEval),408,121,56,82);
  text(348,188,"WEISSSICHT",Muted,1);text(348,205,search.score(),Accent,2);
  text(348,240,game.human==WHITE?"Du: Weiss":"Du: Schwarz",Ink,1);
  frame.fillRoundRect(346,263,118,37,8,Bg);
  std::string last=game.pending?move_to_uci(game.pending):game.moves.empty()?"--":game.moves.back();
  center(405,275,last,Accent,2);
  text(348,317,playing>=0?"SPIELSTIL":"VARIANTE",Muted,1);
  if(playing>=0) {
    text(348,336,botStyle(playing%5),Accent,2);
    text(348,372,botWins(playing)==3?"Beide besiegt":botWins(playing)==1?"Weiss besiegt":botWins(playing)==2?"Schwarz besiegt":"Noch offen",Muted,1);
  }
  std::istringstream pv(search.pv);std::string word,line;int py=336;
  while(playing<0 && pv>>word) {
    if(line.size()+word.size()+1>10) {text(348,py,line,Ink,2);py+=21;line.clear();if(py>399)break;}
    if(!line.empty())line+=' ';line+=word;
  }
  if(!line.empty() && py<=399)text(348,py,line,Ink,2);
  text(12,401,"Zug "+std::to_string(game.moves.size()/2+1),Muted,2);
  text(144,401,retro?"Retro-Vorschau":link.connected?"Brett verbunden":"Brett offline",Muted,1);
  if(notation) {
    frame.fillRoundRect(8,64,320,362,12,Panel);
    text(24,80,"Nr.",Muted,2);text(82,80,"Weiss",Ink,2);text(204,80,"Schwarz",Ink,2);
    int rows=int((game.moves.size()+1)/2);
    if(!rows)center(168,220,"Noch keine Zuege",Muted,2);
    for(int r=0;r<9 && notationTop+r<rows;++r) {
      int n=notationTop+r,y=114+r*32;
      if(n%2==0)frame.fillRoundRect(16,y-5,304,29,5,Bg);
      text(24,y,std::to_string(n+1)+".",Muted,2);
      for(int c=0;c<2;++c)if(size_t(n*2+c)<game.moves.size())text(82+c*122,y,game.moves[n*2+c],Accent,2);
    }
    text(24,410,"Wischen: aeltere / neuere Zuege",Muted,1);
  }
  pill(40,440,124,34,retro?"Display":"Boards");pill(178,440,124,34,notation?"Board":"Notation");pill(316,440,124,34,"Home");
}
int modeY() {return 94+(expanded==1?132:62)+14;}
void drawControls(const BoardView& link,bool active) {
  screenHeader("NEW GAME");text(54,70,"Deine Farbe. Deine Herausforderung.",Muted,1);
  frame.fillRoundRect(40,94,400,expanded==1?132:62,16,Panel);
  text(58,107,"DEINE FARBE",Muted,1);text(58,126,newBlack?"Schwarz":"Weiss",Ink,2);
  pawn(378,104,newBlack);
  text(422,120,expanded==1?"-":"+",Accent,2);
  if(expanded==1) {
    for(int i=0;i<2;++i) {
      int x=56+i*192;frame.fillRoundRect(x,153,176,60,12,newBlack==bool(i)?0x246d:Bg);
      pawn(x+12,163,i);text(x+56,175,i?"Schwarz":"Weiss",Ink,2);
    }
  }
  int y=modeY();
  frame.fillRoundRect(40,y,400,expanded==2?(adaptive?110:150):62,16,Panel);
  text(58,y+12,"SPIELSTAERKE",Muted,1);text(58,y+31,adaptive?"Adaptive":"Elo "+std::to_string(strengthElo),Ink,2);
  text(422,y+26,expanded==2?"-":"+",Accent,2);
  if(expanded==2) {
    pill(56,y+51,176,43,"Adaptive",adaptive?0x246d:Bg);
    pill(248,y+51,176,43,"Elo",adaptive?Bg:0x246d);
    if(!adaptive) {
      pill(56,y+104,58,34,"-");center(240,y+112,std::to_string(strengthElo)+" Elo",Accent,2);pill(366,y+104,58,34,"+");
    }
  }
  center(240,344,adaptive?"Passt sich deinem Spiel an":"Feste Spielstaerke: 700 bis 3000 Elo",Muted,1);
  center(240,364,link.connected?(link.kind==BoardKind::ChessLink?"ChessLink verbunden":"Chessnut verbunden"):"Bitte ein E-Board verbinden",link.connected?Accent:Muted,1);
  pill(40,384,400,48,link.connected?"Spielen":"Board verbinden",Accent);
  pill(40,442,190,30,active?"Beenden":"Vorschau");pill(250,442,190,30,"Zurueck");
}
void drawBoards(const BoardView& link,bool active) {
  screenHeader("BOARDS");text(54,70,"Antippen. Suchen. Verbinden.",Muted,1);
  for(int i=0;i<2;++i) {
    int x=40+i*208;bool connected=link.connected && link.kind==(i?BoardKind::ChessLink:BoardKind::Chessnut);
    frame.fillRoundRect(x,96,192,132,16,connected?0x246d:Panel);
    for(int r=0;r<6;++r)for(int c=0;c<6;++c)frame.fillRect(x+66+c*10,110+r*10,9,9,(r+c)%2?0x3a6a:Accent);
    center(x+96,183,i?"ChessLink":"Chessnut",Ink,2);
    center(x+96,211,connected?"VERBUNDEN":"SUCHEN & VERBINDEN",connected?Accent:Muted,1);
  }
  for(size_t i=0;i<link.peers.size() && i<3 && !link.busy && !link.connected;++i) {
    pill(40,248+int(i)*50,400,42,link.peers[i].name.substr(0,28));
  }
  if(link.connected)pill(40,292,400,48,"Weiter",Accent);
  center(240,416,active?"Brettwechsel: zuerst Partie beenden":link.busy?"Bitte warten ...":"Dein Brett muss eingeschaltet sein",Muted,1);
  center(240,440,link.status.substr(0,48),Accent,1);
}
void botFace(int index,int cell,bool level) {
  int x=botCellX(cell),y=botCellY(cell);frame.fillRoundRect(x,y,128,152,16,Panel);
  botImage(index,x+16,y+8);
  center(x+64,y+110,level?std::to_string(700+index/5*50):botName(index),Ink,2);
  center(x+64,y+134,level?"ELO":botStyle(index%5),Accent,1);
  auto won=botWins(index);
  if(won) {
    frame.fillCircle(x+111,y+16,11,won==3?0x350b:Bg);
    frame.drawCircle(x+111,y+16,11,Accent);
    if(won==3)center(x+111,y+13,"OK",Ink,1);
    else frame.fillCircle(x+111,y+16,5,won==1?Ink:0x0000);
  }
}
int botLevelCount() {auto g=botGroup(botGroupIndex);return (g.to-g.from)/50+1;}
void drawBotGroups() {
  screenHeader("BEAT BOTS");text(54,70,"215 Gegner. Fuenf Persoenlichkeiten.",Muted,1);
  for(int i=0;i<5;++i) {
    auto g=botGroup(i);int y=96+i*64,done=0;
    for(int b=(g.from-700)/50*5;b<=(g.to-700)/50*5+4;++b)if(botWins(b)==3)++done;
    frame.fillRoundRect(40,y,400,54,12,Panel);botImage((g.from-700)/50*5+2,52,y+5,44);
    text(112,y+9,g.label,Ink,2);text(112,y+34,std::to_string(g.from)+" - "+std::to_string(g.to),Muted,1);
    text(344,y+20,std::to_string(done)+"/"+std::to_string((g.to-g.from)/50*5+5),Accent,1);
  }
}
void drawBotLevels() {
  auto g=botGroup(botGroupIndex);screenHeader(g.label);text(54,70,"Elo-Stufe waehlen",Muted,1);
  int first=botLevelPage*6,count=std::min(6,botLevelCount()-first);
  for(int i=0;i<count;++i)botFace((g.from-700)/50*5+(first+i)*5+2,i,true);
  pill(40,440,96,34,"<");center(240,450,std::to_string(botLevelPage+1)+" / "+std::to_string((botLevelCount()+5)/6),Muted,1);pill(344,440,96,34,">");
}
void drawBotStyles() {
  screenHeader("ELO "+std::to_string(chosenElo));text(54,70,"Waehle deinen Gegner",Muted,1);
  for(int i=0;i<5;++i)botFace((chosenElo-700)/50*5+i,i,false);
}
void drawBotStart(const BoardView& link,bool active) {
  screenHeader("BOT START");text(54,70,"Dein Gegner. Deine Farbe.",Muted,1);
  frame.fillRoundRect(40,92,400,132,16,Panel);botImage(chosenBot,56,104);
  text(174,105,botName(chosenBot),Ink,2);text(174,133,botStyle(chosenBot%5),Accent,2);
  text(174,164,std::to_string(700+chosenBot/5*50)+" Elo",Muted,2);
  auto won=botWins(chosenBot);
  text(174,197,won==3?"Mit beiden Farben besiegt":won==1?"Weiss geschafft; Schwarz fehlt":won==2?"Schwarz geschafft; Weiss fehlt":"Gewinne mit Weiss und Schwarz",Muted,1);
  for(int i=0;i<2;++i) {
    int x=40+i*208;frame.fillRoundRect(x,244,192,66,14,newBlack==bool(i)?0x246d:Panel);
    pawn(x+20,257,i);text(x+72,270,i?"Schwarz":"Weiss",Ink,2);
  }
  pill(40,334,400,48,link.connected?"Spielen":"Board verbinden",Accent);
  pill(40,398,190,42,active?"Beenden":"Vorschau");pill(250,398,190,42,"Zurueck");
  center(240,458,link.connected?"Brett verbunden":"Vorschau auch ohne Board",Muted,1);
}
void tap(int x,int y,const core2::Game& game,const BoardView& link,bool active,bool input) {
  Serial.printf("[DISPLAY TAP] %d %d page=%d\n",x,y,int(page));
  dirty=true;
  if(page==Page::Home) {
    int app=appAt(x,y,homePage);
    if(app==0) {retroMode=false;displayAction(DisplayAction::PlayVirginia);controlsReturn=Page::Home;page=active?Page::Game:Page::Controls;}
    else if(app==1) {if(botsAvailable()) {retroMode=false;page=Page::BotGroups;}else notice("Botdateien fehlen");}
    else if(app==2) {displayAction(DisplayAction::PlayRetro);retroMode=true;notation=false;page=Page::Retro;}
    else if(app>=0)notice("Kommt spaeter");
  } else if(page==Page::Game) {
    if(in(x,y,388,14,48,42)) {
      if(retroMode)page=Page::Retro;
      else if(displayCurrentBot()>=0) {chosenBot=displayCurrentBot();newBlack=game.human==BLACK;botStartReturn=Page::Game;page=Page::BotStart;}
      else {controlsReturn=Page::Game;page=Page::Controls;}
    }
    else if(in(x,y,40,440,124,34)) {if(retroMode)page=Page::Retro;else {boardReturn=Page::Game;page=Page::Boards;}}
    else if(in(x,y,178,440,124,34)) {
      notation=!notation;notationTop=std::max(0,int((game.moves.size()+1)/2)-9);
    } else if(in(x,y,316,440,124,34)) {retroMode=false;page=Page::Home;}
  } else if(page==Page::Retro) {
    if(in(x,y,14,10,46,38)) {
      if(mameFrontendBusy()){mameFrontendStop();retroLeaving=true;notice("Emulation wird beendet ...");}
      else {retroMode=false;page=Page::Home;}
    } else if(!retroLeaving)mameFrontendTap(x,y);
  } else if(page==Page::Controls) {
    int my=modeY();
    if(in(x,y,388,20,48,42) || in(x,y,250,438,190,42))page=controlsReturn;
    else if(expanded==1 && in(x,y,56,153,368,60)) {newBlack=x>=240;expanded=2;}
    else if(in(x,y,40,94,400,62))expanded=expanded==1?0:1;
    else if(expanded==2 && in(x,y,56,my+51,368,43))adaptive=x<240;
    else if(expanded==2 && !adaptive && in(x,y,56,my+104,58,34))strengthElo=std::max(700,strengthElo-100);
    else if(expanded==2 && !adaptive && in(x,y,366,my+104,58,34))strengthElo=std::min(3000,strengthElo+100);
    else if(in(x,y,40,my,400,62))expanded=expanded==2?0:2;
    else if(in(x,y,40,384,400,48)) {
      if(!link.connected) {boardReturn=Page::Controls;page=Page::Boards;}
      else {displayAction(DisplayAction::NewGame,newBlack?1:0,adaptive?0:strengthElo);notation=false;page=Page::Game;}
    } else if(in(x,y,40,438,190,42)) {
      if(active) {displayAction(DisplayAction::Abort);page=Page::Controls;}
      else {displayAction(DisplayAction::Preview,newBlack?1:0);page=Page::Game;}
    }
  } else if(page==Page::Boards) {
    if(in(x,y,388,20,48,42))page=boardReturn;
    else if(link.connected && in(x,y,40,292,400,48))page=boardReturn;
    else if(!active && !link.busy) {
      if(in(x,y,40,96,400,132))displayAction(DisplayAction::Scan,x>=240?1:0);
      else if(!link.connected)for(size_t i=0;i<link.peers.size() && i<3;++i)
        if(in(x,y,40,248+int(i)*50,400,42)) {displayAction(DisplayAction::Connect,int(i));break;}
    }
  } else if(page==Page::BotGroups) {
    if(in(x,y,388,20,48,42))page=Page::Home;
    else for(int i=0;i<5;++i)if(in(x,y,40,96+i*64,400,54)) {botGroupIndex=i;botLevelPage=0;page=Page::BotLevels;break;}
  } else if(page==Page::BotLevels) {
    if(in(x,y,388,20,48,42))page=Page::BotGroups;
    else if(in(x,y,40,440,96,34))botLevelPage=std::max(0,botLevelPage-1);
    else if(in(x,y,344,440,96,34))botLevelPage=std::min((botLevelCount()-1)/6,botLevelPage+1);
    else {
      int cell=botCellAt(x,y,std::min(6,botLevelCount()-botLevelPage*6));
      if(cell>=0) {chosenElo=botGroup(botGroupIndex).from+(botLevelPage*6+cell)*50;page=Page::BotStyles;}
    }
  } else if(page==Page::BotStyles) {
    if(in(x,y,388,20,48,42))page=Page::BotLevels;
    else {int cell=botCellAt(x,y,5);if(cell>=0) {chosenBot=(chosenElo-700)/50*5+cell;newBlack=botWins(chosenBot)==1;botStartReturn=Page::BotStyles;page=Page::BotStart;}}
  } else if(page==Page::BotStart) {
    if(in(x,y,388,20,48,42) || in(x,y,250,398,190,42))page=botStartReturn;
    else if(in(x,y,40,244,400,66))newBlack=x>=240;
    else if(in(x,y,40,334,400,48)) {
      if(!link.connected) {boardReturn=Page::BotStart;page=Page::Boards;}
      else {displayAction(DisplayAction::NewBot,chosenBot,newBlack?1:0);notation=false;page=Page::Game;}
    } else if(in(x,y,40,398,190,42)) {
      if(active)displayAction(DisplayAction::Abort);
      else {displayAction(DisplayAction::BotPreview,chosenBot,newBlack?1:0);notation=false;page=Page::Game;}
    }
  }

}
}
bool displayBegin() {
  if(!psramFound() || !frame.begin()) {Serial.println("[DISPLAY] framebuffer/panel failed");return false;}
  bus.writeC8D8(0x36,0xa0);panel.setBrightness(160);frame.setTextWrap(false);frame.fillScreen(Bg);frame.flush();
  wallpaper=static_cast<uint16_t*>(load("wallpaper.rgb",480*480*2));
  pieces=static_cast<uint16_t*>(load("pieces.rgb",24*48*48*2));
  bool assets=wallpaper && pieces;
  for(int i=0;i<6;++i) {std::string name="queen-"+std::to_string(i)+".rgb";queens[i]=static_cast<uint16_t*>(load(name.c_str(),224*330*2));assets=assets && queens[i];}
  for(int i=0;i<5;++i) {std::string name="app-"+std::to_string(i)+".rgba";cards[i]=static_cast<uint8_t*>(load(name.c_str(),CardWidth*CardHeight*4));assets=assets && cards[i];}
  if(!assets) {text(20,180,"Displaydateien fehlen");text(20,220,"LittleFS aufspielen");frame.flush();return false;}
  botsBegin();
  touch.setPins(40,11);touchOnline=touch.begin(Wire,CST92XX_SLAVE_ADDRESS,15,14);
  if(touchOnline) {
    touch.setMaxCoordinates(480,480);touch.setSwapXY(true);touch.setMirrorXY(true,false);
    gestures=xQueueCreate(8,sizeof(Gesture));
    touchOnline=gestures && xTaskCreatePinnedToCore(touchTask,"touch",4096,nullptr,2,&touchTaskHandle,0)==pdPASS;
    if(touchOnline) {
      pinMode(11,INPUT_PULLUP);attachInterrupt(digitalPinToInterrupt(11),touchInterrupt,FALLING);
      xTaskNotifyGive(touchTaskHandle);
    }
  }
  Serial.printf("[DISPLAY] 480x480 CO5300; touch=%s; assets=OK\n",touchOnline?touch.getModelName():"FAILED");
  if(!touchOnline) {frame.fillScreen(Bg);text(20,200,"Touch nicht erkannt");frame.flush();return false;}
  retroGame.start(WHITE);retroGame.message="Retro-Vorschau";
  if(!mameFrontendBegin()){Serial.println("[RETRO] Frontend init failed");return false;}
  began=millis();online=true;return true;
}
bool displayDiagnostic(const std::string& command) {
  int x,y;
  if(sscanf(command.c_str(),"swipe %d",&x)==1 && abs(x)>=60 && abs(x)<=200) {
    Gesture g;g.sx=240;g.x=240+x;g.sy=g.y=240;g.began=millis();return gestures && xQueueSend(gestures,&g,0)==pdTRUE;
  }
  if(sscanf(command.c_str(),"tap %d %d",&x,&y)==2 && x>=0 && x<480 && y>=0 && y<480) {
    Gesture g;g.sx=g.x=x;g.sy=g.y=y;g.began=millis();return gestures && xQueueSend(gestures,&g,0)==pdTRUE;
  }
  if(command=="screenshot" && online) {capturePending=true;dirty=true;return true;}
  if(command=="_capture" && online) {
    struct CaptureGuard {CaptureGuard(){mameFrontendCapture(true);}~CaptureGuard(){mameFrontendCapture(false);}} captureGuard;
    // One HWCDC write holds its transmit lock across header, pixels and footer,
    // preventing Wi-Fi/BLE task logs from interleaving with binary pixels.
    constexpr char header[]="[LCD FRAME] 480 480 RGB565BE 460800\n",footer[]="\n[LCD FRAME END]\n";
    constexpr size_t bytes=sizeof(header)-1+460800+sizeof(footer)-1;
    auto payload=static_cast<uint8_t*>(heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    if(!payload){Serial.println("[LCD ERROR] capture allocation");return false;}
    memcpy(payload,header,sizeof(header)-1);uint8_t *image=payload+sizeof(header)-1;
    const uint16_t *pixels=frame.getFramebuffer();
    for(int n=0;n<480*480;++n){image[n*2]=pixels[n]>>8;image[n*2+1]=pixels[n]&255;}
    memcpy(image+460800,footer,sizeof(footer)-1);
    Serial.setTxTimeoutMs(5000);
    bool ok=Serial.write(payload,bytes)==bytes;
    Serial.setTxTimeoutMs(100);
    free(payload);return ok;
  }
  return false;
}
void displayTick(const core2::Game& game,const BoardView& board,bool active,bool input,const std::string& error,const SearchView& search) {
  if(!online)return;
  mameFrontendTick();
  if(page==Page::Retro && mameFrontendDirty())dirty=true;
  if(retroLeaving && !mameFrontendBusy()) {retroLeaving=false;retroMode=false;page=Page::Home;dirty=true;}
  uint32_t now=millis();
  if(page==Page::Splash && now-began>=3300) {page=Page::Home;dirty=true;}
  Gesture g;
  while(xQueueReceive(gestures,&g,0)==pdTRUE) {
    int dx=g.x-g.sx,dy=g.y-g.sy;
    Serial.printf("[TOUCH] %d,%d -> %d,%d dx=%d dy=%d\n",g.sx,g.sy,g.x,g.y,dx,dy);
    if(page==Page::Home && abs(dx)>40 && abs(dx)>abs(dy)) {
      homePage=swipePage(homePage,dx);drag=0;dirty=true;
      Serial.printf("[DISPLAY] home page=%d\n",homePage+1);
    } else if(page==Page::BotLevels && abs(dx)>60 && abs(dx)>abs(dy)) {
      botLevelPage=std::max(0,std::min((botLevelCount()-1)/6,botLevelPage+(dx<0?1:-1)));dirty=true;
    } else if(page==Page::Game && notation && g.sx<328 && abs(dy)>35 && abs(dy)>abs(dx)) {
      notationTop=std::max(0,std::min(std::max(0,int((game.moves.size()+1)/2)-9),notationTop-dy/32));dirty=true;
    } else if(page!=Page::Splash && now-g.began<1500 && abs(dx)<14 && abs(dy)<14)
      tap(g.x,g.y,game,board,active,input);
  }
  portENTER_CRITICAL(&touchMux);g=liveTouch;portEXIT_CRITICAL(&touchMux);
  int nextDrag=0;
  if(page==Page::Home && g.down && abs(g.x-g.sx)>14 && abs(g.x-g.sx)>abs(g.y-g.sy)) {
    nextDrag=std::max(-480,std::min(480,g.x-g.sx));
    if((homePage==0 && nextDrag>0) || (homePage==PageCount-1 && nextDrag<0))nextDrag/=4;
  }
  if(nextDrag!=drag) {drag=nextDrag;dirty=true;}
  if(notationCount!=game.moves.size()) {
    notationCount=game.moves.size();notationTop=std::max(0,int((notationCount+1)/2)-9);dirty=true;
  }
  if(toastUntil && int32_t(now-toastUntil)>=0) {toastUntil=0;dirty=true;}
  std::string signature=std::to_string(game.epoch)+":"+std::to_string(game.moves.size())+":"+
    game.message+":"+std::to_string(game.thinking)+":"+std::to_string(game.pending)+":"+
    board.status+":"+std::to_string(board.revision)+":"+std::to_string(board.peers.size())+":"+
    std::to_string(active)+":"+std::to_string(input)+":"+error+":"+
    std::to_string(search.depth)+":"+search.score()+":"+search.pv+":"+std::to_string(botProgressRevision());
  if(signature!=lastSignature) {lastSignature=signature;dirty=true;}
  if(now-lastFrame<(page==Page::Game?100U:33U) || (!dirty && page!=Page::Splash))return;
  lastFrame=now;dirty=false;
  switch(page) {
    case Page::Splash:drawSplash(now-began);break;
    case Page::Home:drawHome();break;
    case Page::Game:
      if(retroMode)drawGame(retroGame,BoardView{},false,false,"",SearchView{},true);
      else drawGame(game,board,active,input,error,search);
      break;
    case Page::Controls:drawControls(board,active);break;
    case Page::Boards:drawBoards(board,active);break;
    case Page::Retro:drawRetro();break;
    case Page::BotGroups:drawBotGroups();break;
    case Page::BotLevels:drawBotLevels();break;
    case Page::BotStyles:drawBotStyles();break;
    case Page::BotStart:drawBotStart(board,active);break;
  }
  if(page!=Page::Home && toastUntil && int32_t(toastUntil-now)>0) {
    frame.fillRoundRect(65,276,350,34,10,Panel);center(240,286,toast,Ink,2);
  }
  frame.flush();
  if(capturePending){capturePending=false;displayDiagnostic("_capture");}
}
#endif
