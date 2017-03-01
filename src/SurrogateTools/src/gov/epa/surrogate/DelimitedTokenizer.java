package gov.epa.surrogate;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class DelimitedTokenizer {

    private Pattern p;

    public DelimitedTokenizer(String pattern) {
        p = Pattern.compile(pattern);
    }

    public String[] doTokenize(String input) {
        Matcher m = p.matcher(input);
        List<String> tokens = new ArrayList<String>();
        while (m.find()) {
            String token = input.substring(m.start(), m.end()).trim();

            if (token.startsWith("\"")) {
                tokens.add(startWithDoubleQuote(input, m, token));
            } else if (token.startsWith("'")) {
                tokens.add(startWithSingleQuote(input, m, token));
            } else {
                tokens.add(token);
            }
        }

        return tokens.toArray(new String[0]);

    }

    private String startWithDoubleQuote(String input, Matcher m, String token) {
        return startWithQuote("\"", input, m, token);
    }

    private String startWithSingleQuote(String input, Matcher m, String token) {
        return startWithQuote("'", input, m, token);
    }

    private String startWithQuote(String quote, String input, Matcher m, String token) {
        int start = m.start();
        while (!token.endsWith(quote) && m.find()) {
            token = input.substring(m.start(), m.end()).trim();
        }
        token = input.substring(start, m.end()).trim();
        return token.substring(1, token.length() - 1);
    }
}
