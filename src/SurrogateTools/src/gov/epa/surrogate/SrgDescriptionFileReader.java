package gov.epa.surrogate;


import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;

public class SrgDescriptionFileReader {

	private BufferedReader reader;

	private List<SurrogateDescription> srgDescriptions;

	private String header;

	private int minimumTokens;

	private Map<String,Surrogates> surrogatesMap;

	private String fileName;

	private Comment comment;

	private List<String> comments;

	private Tokenizer toekenizer;

	public SrgDescriptionFileReader(String fileName) throws Exception {
		this.fileName = fileName;
		File file = new File(fileName);
		new FileVerifier().shouldExist(file);
		reader = new BufferedReader(new FileReader(file));
		srgDescriptions = new ArrayList<SurrogateDescription>();
		surrogatesMap = new Hashtable<String,Surrogates>();
		comment = new Comment();
		comments = new ArrayList<String>();
		toekenizer = new CommaDelimitedTokenizer();
	}

	public void read() throws Exception {
		System.out.println("Reading SRGDESC file '" + fileName + "'");
		String line = null;
		headerLine();
		while ((line = reader.readLine()) != null) {
			if (!isComment(line)) {
				SurrogateDescription desc = parse(line);
				addSurrogates(desc);
				srgDescriptions.add(desc);
			}
		}
		reader.close();
		System.out.println("Mimimum surrogate items = " + getMinimumTokens());
		System.out.println("Finished reading the SRGDESC file");
	}

	private void addSurrogates(SurrogateDescription desc){
		Surrogates srgs = surrogatesMap.get(desc.getRegion());
		if (srgs == null) {
			srgs = new Surrogates(desc.getRegion());
			surrogatesMap.put(desc.getRegion(), srgs);
		}
		srgs.addSurrogate(desc.getSurrogateCode(), desc.getSurrogateName());

	}

	// assume first comment line will be a header line
	private void headerLine() throws IOException {
		String line = null;
		while ((line = reader.readLine()) != null) {
			if (isComment(line)) {
				this.header = line;
				minimumTokens(header);
				return;
			}
		}
	}

	private void minimumTokens(String header) {
		this.minimumTokens = (header.contains("GRID")) ? 5 : 4;
	}

	private SurrogateDescription parse(String line) throws Exception {
		String[] tokens = toekenizer.tokens(line);
		if (tokens.length != 4) {
			throw new Exception("Expected four tokens; line - '" + line + "'");
		}
		return new SurrogateDescription(trim(tokens));
	}

	private String[] trim(String[] tokens) {
		for (int i = 0; i < tokens.length; i++) {
			tokens[i] = tokens[i].trim();
		}
		return tokens;
	}

	private boolean isComment(String line) {
		if (comment.isComment(line)) {
			comments.add(line);
			return true;
		}
		return false;
	}

	public String getHeader() {
		return header;
	}

	public int getMinimumTokens() {
		return minimumTokens;
	}

	public SurrogateDescription[] getSurrogateDescriptions(String regionName) {
		List<SurrogateDescription> desc = new ArrayList<SurrogateDescription>();
		for (int i = 0; i < srgDescriptions.size(); i++) {
			SurrogateDescription srgDesc = srgDescriptions.get(i);
			if (srgDesc.getRegion().equals(regionName)) {
				desc.add(srgDesc);
			}
		}
		return desc.toArray(new SurrogateDescription[0]);
	}

	public Surrogates getSurrogates(String regionName) {
		return surrogatesMap.get(regionName);
	}

	public String[] getRegions() {
		List<String> regions = new ArrayList<String>();
		for (int i = 0; i < srgDescriptions.size(); i++) {
			SurrogateDescription desc = srgDescriptions.get(i);
			String region = desc.getRegion();
			if (!regions.contains(region)) {
				regions.add(region);
			}
		}
		return regions.toArray(new String[0]);
	}

	public SurrogateDescription[] getSurrogateDescriptions() {
		return srgDescriptions.toArray(new SurrogateDescription[0]);

	}
	
	public String[] getComments(){
		return comments.toArray(new String[0]);
	}

}
