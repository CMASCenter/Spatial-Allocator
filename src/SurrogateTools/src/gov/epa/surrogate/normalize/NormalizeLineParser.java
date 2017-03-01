package gov.epa.surrogate.normalize;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class NormalizeLineParser {

	private String delimiter;

	private int minimumTokens;

	public NormalizeLineParser(String delimiter, int minimumTokens) {
		this.delimiter = delimiter;
		this.minimumTokens = minimumTokens;
	}

	public NormalizeSurrogateRow parse(String line) {
		return new NormalizeSurrogateRow(process(line), minimumTokens, delimiter);
	}

	private String[] process(String line) {
		if (line.contains("!")) {
			String[] tokens = line.split("!");
			String[] firstTokenSplits = stripSpaces(tokens[0].split(delimiter));
			List<String> tokensList = new ArrayList<String>(Arrays.asList(firstTokenSplits));
			tokensList.add(tokens[1].trim());
			return tokensList.toArray(new String[0]);
		}
		return trim(line.split(delimiter));
	}
	
	private String[] trim(String[] tokens) {
		for (int i = 0; i < tokens.length; i++) {
			tokens[i] = tokens[i].trim();
		}
		return tokens;
	}

	private String[] stripSpaces(String[] tokens) {
		List<String> list = new ArrayList<String>();
		for (int i = 0; i < tokens.length; i++) {
			list.add(tokens[i].trim());
		}
		return list.toArray(new String[0]);
	}

}
