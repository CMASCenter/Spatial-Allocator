Several tools for processing spatial surrogates are available in SurrogateTools.jar. 
All tools require Java 1.5 or later. The tools in the package are as follows:

1. SurrogateTool: A program that runs the spatial allocators to create surrogates,
merge them, and gapfill them.  See the complete user's guide at
http://www.ie.unc.edu/cempd/projects/mims/spatial/srgtool/ for more information.
Run the SurrogateTool using the command: 

java -classpath SurrogateTools.jar gov.epa.surrogate.SurrogateTool control_variables_file

Note: if you wish to use the Java version of merging and gapfilling, leave the 
SRGMERGE EXECUTABLE line out of the control variables file.

2. QATool: A program that summarizes and provides quality assurance information 
about a set of surrogates.  Four reports (not1, gapfill, nodata, and summary)
are generated for each region in the specified SRGDESC file. Run the QATool using 
the command:

java -classpath SurrogateTools.jar gov.epa.surrogate.qa.Main SRGDESC_file tolerance

3. Normalization tool: A program that "normalizes" surrogates for counties that 
do not sum to 1 and makes the sum approximately 1. This should be used with care
because surrogate values for counties / regions on the edge of the grid 
often should not sum to 1.  The tool accepts an exclude list of such counties.
Run the normalization tool with one of these commands:

java -classpath SurrogateTools.jar gov.epa.surrogate.normalize.Main SRGDESC_file exclude_list tolerance
java -classpath SurrogateTools.jar gov.epa.surrogate.normalize.Main SRGDESC_file 

[the default tolerance if left unspecified is 1e-6]


4. Gapfilling tool: A program that gapfills surrogates so that data from other
surrogates is included for counties that are not in the base surrogate. The
behavior is very similar to, and the input file is the same as, the gapfilling 
that is available in the srgmerge program. One exception is that there is no 
assumption that the lowest surrogate has all counties of interets.  Run the 
gapfilling tool using the command: 

java -classpath SurrogateTools.jar gov.epa.surrogate.gapfill.Main gapfill_input_file

5. Merging tool: A program that merges multiple surrogates to output a new
surrogate.  The behavior is very similar to, and the input file is the same as,
the merging that is available in the srgmerge program, except that only two part 
quations are accepted (e.g.a*s1+b*s2 where a+b=1.0). Another difference is that 
if values are missing for one surrogate for a county, values for the other 
surrogate are output.  Run the merging tool using the command

java -classpath SurrogateTools.jar gov.epa.surrogate.merge.Main merge_input_file
