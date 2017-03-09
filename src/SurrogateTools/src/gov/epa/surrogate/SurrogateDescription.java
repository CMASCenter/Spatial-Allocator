package gov.epa.surrogate;

public class SurrogateDescription {

	private String region;

	private int surrogateCode;

	private String surrogateName;

	private String fileName;

	public SurrogateDescription(String[] tokens) {
		// TODO: validate tokens
		this.region = tokens[0].toLowerCase();
		this.surrogateCode = new Integer(tokens[1]).intValue();
		this.surrogateName = tokens[2];
		this.fileName = tokens[3];
	}

	public String getFileName() {
		return fileName;
	}

	public String getRegion() {
		return region;
	}

	public int getSurrogateCode() {
		return surrogateCode;
	}

	public String getSurrogateName() {
		return surrogateName;
	}
	
	public String toString() {
		return region+":"+surrogateCode+":"+surrogateName+":"+fileName;
	}
}
