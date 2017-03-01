package gov.epa.surrogate;

public class Comment {

	private String comment;

	public Comment() {
		comment = "#";
	}
	
	public boolean isComment(String line){
		String trim = line.trim();
		return trim.length() == 0 || trim.startsWith(comment);
	}
}
