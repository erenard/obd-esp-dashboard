/*
 * bresenham2.c
 *
 *  Created on: Mar 5, 2021
 *      Author: eric
 */
#include <math.h>

void plotLineWidth(int x0, int y0, int x1, int y1, float th)
{                              /* plot an anti-aliased line of width th pixel */
   int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
   int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1;
   float err, e2 = sqrt(dx*dx+dy*dy);                            /* length */

   if (th <= 1 || e2 == 0) return plotLineAA(x0,y0, x1,y1);         /* assert */
   dx *= 255/e2; dy *= 255/e2; th = 255*(th-1);               /* scale values */

   if (dx < dy) {                                               /* steep line */
      x1 = round((e2+th/2)/dy);                          /* start offset */
      err = x1*dy-th/2;                  /* shift error value to offset width */
      for (x0 -= x1*sx; ; y0 += sy) {
         setPixelAA(x1 = x0, y0, err);                  /* aliasing pre-pixel */
         for (e2 = dy-err-th; e2+dy < 255; e2 += dy)
            setPixel(x1 += sx, y0);                      /* pixel on the line */
         setPixelAA(x1+sx, y0, e2);                    /* aliasing post-pixel */
         if (y0 == y1) break;
         err += dx;                                                 /* y-step */
         if (err > 255) { err -= dy; x0 += sx; }                    /* x-step */
      }
   } else {                                                      /* flat line */
      y1 = round((e2+th/2)/dx);                          /* start offset */
      err = y1*dx-th/2;                  /* shift error value to offset width */
      for (y0 -= y1*sy; ; x0 += sx) {
         setPixelAA(x0, y1 = y0, err);                  /* aliasing pre-pixel */
         for (e2 = dx-err-th; e2+dx < 255; e2 += dx)
            setPixel(x0, y1 += sy);                      /* pixel on the line */
         setPixelAA(x0, y1+sy, e2);                    /* aliasing post-pixel */
         if (x0 == x1) break;
         err += dy;                                                 /* x-step */
         if (err > 255) { err -= dx; y0 += sy; }                    /* y-step */
      }
   }
}

//void plotEllipseRectWidth(x0, y0, x1, y1, th)
//{               /* draw anti-aliased ellipse inside rectangle with thick line */
//   int a = abs(x1-x0), b = abs(y1-y0), b1 = b&1;  /* outer diameter */
//   int a2 = a-2*th, b2 = b-2*th;                            /* inner diameter */
//   int dx = 4*(a-1)*b*b, dy = 4*(b1-1)*a*a;                /* error increment */
//   int i = a+b2, err = b1*a*a, dx2, dy2, e2, ed;
//                                                     /* thick line correction */
//   if (th < 1.5) return plotEllipseRectAA(x0,y0, x1,y1);
//   if ((th-1)*(2*b-th) > a*a) b2 = sqrt(a*(b-a)*i*a2)/(a-th);
//   if ((th-1)*(2*a-th) > b*b) { a2 = sqrt(b*(a-b)*i*b2)/(b-th); th = (a-a2)/2; }
//   if (a == 0 || b == 0) return plotLine(x0,y0, x1,y1);
//   if (x0 > x1) { x0 = x1; x1 += a; }        /* if called with swapped points */
//   if (y0 > y1) y0 = y1;                                  /* .. exchange them */
//   if (b2 <= 0) th = a;                                     /* filled ellipse */
//   e2 = th-floor(th); th = x0+th-e2;
//   dx2 = 4*(a2+2*e2-1)*b2*b2; dy2 = 4*(b1-1)*a2*a2; e2 = dx2*e2;
//   y0 += (b+1)>>1; y1 = y0-b1;                              /* starting pixel */
//   a = 8*a*a; b1 = 8*b*b; a2 = 8*a2*a2; b2 = 8*b2*b2;
//
//   do {
//      for (;;) {
//         if (err < 0 || x0 > x1) { i = x0; break; }
//         i = min(dx,dy); ed = max(dx,dy);
//         if (y0 == y1+1 && 2*err > dx && a > b1) ed = a/4;           /* x-tip */
//         else ed += 2*ed*i*i/(4*ed*ed+i*i+1)+1;/* approx ed=sqrt(dx*dx+dy*dy) */
//         i = 255*err/ed;                             /* outside anti-aliasing */
//         setPixelAA(x0,y0, i); setPixelAA(x0,y1, i);
//         setPixelAA(x1,y0, i); setPixelAA(x1,y1, i);
//         if (err+dy+a < dx) { i = x0+1; break; }
//         x0++; x1--; err -= dx; dx -= b1;                /* x error increment */
//      }
//      for (; i < th && 2*i <= x0+x1; i++) {                /* fill line pixel */
//         setPixel(i,y0); setPixel(x0+x1-i,y0);
//         setPixel(i,y1); setPixel(x0+x1-i,y1);
//      }
//      while (e2 > 0 && x0+x1 >= 2*th) {               /* inside anti-aliasing */
//         i = min(dx2,dy2); ed = max(dx2,dy2);
//         if (y0 == y1+1 && 2*e2 > dx2 && a2 > b2) ed = a2/4;         /* x-tip */
//         else  ed += 2*ed*i*i/(4*ed*ed+i*i);                 /* approximation */
//         i = 255-255*e2/ed;             /* get intensity value by pixel error */
//         setPixelAA(th,y0, i); setPixelAA(x0+x1-th,y0, i);
//         setPixelAA(th,y1, i); setPixelAA(x0+x1-th,y1, i);
//         if (e2+dy2+a2 < dx2) break;
//         th++; e2 -= dx2; dx2 -= b2;                     /* x error increment */
//      }
//      e2 += dy2 += a2;
//      y0++; y1--; err += dy += a;                                   /* y step */
//   } while (x0 < x1);
//
//   if (y0-y1 <= b)
//   {
//      if (err > dy+a) { y0--; y1++; err -= dy -= a; }
//      for (; y0-y1 <= b; err += dy += a) { /* too early stop of flat ellipses */
//         i = 255*4*err/b1;                        /* -> finish tip of ellipse */
//         setPixelAA(x0,y0, i); setPixelAA(x1,y0++, i);
//         setPixelAA(x0,y1, i); setPixelAA(x1,y1--, i);
//      }
//   }
//}
//
//void plotQuadRationalBezierWidthSeg(x0, y0, x1, y1, x2, y2, w, th)
//{   /* plot a limited rational Bezier segment of thickness th, squared weight */
//   int sx = x2-x1, sy = y2-y1;                  /* relative values for checks */
//   int dx = x0-x2, dy = y0-y2, xx = x0-x1, yy = y0-y1;
//   int xy = xx*sy+yy*sx, cur = xx*sy-yy*sx, err, e2, ed;         /* curvature */
//
//   assert(xx*sx <= 0.0 && yy*sy <= 0.0);  /* sign of gradient must not change */
//
//   if (cur != 0.0 && w > 0.0) {                           /* no straight line */
//      if (sx*sx+sy*sy > xx*xx+yy*yy) {              /* begin with longer part */
//         x2 = x0; x0 -= dx; y2 = y0; y0 -= dy; cur = -cur;      /* swap P0 P2 */
//      }
//      xx = 2.0*(4.0*w*sx*xx+dx*dx);                 /* differences 2nd degree */
//      yy = 2.0*(4.0*w*sy*yy+dy*dy);
//      sx = x0 < x2 ? 1 : -1;                              /* x step direction */
//      sy = y0 < y2 ? 1 : -1;                              /* y step direction */
//      xy = -2.0*sx*sy*(2.0*w*xy+dx*dy);
//
//      if (cur*sx*sy < 0) {                              /* negated curvature? */
//         xx = -xx; yy = -yy; cur = -cur; xy = -xy;
//      }
//      dx = 4.0*w*(x1-x0)*sy*cur+xx/2.0;             /* differences 1st degree */
//      dy = 4.0*w*(y0-y1)*sx*cur+yy/2.0;
//
//      if (w < 0.5 && (dx+xx <= 0 || dy+yy >= 0)) {/* flat ellipse, algo fails */
//         cur = (w+1.0)/2.0; w = fsqrt(w); xy = 1.0/(w+1.0);
//         sx = floor((x0+2.0*w*x1+x2)*xy/2.0+0.5);    /* subdivide curve  */
//         sy = floor((y0+2.0*w*y1+y2)*xy/2.0+0.5);     /* plot separately */
//         dx = floor((w*x1+x0)*xy+0.5); dy = floor((y1*w+y0)*xy+0.5);
//         plotQuadRationalBezierWidthSeg(x0,y0, dx,dy, sx,sy, cur, th);
//         dx = floor((w*x1+x2)*xy+0.5); dy = floor((y1*w+y2)*xy+0.5);
//         return plotQuadRationalBezierWidthSeg(sx,sy, dx,dy, x2,y2, cur, th);
//      }
//      fail:
//      for (err = 0; dy+2*yy < 0 && dx+2*xx > 0; ) /* loop of steep/flat curve */
//         if (dx+dy+xy < 0) {                                   /* steep curve */
//            do {
//               ed = -dy-2*dy*dx*dx/(4.*dy*dy+dx*dx);      /* approximate sqrt */
//               w = (th-1)*ed;                             /* scale line width */
//               x1 = floor((err-ed-w/2)/dy);              /* start offset */
//               e2 = err-x1*dy-w/2;                   /* error value at offset */
//               x1 = x0-x1*sx;                                  /* start point */
//               setPixelAA(x1, y0, 255*e2/ed);           /* aliasing pre-pixel */
//               for (e2 = -w-dy-e2; e2-dy < ed; e2 -= dy)
//                  setPixel(x1 += sx, y0);              /* pixel on thick line */
//               setPixelAA(x1+sx, y0, 255*e2/ed);       /* aliasing post-pixel */
//               if (y0 == y2) return;          /* last pixel -> curve finished */
//               y0 += sy; dy += xy; err += dx; dx += xx;             /* y step */
//               if (2*err+dy > 0) {                            /* e_x+e_xy > 0 */
//                  x0 += sx; dx += xy; err += dy; dy += yy;          /* x step */
//               }
//               if (x0 != x2 && (dx+2*xx <= 0 || dy+2*yy >= 0))
//                  if (abs(y2-y0) > abs(x2-x0)) break fail;
//                  else break;                             /* other curve near */
//            } while (dx+dy+xy < 0);                  /* gradient still steep? */
//                                           /* change from steep to flat curve */
//            for (cur = err-dy-w/2, y1 = y0; cur < ed; y1 += sy, cur += dx) {
//               for (e2 = cur, x1 = x0; e2-dy < ed; e2 -= dy)
//                  setPixel(x1 -= sx, y1);              /* pixel on thick line */
//               setPixelAA(x1-sx, y1, 255*e2/ed);       /* aliasing post-pixel */
//            }
//         } else {                                               /* flat curve */
//            do {
//               ed = dx+2*dx*dy*dy/(4.*dx*dx+dy*dy);       /* approximate sqrt */
//               w = (th-1)*ed;                             /* scale line width */
//               y1 = floor((err+ed+w/2)/dx);              /* start offset */
//               e2 = y1*dx-w/2-err;                   /* error value at offset */
//               y1 = y0-y1*sy;                                  /* start point */
//               setPixelAA(x0, y1, 255*e2/ed);           /* aliasing pre-pixel */
//               for (e2 = dx-e2-w; e2+dx < ed; e2 += dx)
//                  setPixel(x0, y1 += sy);              /* pixel on thick line */
//               setPixelAA(x0, y1+sy, 255*e2/ed);       /* aliasing post-pixel */
//               if (x0 == x2) return;          /* last pixel -> curve finished */
//               x0 += sx; dx += xy; err += dy; dy += yy;             /* x step */
//               if (2*err+dx < 0)  {                           /* e_y+e_xy < 0 */
//                  y0 += sy; dy += xy; err += dx; dx += xx;          /* y step */
//               }
//               if (y0 != y2 && (dx+2*xx <= 0 || dy+2*yy >= 0))
//                  if (abs(y2-y0) <= abs(x2-x0)) break fail;
//                  else break;                             /* other curve near */
//            } while (dx+dy+xy >= 0);                  /* gradient still flat? */
//                                           /* change from flat to steep curve */
//            for (cur = -err+dx-w/2, x1 = x0; cur < ed; x1 += sx, cur -= dy) {
//               for (e2 = cur, y1 = y0; e2+dx < ed; e2 += dx)
//                  setPixel(x1, y1 -= sy);              /* pixel on thick line */
//               setPixelAA(x1, y1-sy, 255*e2/ed);       /* aliasing post-pixel */
//            }
//         }
//      }
//   plotLineWidth(x0,y0, x2,y2, th);            /* confusing error values  */
//}
//
//void plotQuadRationalBezierWidth(x0, y0, x1, y1, x2, y2, w, th)
//{                    /* plot any anti-aliased quadratic rational Bezier curve */
//   int x = x0-2*x1+x2, y = y0-2*y1+y2;
//   int xx = x0-x1, yy = y0-y1, ww, t, q;
//
//   assert(w >= 0.0);
//
//   if (xx*(x2-x1) > 0) {                             /* horizontal cut at P4? */
//      if (yy*(y2-y1) > 0)                          /* vertical cut at P6 too? */
//         if (abs(xx*y) > abs(yy*x)) {               /* which first? */
//            x0 = x2; x2 = xx+x1; y0 = y2; y2 = yy+y1;          /* swap points */
//         }                            /* now horizontal cut at P4 comes first */
//      if (x0 == x2 || w == 1.0) t = (x0-x1)/x;
//      else {                                 /* non-rational or rational case */
//         q = sqrt(4.0*w*w*(x0-x1)*(x2-x1)+(x2-x0)*(x2-x0));
//         if (x1 < x0) q = -q;
//         t = (2.0*w*(x0-x1)-x0+x2+q)/(2.0*(1.0-w)*(x2-x0));        /* t at P4 */
//      }
//      q = 1.0/(2.0*t*(1.0-t)*(w-1.0)+1.0);                 /* sub-divide at t */
//      xx = (t*t*(x0-2.0*w*x1+x2)+2.0*t*(w*x1-x0)+x0)*q;               /* = P4 */
//      yy = (t*t*(y0-2.0*w*y1+y2)+2.0*t*(w*y1-y0)+y0)*q;
//      ww = t*(w-1.0)+1.0; ww *= ww*q;                    /* squared weight P3 */
//      w = ((1.0-t)*(w-1.0)+1.0)*sqrt(q);                    /* weight P8 */
//      x = floor(xx+0.5); y = floor(yy+0.5);                   /* P4 */
//      yy = (xx-x0)*(y1-y0)/(x1-x0)+y0;                /* intersect P3 | P0 P1 */
//      plotQuadRationalBezierWidthSeg(x0,y0, x,floor(yy+0.5), x,y, ww, th);
//      yy = (xx-x2)*(y1-y2)/(x1-x2)+y2;                /* intersect P4 | P1 P2 */
//      y1 = floor(yy+0.5); x0 = x1 = x; y0 = y;       /* P0 = P4, P1 = P8 */
//   }
//   if ((y0-y1)*(y2-y1) > 0) {                          /* vertical cut at P6? */
//      if (y0 == y2 || w == 1.0) t = (y0-y1)/(y0-2.0*y1+y2);
//      else {                                 /* non-rational or rational case */
//         q = sqrt(4.0*w*w*(y0-y1)*(y2-y1)+(y2-y0)*(y2-y0));
//         if (y1 < y0) q = -q;
//         t = (2.0*w*(y0-y1)-y0+y2+q)/(2.0*(1.0-w)*(y2-y0));        /* t at P6 */
//      }
//      q = 1.0/(2.0*t*(1.0-t)*(w-1.0)+1.0);                 /* sub-divide at t */
//      xx = (t*t*(x0-2.0*w*x1+x2)+2.0*t*(w*x1-x0)+x0)*q;               /* = P6 */
//      yy = (t*t*(y0-2.0*w*y1+y2)+2.0*t*(w*y1-y0)+y0)*q;
//      ww = t*(w-1.0)+1.0; ww *= ww*q;                    /* squared weight P5 */
//      w = ((1.0-t)*(w-1.0)+1.0)*sqrt(q);                    /* weight P7 */
//      x = floor(xx+0.5); y = floor(yy+0.5);                   /* P6 */
//      xx = (x1-x0)*(yy-y0)/(y1-y0)+x0;                /* intersect P6 | P0 P1 */
//      plotQuadRationalBezierWidthSeg(x0,y0, floor(xx+0.5),y, x,y, ww, th);
//      xx = (x1-x2)*(yy-y2)/(y1-y2)+x2;                /* intersect P7 | P1 P2 */
//      x1 = floor(xx+0.5); x0 = x; y0 = y1 = y;       /* P0 = P6, P1 = P7 */
//   }
//   plotQuadRationalBezierWidthSeg(x0,y0, x1,y1, x2,y2, w*w, th);
//}
//
//void plotRotatedEllipseWidth(x, y, a, b, angle, th)
//{                                   /* plot ellipse rotated by angle (radian) */
//   int xd = a*a, yd = b*b;
//   int s = sin(angle), zd = (xd-yd)*s;               /* ellipse rotation */
//   xd = sqrt(xd-zd*s), yd = sqrt(yd+zd*s);       /* surrounding rect*/
//   a = floor(xd+0.5); b = floor(yd+0.5); zd = zd*a*b/(xd*yd);
//   plotRotatedEllipseRectWidth(x-a,y-b, x+a,y+b, (4*zd*cos(angle)), th);
//}
//
//void plotRotatedEllipseRectWidth(x0, y0, x1, y1, zd, th)
//{                  /* rectangle enclosing the ellipse, integer rotation angle */
//   int xd = x1-x0, yd = y1-y0, w = xd*yd;
//   if (w != 0.0) w = (w-zd)/(w+w);                    /* squared weight of P1 */
//   assert(w <= 1.0 && w >= 0.0);                /* limit angle to |zd|<=xd*yd */
//   xd = floor(xd*w+0.5); yd = floor(yd*w+0.5);       /* snap to int */
//   plotQuadRationalBezierWidthSeg(x0,y0+yd, x0,y0, x0+xd,y0, 1.0-w, th);
//   plotQuadRationalBezierWidthSeg(x0,y0+yd, x0,y1, x1-xd,y1, w, th);
//   plotQuadRationalBezierWidthSeg(x1,y1-yd, x1,y1, x1-xd,y1, 1.0-w, th);
//   plotQuadRationalBezierWidthSeg(x1,y1-yd, x1,y0, x0+xd,y0, w, th);
//}
//
//void plotCubicBezierWidth(x0, y0, x1, y1, x2, y2, x3, y3, th)
//{                                              /* plot any cubic Bezier curve */
//   int n = 0, i = 0;
//   int xc = x0+x1-x2-x3, xa = xc-4*(x1-x2);
//   int xb = x0-x1-x2+x3, xd = xb+4*(x1+x2);
//   int yc = y0+y1-y2-y3, ya = yc-4*(y1-y2);
//   int yb = y0-y1-y2+y3, yd = yb+4*(y1+y2);
//   int fx0 = x0, fx1, fx2, fx3, fy0 = y0, fy1, fy2, fy3;
//   int t1 = xb*xb-xa*xc, t2, t = new Array(7);
//                                 /* sub-divide curve at gradient sign changes */
//   if (xa == 0) {                                               /* horizontal */
//      if (abs(xc) < 2*abs(xb)) t[n++] = xc/(2.0*xb);  /* one change */
//   } else if (t1 > 0.0) {                                      /* two changes */
//      t2 = sqrt(t1);
//      t1 = (xb-t2)/xa; if (abs(t1) < 1.0) t[n++] = t1;
//      t1 = (xb+t2)/xa; if (abs(t1) < 1.0) t[n++] = t1;
//   }
//   t1 = yb*yb-ya*yc;
//   if (ya == 0) {                                                 /* vertical */
//      if (abs(yc) < 2*abs(yb)) t[n++] = yc/(2.0*yb);  /* one change */
//   } else if (t1 > 0.0) {                                      /* two changes */
//      t2 = sqrt(t1);
//      t1 = (yb-t2)/ya; if (abs(t1) < 1.0) t[n++] = t1;
//      t1 = (yb+t2)/ya; if (abs(t1) < 1.0) t[n++] = t1;
//   }
//   t1 = 2*(xa*yb-xb*ya); t2 = xa*yc-xc*ya;      /* divide at inflection point */
//   i = t2*t2-2*t1*(xb*yc-xc*yb);
//   if (i > 0) {
//      i = sqrt(i);
//      t[n] = (t2+i)/t1; if (abs(t[n]) < 1.0) n++;
//      t[n] = (t2-i)/t1; if (abs(t[n]) < 1.0) n++;
//   }
//   for (i = 1; i < n; i++)                         /* bubble sort of 4 points */
//      if ((t1 = t[i-1]) > t[i]) { t[i-1] = t[i]; t[i] = t1; i = 0; }
//
//   t1 = -1.0; t[n] = 1.0;                               /* begin / end points */
//   for (i = 0; i <= n; i++) {                 /* plot each segment separately */
//      t2 = t[i];                                /* sub-divide at t[i-1], t[i] */
//      fx1 = (t1*(t1*xb-2*xc)-t2*(t1*(t1*xa-2*xb)+xc)+xd)/8-fx0;
//      fy1 = (t1*(t1*yb-2*yc)-t2*(t1*(t1*ya-2*yb)+yc)+yd)/8-fy0;
//      fx2 = (t2*(t2*xb-2*xc)-t1*(t2*(t2*xa-2*xb)+xc)+xd)/8-fx0;
//      fy2 = (t2*(t2*yb-2*yc)-t1*(t2*(t2*ya-2*yb)+yc)+yd)/8-fy0;
//      fx0 -= fx3 = (t2*(t2*(3*xb-t2*xa)-3*xc)+xd)/8;
//      fy0 -= fy3 = (t2*(t2*(3*yb-t2*ya)-3*yc)+yd)/8;
//      x3 = floor(fx3+0.5); y3 = floor(fy3+0.5);     /* scale bounds */
//      if (fx0 != 0.0) { fx1 *= fx0 = (x0-x3)/fx0; fx2 *= fx0; }
//      if (fy0 != 0.0) { fy1 *= fy0 = (y0-y3)/fy0; fy2 *= fy0; }
//      if (x0 != x3 || y0 != y3)                            /* segment t1 - t2 */
//         plotCubicBezierSegWidth(x0,y0, x0+fx1,y0+fy1, x0+fx2,y0+fy2, x3,y3, th);
//      x0 = x3; y0 = y3; fx0 = fx3; fy0 = fy3; t1 = t2;
//   }
//}
//
//void plotCubicBezierSegWidth(x0,y0, x1,y1, x2,y2, x3,y3, th)
//{                     /* split cubic Bezier segment in two quadratic segments */
//   int x = floor((x0+3*x1+3*x2+x3+4)/8),
//       y = floor((y0+3*y1+3*y2+y3+4)/8);
//   plotQuadRationalBezierWidthSeg(x0,y0,
//      floor((x0+3*x1+2)/4),floor((y0+3*y1+2)/4), x,y, 1,th);
//   plotQuadRationalBezierWidthSeg(x,y,
//      floor((3*x2+x3+2)/4),floor((3*y2+y3+2)/4), x3,y3, 1,th);
//}
//
