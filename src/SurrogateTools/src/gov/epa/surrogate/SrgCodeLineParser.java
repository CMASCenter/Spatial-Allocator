package gov.epa.surrogate;

public class SrgCodeLineParser {

	private String tag;

	private Surrogates surrogates;

	public SrgCodeLineParser(Surrogates surrogates) {
		this.surrogates = surrogates;
		this.tag = "SRGDESC";
	}

	// line should starts with "SRGDESC
	public void parse(String line) throws Exception {
		if (line.startsWith("#SRGDESC")) {
			String tagLessLine = stripTag(line);
			String[] tokens = parse(tagLessLine, line);
			surrogates.addSurrogate(srgCode(tokens[0].trim()), tokens[1].trim());
		}
	}

	private int srgCode(String code) throws Exception {
		try {
			return Integer.parseInt(code);
		} catch (NumberFormatException e) {
			throw new Exception("Expected int for surrogate code but found '" + code + "'");
		}
	}

	private String[] parse(String tagLessLine, String completeLine) throws Exception {
		String[] tokens = tokenizer(tagLessLine);
		if (tokens == null) {
			throw new Exception("The line '" + completeLine
					+ "' is not in the correct format. Expected format #SRGDESC=<srg id>,<srg name>");
		}
		return tokens;
	}

	private String[] tokenizer(String tagLessLine) {
		int index = tagLessLine.indexOf(",");
		if (index != -1) {
			String[] tokens = new String[2];
			tokens[0] = tagLessLine.substring(0, index).trim();
			tokens[1] = stripQuotes(tagLessLine.substring(index+1, tagLessLine.length()).trim());
			return tokens;
		}
		return null;
	}

	private String stripQuotes(String token) {
		if (token.startsWith("'") && token.endsWith("'"))
			return token.substring(1, token.length() - 1);

		if (token.startsWith("\"") && token.endsWith("\""))
			return token.substring(1, token.length() - 1);

		return token;
	}

	private String stripTag(String line) throws Exception {
		String[] tokens = line.split(tag);
		String token = tokens[1].trim();
		if (!token.startsWith("=")) {
			throw new Exception("Expecting '=' after " + tag + " tag");
		}
		return token.substring(1).trim();
	}
}
