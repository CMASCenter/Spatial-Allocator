/*
This code is described in "Computational Geometry in C" (Second Edition),
Chapter 7.  It is not written to be comprehensible without the 
explanation in that book.

This program reads a polygon P followed by query points from stdin.
The input format is:
	n
	x0 y0
	x1 y1 
	...
	xn-1 yn-1
	qx qy
	qx qy
	qx qy
	...
For each query point q, InPoly returns one of four char's:
	i : q is strictly interior to P
	o : q is strictly exterior to P
	v : q is a vertex of P
	e : q lies on the relative interior of an edge of P
These represent mutually exclusive categories.
For an explanation of the code, see Chapter 7 of 
"Computational Geometry in C (Second Edition)."

Written by Joseph O'Rourke, contributions by Min Xu, June 1997.
Questions to orourke@cs.smith.edu.
--------------------------------------------------------------------
This code is Copyright 1998 by Joseph O'Rourke.  It may be freely 
redistributed in its entirety provided that this copyright notice is 
not removed.
--------------------------------------------------------------------
*/
#include <stdio.h>
#include <math.h>

#include "mims_spatl.h"

/*
InPoly returns a integer
*/
int InPoly( Vertex *q, Shape *p )
{
  int	 i, i1;      /* point index; i1 = i-1 mod n */
  double x;          /* x intersection of e with ray */
  int	 Rcross = 0; /* number of right edge/ray crossings */
  int    Lcross = 0; /* number of left edge/ray crossings */
  int n;
  Vertex *v;
  double viy, vi1y;

  n = p->num_vertices;
  v = p->vertex;

  /* For each edge e=(i-1,i), see if crosses ray. */
  for( i = 0; i < n; i++ ) {

    /* First see if q is a vertex. */
    if ( v[i].x==q->x && v[i].y==q->y ) return VTX;

    i1 = ( i + n - 1 ) % n;
    
    /* if e "straddles" the x-axis... */

    viy  = v[i].y - q->y;
    vi1y = v[i1].y - q->y;

    if( ( viy > 0.0 ) != ( vi1y > 0.0 ) ) {
      
      /* e straddles ray, so compute intersection with ray. */
      x = (v[i].x * vi1y - v[i1].x * viy)
	   / (v[i1].y - v[i].y);
      
      /* crosses ray if strictly positive intersection. */
      if (x > q->x) Rcross++;
    }
    
    if( ( viy < 0.0 ) != ( vi1y < 0.0 ) ) {
      /* e straddles ray, so compute intersection with ray. */
      x = (v[i].x * vi1y - v[i1].x * viy)
	   / (v[i1].y - v[i].y);
      
      /* crosses ray if strictly positive intersection. */
      if (x < q->x) Lcross++;
    }	
  }
  /* q on the edge if left and right cross are not the same parity. */
  if( ( Rcross % 2 ) != (Lcross % 2 ) )
    return EDGE;
  
  /* q inside iff an odd number of crossings. */
  if ((Rcross % 2) == 1) return IN;
  else return OUT;

}

