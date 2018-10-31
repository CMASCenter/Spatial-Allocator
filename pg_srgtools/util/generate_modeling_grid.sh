#!/bin/bash
# create_grid_900921.sh modified 8/3/2016 from below
# create_grid_900915.sh modified 8/30/2016 to change projection from 900915 to 900921
# create_grid.sh original by Catherine Seppanen
# created 6/9/2016 Jo Ellen Brandmeyer, Institute for the Environment
# create a grid and load it into a new table in the surrogates database, conus36km_062016 schema
# grid can be rectangular (xcellsize, ycellsize) instead of square (cellsize)
# conus12km_082016 schema
 
 dbname=surrogates
 schemaname=public
 server=localhost
 user=dyang

######################################################################
# '4CALIF1'
# 'LAM_40N97W'  -2400000.000  -732000.000      4000.000      4000.000 225 325   1 

#  tblname='calif4k_225x325'   # name of table in schema

#  proj=900921                  # value of srid in spatial_ref_sys data table
#  xorig=-2400000               # x-coordinate of lower-left corner (LLC) of grid
#  yorig=-732000                # y-coordinate of LLC of grid
#  xcellsize=4000               # cell size in x-direction
#  ycellsize=4000               # cell size in y-direction
#  cols=225                     # number of columns (x)
#  rows=325                     # number of rows (y)

######################################################################
# 'HEMI108_187x187'
# 'POLAR_HEMI'  -10098000.000  -10098000.000    108000.000    108000.000 187 187   1

# proj=900915
# xorig=-10098000
# yorig=-10098000
# xcellsize=108000
# ycellsize=108000
# cols=187
# rows=187
# 
# tblname='hemi108km_187x187'

######################################################################
# 'CONUS36KM_172x148'
# 'LAM_40N97W' -2952000 -2772000 36000 36000 172 148 1

# tblname='conus36km_172x148'	# name of table in schema
#
#  proj=900921			# value of srid in spatial_ref_sys data table
#  xorig=-2952000		# x-coordinate of lower-left corner (LLC) of grid 
#  yorig=-2772000		# y-coordinate of LLC of grid
#  xcellsize=36000		# cell size in x-direction
#  ycellsize=36000		# cell size in y-direction
#  cols=172			# number of columns (x)
#  rows=148			# number of rows (y)
######################################################################
# 'CONUS36KM_148x112'
# 'LAM_40N97W' -2736000 -2088000 36000 36000 148 112 1

# tblname='conus36km_148x112'	# name of table in schema
#
# proj=900921			# value of srid in spatial_ref_sys data table
# xorig=-2736000		# x-coordinate of lower-left corner (LLC) of grid 
# yorig=-2088000		# y-coordinate of LLC of grid
# xcellsize=36000		# cell size in x-direction
# ycellsize=36000		# cell size in y-direction
# cols=148			# number of columns (x)
# rows=112			# number of rows (y)
#
######################################################################
# begin new section

# 'us12k_444x336'
# 'LAM_40N97W'    -2736000.000   -2088000.00    12000.000     12000.000  444 336   1

tblname='us12k_444x336'	# name of table in schema

proj=900921			# value of srid in spatial_ref_sys data table
xorig=-2736000			# x-coordinate of lower-left corner (LLC) of grid 
yorig=-2088000			# y-coordinate of LLC of grid
xcellsize=12000			# cell size in x-direction
ycellsize=12000			# cell size in y-direction
cols=444			# number of columns (x)
rows=336			# number of rows (y)

######################################################################
# begin new section

#'us4k'
#'LAM_40N97W'    -2736000.000   -2088000.00     4000.000     4000.000  1332 1008   1

#tblname='us4k_1332x1008'	# name of table in schema

#proj=900921			# value of srid in spatial_ref_sys data table
#xorig=-2736000			# x-coordinate of lower-left corner (LLC) of grid 
#yorig=-2088000			# y-coordinate of LLC of grid
#xcellsize=4000			# cell size in x-direction
#ycellsize=4000			# cell size in y-direction
#cols=1332			# number of columns (x)
#rows=1008			# number of rows (y)

# do not modify below this line
######################################################################

PGPASSWORD="sepia6#vibes" $PGBIN/psql -h $server -U $user -q $dbname << END		# start PostgreSQL session on dbname from shell, quiet mode, submit commands until END
DROP TABLE IF EXISTS $schemaname.$tblname;
CREATE TABLE $schemaname.$tblname (	-- create a table tblname in schema schemaname with colnum (x-dir), rownum (y-dir)
  colnum INT NOT NULL,
  rownum INT NOT NULL,
  gridcell geometry(Polygon, $proj),	-- and a geometry column of type polygon
  PRIMARY KEY (colnum, rownum)		-- define primary key as multifield key
);
CREATE INDEX ON $schemaname.$tblname USING GIST (gridcell);	-- create spatial index on the geometry column
END

# ended psql process so back to bash script

for colnum in `seq 1 $cols`;		# outer loop: step across x-direction 
do
  for rownum in `seq 1 $rows`;		# inner loop: step across y-direction
  do					# compute coordinates of the 4 corners of the grid cell
    # bottom-left corner
    x1=$[xorig + (colnum - 1) * xcellsize]	# subtract 1 to make base 1 calculation
    y1=$[yorig + (rownum - 1) * ycellsize]
    
    # top-left corner
    x2=$x1
    y2=$[y1 + ycellsize]
    
    # top-right corner
    x3=$[x1 + xcellsize]
    y3=$y2
    
    # bottom-right corner
    x4=$x3
    y4=$y1
 
# must start and stop psql process around each SQL insert statement
PGPASSWORD="sepia6#vibes" $PGBIN/psql -h $server -U $user -q $dbname << END1
INSERT INTO $schemaname.$tblname (colnum, rownum, gridcell) VALUES ($colnum, $rownum, ST_GeomFromText('POLYGON(($x1 $y1, $x2 $y2, $x3 $y3, $x4 $y4, $x1 $y1))', $proj));
END1

# SQL insert statement finished, so back to nested loops in bash script
  done
done

