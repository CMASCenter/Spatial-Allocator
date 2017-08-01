          PROGRAM    CONVERT_TXT2NCF

ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c     Created by: M. Omary "omarymo@gmail.com" (12/14/2010)
c University of North Carolina Institute for the Environment
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
      USE M3UTILIO


      IMPLICIT NONE 

      
      REAL, ALLOCATABLE   :: VAL_IN  (:,:)
      REAL                :: VAL

      INTEGER            :: LOGDEV  
      INTEGER            :: JDATE0, JDATE, JDATE1, JDATE2, JTIME

      CHARACTER(300)     :: LINE
      CHARACTER(7)       :: DATE     
				      
      INTEGER            :: INID  = 96    ! # of output file for daily totals

      CHARACTER(25)      :: SEGMENT(20)  
      CHARACTER(250)     :: INNAME    ! name of text input files
      CHARACTER(16)      :: OUTntCDF  ! logbical name of ncf output files

      CHARACTER*16       PNAME 
      INTEGER            I, J, K, T, T0, T1, T2, C, N, NLINES
      DATA PNAME       /'SURROGATES'  /
      DATA OUTntCDF    /'OUT_NETCDF'/
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
C   begin body of program SUM_EMIS

        LOGDEV = INIT3()  
        CALL  GETENV("INPUTF",INNAME)
 
c.....open emission domain total txt file


        OPEN (INID, FILE=INNAME, STATUS='OLD') 

        TSTEP3D  = 10000
        SDATE3D  = 2007001
        NLAYS3D  = 1
        NVARS3D  = 1
        NTHIK3D  = 1

        VNAME3D(1) = 'FRACTION'
        UNITS3D(1) = 'fraction'
        VDESC3D(1) = 'Surogate fraction'
        FDESC3D(1) = 'Prepared by M. Omary'

c        GDTYP3D = 2          !Lambert
        GDTYP3D = 6          !Polar Stereographic
        FTYPE3D = 1          !Gridded
        VGTYP3D = IMISS3
	VGTOP3D = 100
        GDNAM3D   = 'SEMAP'
        VTYPE3D =  M3REAL  

        
c.....allocate dynamic array
c      ALLOCATE(VAL_IN  (NCOLS3D,NROWS3D))

c.....open emission domain total ncf file: one layer, time independent
        SEGMENT = ' '

          READ(INID,800, END=90 )LINE
             IF (LINE(2:5) .EQ. 'GRID' ) THEN
                CALL PARSLINE(LINE,20,SEGMENT)
                   XORIG3D = STR2REAL(SEGMENT(3))
                   YORIG3D = STR2REAL(SEGMENT(4))
                   XCELL3D = STR2REAL(SEGMENT(5))
                   YCELL3D = STR2REAL(SEGMENT(6))
                   NCOLS3D = STR2INT(SEGMENT(7))
                   NROWS3D = STR2INT(SEGMENT(8))
                   P_ALP3D = STR2REAL(SEGMENT(12))
                   P_BET3D = STR2REAL(SEGMENT(13))
                   P_GAM3D = STR2REAL(SEGMENT(14))
                   XCENT3D = STR2REAL(SEGMENT(15))
                   YCENT3D = STR2REAL(SEGMENT(16))
             END IF

c                   WRITE(*,*)LINE
                   WRITE(*,*)'XORIG3D',XORIG3D
                   WRITE(*,*)'YORIG3D',YORIG3D
                   WRITE(*,*)'XCELL3D',XCELL3D
                   WRITE(*,*)'YCELL3D',YCELL3D
                   WRITE(*,*)'NCOLS3D',NCOLS3D
                   WRITE(*,*)'NROWS3D',NROWS3D
                   WRITE(*,*)'P_ALP3D',P_ALP3D
                   WRITE(*,*)'P_BET3D',P_BET3D
                   WRITE(*,*)'P_GAM3D',P_GAM3D
                   WRITE(*,*)'XCENT3D',XCENT3D
                   WRITE(*,*)'YCENT3D',YCENT3D


c.....allocate dynamic array
      ALLOCATE(VAL_IN  (NCOLS3D,NROWS3D))

c
	JDATE   = 2007001
        T       = 0
        VAL_IN  = 0

        DO 
          READ(INID,800, END=90 )LINE
          IF(LINE(1:1) .EQ. '#' ) CYCLE

          SEGMENT = ' '
          CALL PARSLINE(LINE, 10, SEGMENT)
          I   = STR2INT(SEGMENT(3))
          J   = STR2INT(SEGMENT(4))
          VAL = STR2REAL(SEGMENT(5))
c      WRITE(*,*)I,J,VAL
          VAL_IN(I,J) = VAL_IN(I,J) + VAL
        END DO
90      CONTINUE

	 IF ( .NOT. OPEN3(OUTntCDF, FSUNKN3, PNAME) ) THEN		     
            WRITE(*, *) 'ERROR: open output file failed  ', 
     &                                       TRIM(OUTntCDF)
         END IF	
									
         IF(.NOT.WRITE3(OUTntCDF,'FRACTION',JDATE,T, VAL_IN))THEN      
            WRITE(*,*)'Could not write to',OUTntCDF
         END IF

c************  FORMATS *********************
700   FORMAT(A7)
800   FORMAT(A)

      END PROGRAM CONVERT_TXT2NCF

