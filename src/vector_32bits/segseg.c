/*
This code is described in "Computational Geometry in C" (Second Edition),
Chapter 7.  It is not written to be comprehensible without the 
explanation in that book.

Compile:  gcc -o segseg segseg.c (or simply: make)

Written by Joseph O'Rourke.
Last modified: November 1997
Questions to orourke@cs.smith.edu.
--------------------------------------------------------------------
This code is Copyright 1998 by Joseph O'Rourke.  It may be freely 
redistributed in its entirety provided that this copyright notice is 
not removed.
--------------------------------------------------------------------
*/
#include <stdio.h>
#include <math.h>
#ifndef AIX
#define	EXIT_FAILURE 1
#endif
#define	X 0
#define	Y 1
typedef	enum { FALSE, TRUE } bool;
typedef	enum { 
  NO_INT,
  PROP_INT,
  VTX_INT,
  EDGE_AB_INT,
  EDGE_CD_INT,
  EDGE_AD_INT,
  EDGE_BC_INT,
  EDGE_AC_INT,
  EDGE_BD_INT
} intersection_type;

typedef struct
{
  double              x;            /* Vertex x component                */
  double              y;            /* vertex y component                */
} Point;


/*---------------------------------------------------------------------
Function prototypes.
---------------------------------------------------------------------*/
intersection_type ParallelInt( Point *a, Point *b, Point *c, Point *d, Point *p );
bool	Between( Point *a, Point *b, Point *c );
int	Collinear( Point *a, Point *b, Point *c );
int     AreaSign( Point *a, Point *b, Point *c );
/*-------------------------------------------------------------------*/

/*---------------------------------------------------------------------
SegSegInt: Finds the Point of intersection p between two closed
segments ab and cd.  Returns p and a char with the following meaning:
   'e': The segments collinearly overlap, sharing a Point *.
   'v': An endPoint * (vertex) of one segment is on the other segment,
        but 'e' doesn't hold.
   '1': The segments intersect properly (i.e., they share a Point * and
        neither 'v' nor 'e' holds).
   '0': The segments do not intersect (i.e., they share no Point *s).
Note that two collinear segments that share just one Point *, an endPoint *
of each, returns 'e' rather than 'v' as one might expect.
---------------------------------------------------------------------*/
intersection_type SegSegInt( Point *a, Point *b, Point *c, Point *d, Point *p )
{
   double  s, t;       /* The two parameters of the parametric eqns. */
   double num, denom;  /* Numerator and denoninator of equations. */
   intersection_type code;     /* Return char characterizing intersection. */

   denom = a->x * ( d->y - c->y ) +
           b->x * ( c->y - d->y ) +
           d->x * ( b->y - a->y ) +
           c->x * ( a->y - b->y );

   /* If denom is zero, then segments are parallel: handle separately. */
   if (denom == 0.0)
      return  ParallelInt(a, b, c, d, p);

   num =    a->x * ( d->y - c->y ) +
            c->x * ( a->y - d->y ) +
            d->x * ( c->y - a->y );
   if ( (num == 0.0) || (num == denom) ) code = VTX_INT;
   s = num / denom;

   num = -( a->x * ( c->y - b->y ) +
            b->x * ( a->y - c->y ) +
            c->x * ( b->y - a->y ) );
   if ( (num == 0.0) || (num == denom) ) code = VTX_INT;
   t = num / denom;

   if      ( (0.0 < s) && (s < 1.0) &&
             (0.0 < t) && (t < 1.0) )
     code = PROP_INT;
   else if ( (0.0 > s) || (s > 1.0) ||
             (0.0 > t) || (t > 1.0) )
     code = NO_INT;

   p->x = a->x + s * ( b->x - a->x );
   p->y = a->y + s * ( b->y - a->y );

   return code;
}
intersection_type ParallelInt( Point *a, Point *b, Point *c, Point *d, Point *p )
{
  bool abc, abd, cda, cdb;
 
  p = NULL;

  if ( !Collinear( a, b, c) )
      return NO_INT;

  abc = Between( a, b, c );
  abd = Between( a, b, d );

  cda = Between( c, d, a );
  cdb = Between( c, d, b );

  if ( abc && abd ) {
    return EDGE_CD_INT;
  }

  if ( cda && cdb ) {
    return EDGE_AB_INT;
  }

  if ((abc != abd) && (cda != cdb)) {
    if ((a->x == d->x) && (a->y == d->y)) {
      p = a;
      return VTX_INT;
    }
    if ((a->x == c->x) && (a->y == c->y)) {
      p = a;
      return VTX_INT;
    }
    if ((b->x == c->x) && (b->y == c->y)) {
      p = b;
      return VTX_INT;
    }
    if ((b->x == d->x) && (b->y == d->y)) {
      p = b;
      return VTX_INT;
    }

    if (abd && cda ) return EDGE_AD_INT;
    if (abc && cdb ) return EDGE_BC_INT;
    if (abc && cda ) return EDGE_AC_INT;
    if (abd && cdb ) return EDGE_BD_INT;
  }
  
   return NO_INT;
}
/*---------------------------------------------------------------------
Returns TRUE iff Point c lies on the closed segment ab.
Assumes it is already known that abc are collinear.
---------------------------------------------------------------------*/
bool    Between( Point *a, Point *b, Point *c )
{

   /* If ab not vertical, check betweenness on x; else on y. */
   if ( a->x != b->x )
      return (bool)(((a->x <= c->x) && (c->x <= b->x)) ||
	            ((a->x >= c->x) && (c->x >= b->x)));
   else
      return (bool)(((a->y <= c->y) && (c->y <= b->y)) ||
                    ((a->y >= c->y) && (c->y >= b->y)));
}

int Collinear( Point *a, Point *b, Point *c )
{
   return AreaSign( a, b, c ) == 0;
}
int     AreaSign( Point *a, Point *b, Point *c )
{
    double area2;

    area2 = ( b->x - a->x ) * ( c->y - a->y ) -
            ( c->x - a->x ) * ( b->y - a->y );

    /* The area should be an integer. */
    if      ( area2 >  0.0 ) return  1;
    else if ( area2 <  0.0 ) return -1;
    else                     return  0;
}


