#pragma once
#include <algorithm>
namespace bauerino {
constexpr int Width=480, Height=480, ViewTop=60, ViewBottom=442;
constexpr int AppCount=5, CardWidth=192, CardHeight=164, RowHeight=182, PageCount=2;
constexpr int BoardX=8, BoardY=64, Cell=40;
inline int swipePage(int page,int dx) {
  if(dx<-60 && page<PageCount-1)return page+1;
  if(dx>60 && page>0)return page-1;
  return page;
}
inline int cardX(int i) {return i%2?264:24;}
inline int cardY(int i) {return 70+(i%4)/2*RowHeight;}
inline int appAt(int x,int y,int page) {
  if(y<ViewTop || y>=ViewBottom)return -1;
  for(int i=page*4;i<std::min(AppCount,(page+1)*4);++i)if(x>=cardX(i) && x<cardX(i)+CardWidth &&
    y>=cardY(i) && y<cardY(i)+CardHeight)return i;
  return -1;
}
inline int squareAt(int x,int y,bool black) {
  if(x<BoardX || y<BoardY || x>=BoardX+8*Cell || y>=BoardY+8*Cell)return -1;
  int square=(7-(y-BoardY)/Cell)*8+(x-BoardX)/Cell;
  return black?63-square:square;
}
}
