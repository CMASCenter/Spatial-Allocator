FC = pgf90
IOAPILIB = /nas01/depts/ie/cempd/apps/ioapi_31/20120409/Linux2_x86_64pg_pgcc_nomp_103
NETCDFLIB=/nas01/depts/ie/cempd/apps/netcdf/netcdf-3.6.3_pgi_10.3_killdevil/lib
SMKLIB=/proj/ie/proj/staff/bbaek/SMOKEv31/subsys/smoke/Linux2_x86_64pg
EDSSLIB=/proj/ie/proj/staff/bbaek/SMOKEv31/subsys/edss_tools/Linux2_x86_64pg

FFLAGS = -g -O2 -Mextend -Mfixed -I$(IOAPILIB)
LDFLAGS = -L$(SMKLIB) -lsmoke -L$(EDSSLIB) -ledsstools -L$(IOAPILIB) -lioapi -L$(NETCDFLIB) -lnetcdf

SOURCES = convert_txt2ncf.f
TARGET =  convert_txt2ncf_polar.x

$(TARGET): $(SOURCES:.f=.o)
	$(FC) -o $(TARGET) $(SOURCES:.f=.o) $(LDFLAGS)

%.o: %.f
	$(FC) -c $(FFLAGS) $<

clean:	
	rm -f *~ *.o core *.stb $(TARGET)
