package gov.epa.surrogate.qa;

public class LineParser {
	
	private String delimiter;
	
	private int minimumTokens;

	public LineParser(String delimiter, int minimumTokens){
		this.delimiter = delimiter;
		this.minimumTokens = minimumTokens;
	}

	public SurrogateRow parse(String line, boolean gapFilled) {
		String[] tokens = line.split(delimiter);
		return new SurrogateRow(tokens,minimumTokens,gapFilled);
	}
}
