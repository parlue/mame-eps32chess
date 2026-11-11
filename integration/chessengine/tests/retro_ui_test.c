// Exercise the actual scanline UI controller with its UCI mailbox and tap path.
#define asm(x)
#define strtok_r strtok_s
#include "../../../esp32s3/src/retro_test_ui.c"
#include <assert.h>
const uint16_t piece_tiles[24*48*48]={0};
void bauerino_lcd_invalidate(bool full){(void)full;}
bool retro_touch_init(void){return false;}
bool retro_touch_point(int *x,int *y){(void)x;(void)y;return false;}
unsigned heap_caps_get_free_size(unsigned caps){(void)caps;return 0;}
static void expect(const char *expected){char command[896];assert(retro_ui_command(command,sizeof(command)));assert(!strcmp(command,expected));}
int main(void){
    retro_ui_init();
    retro_ui_tap(300,340); // Load
    retro_ui_tap(240,150); // MM IV
    char driver[4];assert(retro_ui_driver(driver)&&!strcmp(driver,"mm4"));
    retro_ui_open(driver);expect("uci");
    retro_ui_reply("option name Speed type spin default 100 min 0 max 10000");
    retro_ui_reply("option name Level type string default le 1");
    retro_ui_reply("uciok");expect("isready");retro_ui_reply("readyok");
    assert(option_count==2&&ready);
    retro_ui_tap(60,410); // Play
    retro_ui_tap(100,180); // White test
    expect("setoption name Speed value 0");expect("setoption name Level value le 1");
    expect("ucinewgame");expect("isready");retro_ui_reply("readyok");
    expect("position startpos moves e2e4");expect("go");
    retro_ui_reply("bestmove e7e5");
    assert(ply==2&&board[28]=='P'&&board[36]=='p');
    retro_ui_tap(60,340);assert(page==BOARD);
    retro_ui_tap(240,450);retro_ui_tap(180,340);assert(page==NOTATION);
    retro_ui_tap(240,450);retro_ui_tap(180,410); // Move switches engine colour
    expect("position startpos moves e2e4 e7e5");expect("go");
    retro_ui_reply("bestmove g1f3");assert(ply==3&&board[21]=='N');
    retro_ui_tap(410,410);expect("quit");retro_ui_closed();assert(!running&&!thinking);
    retro_ui_tap(300,340);retro_ui_tap(240,250); // MM V
    assert(retro_ui_driver(driver)&&!strcmp(driver,"mm5"));
    retro_ui_open(driver);expect("uci");retro_ui_reply("uciok");expect("isready");retro_ui_reply("readyok");
    retro_ui_tap(60,410);retro_ui_tap(350,180); // Human black: engine opens
    expect("ucinewgame");expect("isready");retro_ui_reply("readyok");expect("go");
    assert(ply==0&&thinking&&black);
    assert(!retro_ui_diagnostic("position startpos moves e2e4 e7e5"));
    assert(retro_ui_diagnostic("quit"));expect("quit");retro_ui_closed();
    assert(!retro_ui_diagnostic("go"));
    retro_ui_open("mm4");ready=true;
    assert(!retro_ui_diagnostic("position fen invalid"));
    char oversized[897];memset(oversized,'x',sizeof(oversized));oversized[896]=0;
    assert(!retro_ui_diagnostic(oversized));
    assert(retro_ui_diagnostic("position startpos moves e2e4 e7e5 g1f3 b8c6"));
    expect("position startpos moves e2e4 e7e5 g1f3 b8c6");assert(ply==4&&board[42]=='n');
    assert(retro_ui_diagnostic("go"));expect("go");assert(thinking);
    assert(!retro_ui_diagnostic("ucinewgame"));
    assert(retro_ui_diagnostic("quit"));expect("quit");retro_ui_closed();
    puts("PASS: actual Retro UI tap routing, UCI discovery, options, white/black start, board/notation, Move, Quit and driver switch");
    return 0;
}
