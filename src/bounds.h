#ifndef BOUNDS_H_
#define BOUNDS_H_

#include <algorithm>

using namespace std;

class Bounds {
public: /* statics */
  static Bounds* createRectangle(int x, int y, int width, int height) { return new Bounds(x,y,width,height); }
  static Bounds* createBounds(int left, int top, int right, int bottom) {
    int realLeft = min(left, right);
    int realTop = min(top, bottom);
    int realRight = max(left, right);
    int realBottom = max(top, bottom);

    return new Bounds(realLeft, realTop, (realRight-realLeft), (realBottom-realTop)); 
  }
  Bounds(int x, int y, int width, int height) : left(x), top(y), right(x+width), bottom(y+height) {}
  virtual ~Bounds() {}
public:
  int getWidth() {
    return right-left;
  }
  int getHeight() {
    return bottom-top;
  }
  int getX() {
    return left;
  }
  int getY() {
    return top;
  }
private:
  int top,left,bottom,right;

};

#endif