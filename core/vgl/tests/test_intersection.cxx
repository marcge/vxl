// Some tests for vgl_intersection
// J.L. Mundy June 13, 2014

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_line_segment_2d.h>
#include <vgl/vgl_intersection.h>

void test_intersection()
{
  vcl_cout << "*****************************\n"
           << " Testing vgl_intersection\n"
           << "*****************************\n\n";
  //unit box with lower left corner at (0, 0)
  vgl_point_2d<double> pll(0.0, 0.0), pur(1.0, 1.0);
  vgl_box_2d<double> box(pll, pur);
  vgl_line_segment_2d<double> lint;//intersection line segment
  // case I - line segment completely outside box
  vgl_point_2d<double> p11(1.5, 1.5), p12(2.0, 2.0);
  vgl_line_segment_2d<double> l1(p11, p12);
  bool caseI = vgl_intersection(box, l1, lint);
  TEST("No intersection", caseI, false);
  // case II - line segment completely inside box
  vgl_point_2d<double> p21(0.2, 0.2), p22(0.8, 0.8);
  vgl_line_segment_2d<double> l2(p21, p22);
  bool caseII = vgl_intersection(box, l2, lint);
  caseII = caseII && (lint==l2);
  TEST("Lineseg inside box", caseII, true);
  // Case III - line segment intersects box and cuts line at two points
  vgl_point_2d<double> p31(0.5, -0.5), p32(0.5, 1.5);
  vgl_line_segment_2d<double> l3(p31, p32);
  bool caseIII = vgl_intersection(box, l3, lint);
  vgl_point_2d<double> p1III(0.5,0.0), p2III(0.5, 1.0);
  vgl_point_2d<double> pla = lint.point1(), plb = lint.point2();
  caseIII = caseIII && (((p1III == pla)&&(p2III == plb)) ||
			((p1III == plb)&&(p2III == pla)));
  TEST("Lineseg intersects box (2 pts.)", caseIII, true);
  // Case IV - line segment intersects box and cuts line at one point
  vgl_point_2d<double> p41(0.5, 0.5), p42(0.5, 1.5);
  vgl_line_segment_2d<double> l4(p41, p42);
  bool caseIV = vgl_intersection(box, l4, lint);
  vgl_point_2d<double> p1IV(0.5,0.5), p2IV(0.5, 1.0);
  pla = lint.point1(); plb = lint.point2();
  caseIV = caseIV && (((p1IV == pla)&&(p2IV == plb)) ||
			((p1IV == plb)&&(p2IV == pla)));
  TEST("Lineseg intersects box (1 pt.)", caseIV, true);
}


TESTMAIN(test_intersection);
