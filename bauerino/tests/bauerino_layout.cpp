#include "bauerino_layout.h"
#include "bauerino_search.h"
#include <cassert>
#include <iostream>
using namespace bauerino;
int main() {
  static_assert(Width==480 && Height==480);
  for(int i=0;i<AppCount;++i) {
    assert(appAt(cardX(i)+96,cardY(i)+80,i/4)==i);
    assert(appAt(cardX(i)+96,cardY(i)+80,1-i/4)!=i);
    assert(cardX(i)>=0 && cardX(i)+CardWidth<=480);
    assert(cardY(i)>=ViewTop && cardY(i)+CardHeight<=ViewBottom);
  }
  assert(appAt(120,459,0)==-1);assert(appAt(240,80,0)==-1);
  assert(swipePage(0,-100)==1);assert(swipePage(1,100)==0);
  assert(swipePage(0,100)==0);assert(swipePage(1,-100)==1);
  assert(swipePage(0,20)==0);
  for(int row=0;row<8;++row)for(int col=0;col<8;++col) {
    int x=BoardX+col*Cell+Cell/2,y=BoardY+row*Cell+Cell/2;
    assert(squareAt(x,y,false)==(7-row)*8+col);
    assert(squareAt(x,y,true)==63-((7-row)*8+col));
  }
  assert(squareAt(BoardX+8*Cell,200,false)==-1);assert(squareAt(BoardX,BoardY+8*Cell,false)==-1);
  assert(squareAt(BoardX-1,BoardY,false)==-1);assert(squareAt(BoardX,BoardY-1,false)==-1);
  SearchView s;s.read("info depth 12 score cp 37 pv e7e5 g1f3",false);
  assert(s.depth==12 && s.score()=="-0.37" && s.pv=="e7e5 g1f3");
  s.read("info string depth 999 score cp 900",true);assert(s.depth==12 && s.score()=="-0.37");
  s.read("info depth 13 nodes 200",true);assert(s.depth==13 && s.pv=="e7e5 g1f3");
  s.read("info score mate -3",false);assert(s.score()=="M3");
  s.read("info score cp 5",true);assert(s.score()=="+0.05");
  std::cout<<"PASS: 480x480 app hit regions, Setup to the right, 64 squares in both orientations\n";
}
