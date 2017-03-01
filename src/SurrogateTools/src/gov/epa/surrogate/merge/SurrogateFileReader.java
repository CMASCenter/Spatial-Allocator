package gov.epa.surrogate.merge;

import gov.epa.surrogate.Comment;
import gov.epa.surrogate.SrgCodeLineParser;
import gov.epa.surrogate.SurrogateFileInfo;
import gov.epa.surrogate.Surrogates;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;

public class SurrogateFileReader {

	private Surrogates surrogates;

	private BufferedReader reader;

	private List<String> comments;

	private SrgCodeLineParser srgCodeLineParser;

	private Counties counties;

	private String surrogateName;
	
	private int minTokens;
	

	public SurrogateFileReader(Surrogates surrogates, SurrogateFileInfo surrogateFileInfo)
			throws FileNotFoundException {
		this.surrogates = surrogates;
		this.reader = new BufferedReader(new FileReader(new File(surrogateFileInfo.getSurrogateFileName())));
		this.surrogateName = surrogateFileInfo.getSurrogateName();
		this.comments = new ArrayList<String>();
		this.srgCodeLineParser = new SrgCodeLineParser(surrogates);
		this.counties = new Counties();
		
		//System.out.println("Read srg:"+ surrogateName);
	}

	public void read() throws Exception {
		String line = null;
		Comment comment = new Comment();
		while ((line = reader.readLine()) != null) {
			line = line.trim();
			if (comment.isComment(line)) {
				processComment(line);
			} else {
				processData(line);
			}
		}
		reader.close();
	}

	private void processComment(String line) throws Exception {
		if (!line.startsWith("#GRID") && !line.startsWith("#POLYGON")) {
			comments.add(line);
		}
		if ( line.startsWith("#GRID") )  minTokens = 5; //6th items is ! 
		if ( line.startsWith("#POLYGON") )  minTokens = 4; //5th items is !
		
		srgCodeLineParser.parse(line);
	}

	private void processData(String line) throws Exception {
		SurrogateRow row = new SurrogateRow( line, this.minTokens );
		
		//System.out.println("Srg code:"+ row.getSurrogateCode() + "srg name:"+ surrogateName);
		
		if (verifySurrogateID(row.getSurrogateCode(), surrogates, surrogateName)) {
			counties.add(row);
			
		}
	}

	private boolean verifySurrogateID(int surrogateCode, Surrogates surrogates, String surrogateName) throws Exception {
		int surrogateID = surrogates.getSurrogateID(surrogateName);
		return (surrogateCode == surrogateID);
	}

	public Counties getCounties() {
		return counties;
	}

}
