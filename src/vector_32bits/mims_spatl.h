#ifndef MIMS_SPATL_INCLUDED

#define MIMS_SPATL_INCLUDED

/** mims_spatl.h: global type definitions for MIMS Spatial Allocator */
#include "gpc.h"
#include "io.h"

#define MISSING -999999.0
#define MISSING_N -999999
#define MISSING_S "-999999"

#define MIN(x,y) ((x)<(y) ? (x):(y))
#define MAX(x,y) ((x)>(y) ? (x):(y))


#define OVERLAP0(begin1, end1, begin2, end2) \
        (MIN(end1, end2) >= MAX(begin1, begin2))

/* determine whether an overlap exists in the x direction */
#define OVERLAPx(x1,x2) OVERLAP0(x1->xmin,x1->xmax, x2->xmin, x2->xmax)
/* determine whether an overlap exists in the y direction */
#define OVERLAPy(y1,y2) OVERLAP0(y1->ymin,y1->ymax, y2->ymin, y2->ymax)

/* determine whether the bounding boxes of two polygons overlap */
#define OVERLAP2(a,b) (OVERLAPx((a),(b)) && OVERLAPy((a),(b)))

#define SOLID_POLYGON 0
#define NOT_A_HOLE 0
#define IS_A_HOLE 1
#define ABS(a) ((a) >= 0 ? (a) : (-a))

#define   SINUGRD3   (11)

typedef enum { FALSE, TRUE } bool; 

/* where a vertex lies in relation to a polygon */
typedef enum {
  OUT,
  IN,
  VTX,
  EDGE,
  ISC
} vertex_position_types;

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

/* types of processing the program can do */
typedef	enum { 
  SURROGATE,
  AGGREGATE,
  AVERAGE,
  CONVERT_BELD,
  CONVERT_SHAPE,
  FILTER_SHAPE,
  ALLOCATE,
  OVERLAY
} JobType;

/* map gpc types to other names */
typedef gpc_vertex Vertex;
typedef gpc_vertex_list Shape;
typedef gpc_polygon PolyShape;

/* a bounding box for an object */
typedef struct _BoundingBox {
  double xmin;
  double ymin;
  double xmax;
  double ymax;
} BoundingBox;

/* map projection information */
typedef struct _MapProjInfo {
  char *earth_ellipsoid;
  char *custom_proj_str;
  double p_alp;
  double p_bet;
  double p_gam;
  double xcent;
  double ycent;
  double xorig;
  double yorig;
  double xcell;
  double ycell;
  char *gridname;
  int ctype;
  int ncols;
  int nrows;
} MapProjInfo;


/* a description of a shape file attribute */
typedef struct _AttributeDesc {
  char *name;
  int type; /* FTDouble, FTInteger, or FTString */
  int category; /* Surrogate category */
} AttributeDesc;

/* all attributes for a shape file */
typedef struct _AttributeHeader {
  int num_attr;
  AttributeDesc **attr_desc;
} AttributeHeader;

typedef union _AttributeValue {
  double val;
  int ival;
  char *str;
} AttributeValue;

typedef struct _Parent {
  struct _PolyParent *pp;
  PolyShape *ps;
  int index;
} Parent;

/* tracks the parent polygons for an intersection of 2 polygons */
typedef struct _PolyParent {
  Parent *p1;
  Parent *p2;
} PolyParent;

/* a linked list item to store polygons, their parents and bounding boxes */
typedef struct _PolyShapeList {
  PolyShape *ps;
  PolyParent *pp;
  BoundingBox *bb;
  struct _PolyShapeList *next;
  struct _PolyShapeList *prev;
} PolyShapeList;

/* a set of shapes (point, line or polygon), such as those that might be 
 * specified in a shape file, with all associated info */
typedef struct _PolyObject {
  BoundingBox *bb;
  int nSHPType;
  int nObjects;
  char *name;
  MapProjInfo *map;
  AttributeHeader *attr_hdr;
  AttributeValue **attr_val;
  PolyShapeList *plist;
  struct _PolyObject *parent_poly1;
  struct _PolyObject *parent_poly2;
} PolyObject;

typedef struct _Isect {
  Vertex v;
  int type;
} Isect;

/* a structure that contains information about how one polygon intersects
 * other polygons in a separate set of polys.  Used by sum2poly */
typedef struct _PolyIntStruct {
  int numIntersections;
  int *intIndices;   /* the indices of the intersecting obects - malloc */
  double * intValues;  /* the values for each intersection */
} PolyIntStruct;

typedef struct _PointFileInfo {
  char *name;
  int index;
} PointFileIndex;


/* function prototypes */

PolyObject *PolyReader(char *, char *, MapProjInfo *, BoundingBox *, MapProjInfo *);
PolyObject *PolyShapeReader(char *, MapProjInfo *, BoundingBox *, MapProjInfo *);
int PolyMSahpeInOne(PolyObject *);
PolyObject *RegularGridReader(char *, BoundingBox *, MapProjInfo *, bool useBBoptimization);
PolyObject *EGridReader(char *, char *, BoundingBox *, MapProjInfo *, bool useBBoptimization);
PolyObject *IoapiInputReader(char *, MapProjInfo *, MapProjInfo *, bool useBBoptimization);
PolyObject *VariableGridReader(char *name, BoundingBox *bbox,  MapProjInfo *mpinfo); 
PolyObject *VariableGridReader1(char *name, BoundingBox *bbox,  MapProjInfo *mpinfo);
PolyObject *FractionalVegetationReader(char *, MapProjInfo *, BoundingBox *, MapProjInfo *);
PolyObject *getNewPoly(int);
PolyShape *getNewPolyShape(int);
Shape *getNewShape(int);
void freeShape(Shape *s);
PolyParent *newPolyParent(Parent *, Parent *);
Parent *newParent(PolyParent *, PolyShape *, int);
AttributeHeader *getNewAttrHeader(void);
BoundingBox *newBBox(double, double, double, double);
BoundingBox *newBoundingBox(PolyShape *);
void polyShapeIncl(PolyShapeList **, PolyShape *, PolyParent *);
void freePolyShape(PolyShape *);
int addNewAttrHeader(AttributeHeader *, char *, int);
int attachDBFAttribute(PolyObject *, char *, char *);
int attachVegeAttribute(PolyObject *, char *, char *);
int printPoly(PolyObject *);
int addLineSegment(Vertex *, Vertex *, PolyShape *, int isHole);
intersection_type SegSegInt( Vertex *, Vertex *, Vertex *, Vertex *, Vertex *);
int polyIsect(PolyObject *, PolyObject *, PolyObject *, bool isShapeOverlay);
int polyUnion(PolyObject *inpoly, PolyObject *p);
double Area(Shape *);
double PolyArea(PolyShape *);
double Length(Shape *);
double PolyLength(PolyShape *);
MapProjInfo *getNewMap(void);
int mimsProject(PolyObject *, MapProjInfo *);
JobType mimsJobType(char *);
int printBoundingBox(BoundingBox *);
int copyBBoxToFrom(BoundingBox *, BoundingBox *);
int projectPoint ( double x, double y, double *newx, double *newy);
int storeProjection ( MapProjInfo *inproj, MapProjInfo *outproj );
int compareLatLongDatum ( MapProjInfo *inMap1, MapProjInfo *inMap2 );
int compareProjection ( MapProjInfo *inMap1, MapProjInfo *inMap2 );
int fillBBox(BoundingBox *bb, double xmin, double ymin, double xmax, double ymax);
double getPolyIntValue(PolyIntStruct *polyInts, int i, int j, int n1);
int setPolyIntValue(PolyIntStruct *polyInts, int i, int j, double newVal, int n1);
int getNumIntersections(PolyIntStruct *polyInts, int i, int n1);
MapProjInfo *getFullMapProjection(char *, char *);
JobType getSAJobType();

int sum1Poly(PolyObject *poly, double **psum, int *pn1, int attr_id,
    int use_weight_attr_value);
int sum2Poly(PolyObject *dwg_poly, double ***psum, int *pnum_data_polys,
   int *pnum_grid_polys, int attr_id, PolyIntStruct **polyIntInfoPtr,
   int use_weight_attr_value);
int reportSurrogate(PolyObject *poly, char *ename, int use_weight_val,
  char *gridOutFileName);
int createConvertOutput(PolyObject *poly, char *ename);
int allocate(PolyObject *poly, char *ename, int use_weight_val);
int avg1Poly(PolyObject * poly, double **pavg, int *pnum_data_polys, int attr_id, int use_weight_attr_value);
int typeAreaPercent(PolyObject *poly, double ***psum, double *gridA, int attr_id, int numTypes, char ***list);
void trim(char *inString, char *outString);
int getMapProjFromGriddesc(char *gname, MapProjInfo *map);
PolyObject *BoundingBoxReader(MapProjInfo *file_mapproj, MapProjInfo *output_mapproj);
PolyObject *PolygonFileReader(char *ename, MapProjInfo *file_mapproj, MapProjInfo *output_mapproj);
PolyObject *PointFileReader(char *ename, MapProjInfo *file_mapproj, MapProjInfo *output_mapproj);
int reportOverlays(PolyObject *poly);
void freePolyObject(PolyObject *p);
int discreteOverlap( PolyObject *poly, int **pmax, int *pn1, int attr_id);
void addNewVertices(Shape *shp, int n);
PolyObject *calculateCentroid(PolyObject *p);
gpc_vertex getCentroidVertex(PolyShape *ps);
MapProjInfo *copyMapProj(MapProjInfo *inMap);
PolyObject *getCentroidPoly(PolyObject *p);

#endif
